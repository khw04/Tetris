#define main tetris_application_main
#include "../tetris.c"
#undef main
#include <assert.h>

static void test_lines(void) {
    FILE *input = tmpfile();
    assert(input);
    fputs("12345678901234567890\n2\n", input);
    fputc('a', input); fputc(0, input); fputs("b\nfinal", input);
    rewind(input);
    char buffer[10];
    assert(read_line(input, buffer, sizeof(buffer)) == LINE_INVALID);
    assert(read_line(input, buffer, sizeof(buffer)) == LINE_OK && !strcmp(buffer, "2"));
    assert(read_line(input, buffer, sizeof(buffer)) == LINE_INVALID);
    assert(read_line(input, buffer, sizeof(buffer)) == LINE_OK && !strcmp(buffer, "final"));
    assert(read_line(input, buffer, sizeof(buffer)) == LINE_END);
    fclose(input);
    int choice;
    assert(parse_choice(" 2 ", 1, 4, &choice) && choice == 2);
    assert(!parse_choice("2junk", 1, 4, &choice));
    assert(!parse_choice("999999999999999999999999999", 1, 4, &choice));
    assert(!parse_choice("", 1, 4, &choice));
    assert(!valid_name("bad,name") && !valid_name("bad\033name") && !valid_name("   "));
    assert(valid_name("한글 이름"));
}

static void test_bag(void) {
    srand(42);
    state.bag.index = PIECE_TYPES;
    for (int bag = 0; bag < 1000; bag++) {
        unsigned mask = 0;
        for (int i = 0; i < PIECE_TYPES; i++) {
            int piece = take_from_bag();
            assert(piece >= 0 && piece < PIECE_TYPES);
            assert(!(mask & (1u << piece)));
            mask |= 1u << piece;
        }
        assert(mask == 127);
    }
    fflush(stdout);
    int output = dup(STDOUT_FILENO);
    FILE *null = fopen("/dev/null", "w");
    assert(output >= 0 && null);
    assert(dup2(fileno(null), STDOUT_FILENO) >= 0);
    init_game();
    assert(state.bag.index == 2);
    assert(state.current == state.bag.pieces[0] && state.next == state.bag.pieces[1]);
    fflush(stdout);
    assert(dup2(output, STDOUT_FILENO) >= 0);
    close(output); fclose(null);
}

static void test_kicks(void) {
    memset(&state, 0, sizeof(state));
    state.current = I_BLOCK; state.rotation = 3; state.x = 7;
    assert(can_move(0, 0, 0) && !can_move(0, 0, 1));
    rotate_piece();
    assert(state.rotation == 0 && state.x == 4 && can_move(0, 0, 0));
    state.rotation = 1; state.x = -3;
    assert(can_move(0, 0, 0));
    rotate_piece();
    assert(state.rotation == 2 && state.x == 0 && can_move(0, 0, 0));
    state.rotation = 0; state.x = 3; state.y = 18;
    rotate_piece();
    assert(state.rotation == 1 && state.y == 15 && can_move(0, 0, 0));

    memset(state.board, 1, sizeof(state.board));
    state.current = T_BLOCK; state.rotation = 0; state.x = 3; state.y = 5;
    const char (*cells)[4][4] = get_block(state.current);
    for (int row = 0; row < 4; row++)
        for (int col = 0; col < 4; col++)
            if (cells[0][row][col]) state.board[state.y + row][state.x + col] = 0;
    assert(can_move(0, 0, 0));
    rotate_piece();
    assert(state.rotation == 0 && state.x == 3 && state.y == 5);
    memset(state.board, 0, sizeof(state.board));
    state.current = O_BLOCK;
    rotate_piece();
    assert(state.rotation == 0 && state.x == 3 && state.y == 5);
}

static void test_records(void) {
    struct result record;
    char good[] = "한글 이름,123,2024,2,29,23,59,0";
    assert(parse_record(good, &record));
    const char *bad[] = {
        "name,1,2023,2,29,0,0,0", "name,-1,2026,1,1,0,0,0",
        "name,1x,2026,1,1,0,0,0", "name,1,2026,13,1,0,0,0",
        "name,1,2026,1,1,24,0,0", "name,1,2026,1,1,0,0,0,extra",
        "name,999999999999999999999999999,2026,1,1,0,0,0",
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,1,2026,1,1,0,0,0", "broken", ""
    };
    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
        char line[256]; strcpy(line, bad[i]);
        assert(!parse_record(line, &record));
    }
    char directory[] = "/tmp/tetris-records-XXXXXX";
    assert(mkdtemp(directory));
    char filename[256]; snprintf(filename, sizeof(filename), "%s/result.txt", directory);
    FILE *file = fopen(filename, "w"); assert(file);
    for (int i = 0; i < 1205; i++)
        fprintf(file, "player%d,%d,2026,11,11,12,0,0\n", i, i);
    fputs("broken\n", file);
    for (int i = 0; i < 2000; i++) fputc('a', file);
    fputs("\nnewest,9999,2026,11,11,12,0,0", file); /* Valid EOF without newline. */
    fclose(file);
    assert(read_results_from_file(filename) == 1206 && records.skipped == 2);
    sort_results_by_score();
    assert(!strcmp(records.items[0].name, "newest") && records.items[0].rank == 1);
    assert(records.items[1].score == 1204);
    assert(records.items[1205].score == 0);
    assert(read_results_from_file(filename) == 1206); /* Reload releases old storage. */
    unlink(filename); rmdir(directory);
    free(records.items); memset(&records, 0, sizeof(records));
}

int main(void) {
    test_lines(); test_bag(); test_kicks(); test_records();
    puts("PASS: bounded input/EOF, 1000 bags, wall/floor/blocked kicks, 1206 records");
    return 0;
}
