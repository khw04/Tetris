#define main tetris_application_main
#include "../tetris.c"
#undef main
#include <assert.h>

int main(int argc, char **argv) {
    (void)argv;
    memset(state.board, 0, sizeof(state.board));
    state.status = GAME_START;
    state.paused = 0;
    state.current = T_BLOCK;
    state.next = I_BLOCK;
    state.rotation = 0;
    state.x = 3; state.y = 2;
    if (argc > 1) {
        state.score = 1240;
        state.lines = 12;
        for (int col = 0; col < 8; col++) {
            state.board[18][col] = col % 7 + 1;
            if (col < 3 || col > 5) state.board[17][col] = (col + 2) % 7 + 1;
        }
        state.last_lines = 2;
        state.last_points = 300;
        state.notice_until = now_ms() + 1800;
        draw_board();
        return 0;
    }

    char saved[19][8];
    memcpy(saved, state.board, sizeof(saved));
    handle_key(' ');
    assert(state.paused);
    handle_key('a'); handle_key('i'); handle_key('j');
    assert(state.x == 3 && state.y == 2 && state.rotation == 0);
    assert(memcmp(saved, state.board, sizeof(saved)) == 0);
    handle_key(' ');
    assert(!state.paused);
    handle_key('j');
    assert(state.x == 2);
    handle_key(' '); handle_key('p');
    assert(state.status == GAME_END); /* Exit also works while paused. */

    state.paused = 0;
    state.status = GAME_START;
    state.current = T_BLOCK;
    state.x = 3; state.y = get_landing_y();
    int landing = state.y;
    lock_piece();
    assert(state.board[landing][3] == T_BLOCK + 1);
    assert(state.board[landing + 1][4] == T_BLOCK + 1);

    memset(state.board, 0, sizeof(state.board));
    state.board[16][2] = J_BLOCK + 1;
    for (int row = 17; row < 19; row++)
        for (int col = 0; col < 8; col++) state.board[row][col] = col % 7 + 1;
    state.lines = 8;
    state.score = 0;
    clear_lines();
    assert(state.score == 300 && state.lines == 10 && get_level() == 2);
    assert(state.last_lines == 2 && state.last_points == 300);
    assert(state.notice_until > now_ms());
    assert(state.board[18][2] == J_BLOCK + 1); /* Color survives row shifts. */
    puts("PASS: pause, resume, exit, fixed colors, line counts, level, feedback");
    return 0;
}
