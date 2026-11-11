"""Exercise the compiled game through a real Linux pseudo-terminal."""
import fcntl
import os
import pty
import select
import signal
import struct
import subprocess
import sys
import tempfile
import termios
import time
from pathlib import Path

binary = str(Path(sys.argv[1] if len(sys.argv) > 1 else './tetris').resolve())


class Session:
    def __enter__(self):
        self.directory = tempfile.TemporaryDirectory(prefix='tetris-terminal-')
        self.master, self.slave = pty.openpty()
        fcntl.ioctl(self.slave, termios.TIOCSWINSZ, struct.pack('HHHH', 30, 80, 0, 0))
        self.original = termios.tcgetattr(self.slave)
        self.process = subprocess.Popen([binary], stdin=self.slave, stdout=self.slave,
                                        stderr=self.slave, cwd=self.directory.name)
        self.wait_for('SELECT')
        return self

    def send(self, text):
        os.write(self.master, text.encode())

    def wait_for(self, marker):
        output = b''
        deadline = time.monotonic() + 5
        while time.monotonic() < deadline:
            if select.select([self.master], [], [], 0.1)[0]:
                output += os.read(self.master, 65536)
                if marker.encode() in output:
                    return output
        raise AssertionError((marker, self.process.poll(), output[-1000:]))

    def finish(self, expected=0):
        assert self.process.wait(timeout=3) == expected
        assert termios.tcgetattr(self.slave) == self.original

    def __exit__(self, *unused):
        if self.process.poll() is None:
            self.process.kill()
            self.process.wait()
        os.close(self.master)
        os.close(self.slave)
        self.directory.cleanup()


with Session() as session:
    session.send('9\n')
    session.wait_for('SELECT')
    session.send('1junk\n')
    session.wait_for('SELECT')
    session.send('x' * 1000 + '\n')
    session.wait_for('SELECT')
    session.send('4\n')
    session.finish()

with Session() as session:
    session.send('\x04')  # EOF at the menu must not spin forever.
    session.finish()

with Session() as session:
    path = Path(session.directory.name, 'result.txt')
    existing = 'old,12,2026,11,11,0,0,0'
    path.write_text(existing)  # No trailing newline.
    session.send('1\n')
    session.wait_for('PLAYING')
    session.send(' ')
    session.wait_for('PAUSED')
    session.send(' ')
    session.wait_for('PLAYING')
    session.send('p')
    session.wait_for('SESSION COMPLETE')
    session.send('1\n')
    session.wait_for('이름을 입력하세요')
    session.send('x' * 100 + '\n')
    session.wait_for('1~29바이트')
    assert path.read_text() == existing
    session.send('1\n')
    session.wait_for('이름을 입력하세요')
    session.send('한글 이름\n')
    session.wait_for('결과가 저장되었습니다')
    lines = path.read_text().splitlines()
    assert lines[0] == existing and lines[1].startswith('한글 이름,') and len(lines) == 2
    session.send('2\n')
    session.wait_for('PLAYING')
    session.send('p')
    session.wait_for('SESSION COMPLETE')
    session.send('3\n')
    session.wait_for('SELECT')
    session.send('4\n')
    session.finish()

with Session() as session:
    Path(session.directory.name, 'result.txt').write_text(''.join(
        f'player{i},{i},2026,11,11,0,0,0\n' for i in range(1205)))
    session.send('2\n')
    session.wait_for('검색할 이름')
    session.send('player1204\n')
    session.wait_for('1위. player1204')
    session.send('\n')
    session.wait_for('SELECT')
    session.send('4\n')
    session.finish()

for where, number in [('menu', signal.SIGINT), ('game', signal.SIGINT),
                       ('pause', signal.SIGTERM), ('name', signal.SIGINT),
                       ('summary', signal.SIGHUP)]:
    with Session() as session:
        if where != 'menu':
            session.send('1\n')
            session.wait_for('PLAYING')
        if where == 'pause':
            session.send(' ')
            session.wait_for('PAUSED')
        if where in ('name', 'summary'):
            session.send('p')
            session.wait_for('SESSION COMPLETE')
        if where == 'name':
            session.send('1\n')
            session.wait_for('이름을 입력하세요')
        os.kill(session.process.pid, number)
        session.finish(128 + number)

print('PASS: invalid/long input, EOF, save/retry, newest search, 5 signal/terminal restoration paths')
