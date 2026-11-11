CC = gcc
CFLAGS = -std=gnu11 -O2 -Wall -Wextra -Wpedantic
TEST_BINS = .test-build/test_core .test-build/test_ghost .test-build/test_ui

.PHONY: all run test test-terminal clean
all: tetris

tetris: tetris.c
	$(CC) $(CFLAGS) -o $@ $<

run: tetris
	./tetris

.test-build/%: tests/%.c tetris.c
	mkdir -p .test-build
	$(CC) $(CFLAGS) -o $@ $<

test: $(TEST_BINS)
	./.test-build/test_core
	./.test-build/test_ghost
	./.test-build/test_ui

test-terminal: tetris
	python3 tests/test_terminal.py ./tetris

clean:
	rm -f tetris $(TEST_BINS)
