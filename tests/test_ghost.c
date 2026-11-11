#define main tetris_application_main
#include "../tetris.c"
#undef main
#include <assert.h>
#include <limits.h>

static const char (*shape(int number))[4][4] {
    switch (number) {
        case 0: return i_block;
        case 1: return t_block;
        case 2: return s_block;
        case 3: return z_block;
        case 4: return l_block;
        case 5: return j_block;
        default: return o_block;
    }
}

/* Independent oracle: minimum distance from any occupied cell to the
   first fixed cell below it, or to the floor. */
static int expected_landing(void) {
    int distance = INT_MAX;
    const char (*cells)[4][4] = shape(state.current);
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            if (!cells[state.rotation][row][col]) continue;
            int below = state.y + row + 1;
            while (below < 19 && !state.board[below][state.x + col]) below++;
            int gap = below - (state.y + row) - 1;
            if (gap < distance) distance = gap;
        }
    }
    return state.y + distance;
}

static void check_preview(int *count) {
    char saved_board[19][8];
    memcpy(saved_board, state.board, sizeof(saved_board));
    int saved_x = state.x, saved_y = state.y, saved_state = state.rotation;
    int saved_number = state.current, saved_next = state.next;
    int saved_game = state.status;
    long saved_point = state.score;
    int landing = get_landing_y();
    assert(landing == expected_landing());
    assert(can_move(0, landing - state.y, 0));
    assert(!can_move(0, landing - state.y + 1, 0));
    assert(state.x == saved_x && state.y == saved_y && state.rotation == saved_state);
    assert(state.current == saved_number && state.next == saved_next);
    assert(state.score == saved_point && state.status == saved_game);
    assert(memcmp(saved_board, state.board, sizeof(saved_board)) == 0);
    (*count)++;
}

static void check_render(int expected_purple_cells) {
    FILE *capture = tmpfile();
    assert(capture != NULL);
    fflush(stdout);
    int saved_stdout = dup(STDOUT_FILENO);
    assert(saved_stdout >= 0);
    assert(dup2(fileno(capture), STDOUT_FILENO) >= 0);
    draw_board();
    fflush(stdout);
    assert(dup2(saved_stdout, STDOUT_FILENO) >= 0);
    close(saved_stdout);
    rewind(capture);
    char output[16384];
    size_t length = fread(output, 1, sizeof(output) - 1, capture);
    output[length] = '\0';
    fclose(capture);
    int purple_cells = 0;
    for (char *cursor = output; (cursor = strstr(cursor, "\033[45m  \033[0m")); cursor++) {
        purple_cells++;
    }
    assert(purple_cells == expected_purple_cells + 1); /* Side-panel legend. */
}

int main(void) {
    int count = 0;
    srand(42);
    state.score = 17;
    state.status = GAME_START;
    state.next = O_BLOCK;
    for (int scenario = 0; scenario < 33; scenario++) {
        for (int row = 0; row < 19; row++) {
            for (int col = 0; col < 8; col++) {
                state.board[row][col] = scenario && row >= 4 && rand() % 5 == 0;
            }
        }
        for (state.current = 0; state.current < 7; state.current++) {
            for (state.rotation = 0; state.rotation < 4; state.rotation++) {
                for (state.x = -3; state.x < 8; state.x++) {
                    for (state.y = 0; state.y < 19; state.y++) {
                        if (can_move(0, 0, 0)) check_preview(&count);
                    }
                }
            }
        }
    }

    memset(state.board, 0, sizeof(state.board));
    state.current = I_BLOCK;
    state.rotation = 0;
    state.x = 2; state.y = 2;
    check_render(4);
    state.board[15][3] = 1;
    assert(get_landing_y() == 14);
    check_render(4);
    memset(state.board, 0, sizeof(state.board));
    state.rotation = 1;
    state.x = 0; state.y = 14;
    check_render(1); /* Three cells overlap the current piece. */
    state.y = 15;
    check_render(0); /* Already resting on the floor. */

    state.rotation = 0;
    state.x = 2; state.y = 2;
    int landing = get_landing_y();
    long expected_score = state.score + (landing - state.y) * 2;
    drop_piece();
    assert(state.score == expected_score);
    for (int col = 0; col < 8; col++) {
        assert(state.board[landing][col] == (col >= 2 && col < 6));
    }
    printf("PASS: %d landing/immutability cases, 4 render cases, hard-drop agreement\n", count);
    return 0;
}
