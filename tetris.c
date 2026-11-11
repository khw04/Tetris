#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <time.h>

/* 블록 모양 */
#define I_BLOCK 0
#define   T_BLOCK 1
#define S_BLOCK 2
#define Z_BLOCK 3
#define L_BLOCK 4
#define J_BLOCK 5
#define O_BLOCK 6

/* 게임 시작, 게임 종료 */
#define GAME_START 0
#define GAME_END 1

enum { BOARD_ROWS = 19, BOARD_COLS = 8, PIECE_TYPES = 7 };

static const char i_block[4][4][4] = {
    {  // 첫 번째 회전 상태
        {1, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} },
    {  // 두 번째 회전 상태
        {0, 0, 0, 1}, {0, 0, 0, 1}, {0, 0, 0, 1}, {0, 0, 0, 1} },
    {  // 세 번째 회전 상태
        {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {1, 1, 1, 1} },
    {  // 네 번째 회전 상태
        {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0} }
};

static const char t_block[4][4][4] =
   {
         {{1, 0, 0, 0},   {1, 1, 0, 0},   {1, 0, 0, 0},   {0, 0, 0, 0}},
         {{1, 1, 1, 0},   {0, 1, 0, 0},   {0, 0, 0, 0},   {0, 0, 0, 0}},
         {{0, 0, 1, 0},   {0, 1, 1, 0},   {0, 0, 1, 0},   {0, 0, 0, 0}},
         {{0, 0, 0, 0},   {0, 1, 0, 0},   {1, 1, 1, 0},   {0, 0, 0, 0}}
   };

static const char s_block[4][4][4] =
   {
         {{1, 0, 0, 0},   {1, 1, 0, 0},   {0, 1, 0, 0},   {0, 0, 0, 0}},
         {{0, 1, 1, 0},   {1, 1, 0, 0},   {0, 0, 0, 0},   {0, 0, 0, 0}},
         {{0, 1, 0, 0},   {0, 1, 1, 0},   {0, 0, 1, 0},   {0, 0, 0, 0}},
         {{0, 0, 0, 0},   {0, 1, 1, 0},   {1, 1, 0, 0},   {0, 0, 0, 0}}
   };

static const char z_block[4][4][4] =
   {
         {{0, 1, 0, 0},   {1, 1, 0, 0},   {1, 0, 0, 0},   {0, 0, 0, 0}},
         {{1, 1, 0, 0},   {0, 1, 1, 0},   {0, 0, 0, 0},   {0, 0, 0, 0}},
         {{0, 0, 1, 0},   {0, 1, 1, 0},   {0, 1, 0, 0},   {0, 0, 0, 0}},
         {{0, 0, 0, 0},   {1, 1, 0, 0},   {0, 1, 1, 0},   {0, 0, 0, 0}}
   };

static const char l_block[4][4][4] =
   {
         {{1, 0, 0, 0},   {1, 0, 0, 0},   {1, 1, 0, 0},   {0, 0, 0, 0}},
         {{1, 1, 1, 0},   {1, 0, 0, 0},   {0, 0, 0, 0},   {0, 0, 0, 0}},
         {{0, 1, 1, 0},   {0, 0, 1, 0},   {0, 0, 1, 0},   {0, 0, 0, 0}},
         {{0, 0, 0, 0},   {0, 0, 1, 0},   {1, 1, 1, 0},   {0, 0, 0, 0}}
   };

static const char j_block[4][4][4] =
   {
         {{0, 1, 0, 0},{0, 1, 0, 0},{1, 1, 0, 0},{0, 0, 0, 0}},
        {{1, 0, 0, 0},{1, 1, 1, 0},{0, 0, 0, 0},{0, 0, 0, 0}},
        {{1, 1, 0, 0},{1, 0, 0, 0},{1, 0, 0, 0},{0, 0, 0, 0}},
        {{1, 1, 1, 0},{0, 0, 1, 0},{0, 0, 0, 0},{0, 0, 0, 0}}
   };

static const char o_block[4][4][4] =
   {
         {{1, 1, 0, 0},   {1, 1, 0, 0},   {0, 0, 0, 0},   {0, 0, 0, 0}},
         {{1, 1, 0, 0},   {1, 1, 0, 0},   {0, 0, 0, 0},   {0, 0, 0, 0}},
         {{1, 1, 0, 0},   {1, 1, 0, 0},   {0, 0, 0, 0},   {0, 0, 0, 0}},
         {{1, 1, 0, 0},   {1, 1, 0, 0},   {0, 0, 0, 0},   {0, 0, 0, 0}}
   };

typedef struct {
    int pieces[PIECE_TYPES];
    int index;
} PieceBag;

typedef struct {
    unsigned char board[BOARD_ROWS][BOARD_COLS];
    int current, next, rotation, x, y, status, paused, lines;
    long score;
    int last_lines;
    long last_points;
    long long notice_until;
    unsigned int generation;
    PieceBag bag;
} GameState;

struct result {
    char name[30];
    long score;
    int year, month, day, hour, minute;
    size_t rank, order;
};

typedef struct {
    struct result *items;
    size_t count, capacity, skipped;
} RecordStore;

static GameState state;
static RecordStore records;
static struct {
    struct termios original;
    int active;
} terminal;
static volatile sig_atomic_t shutdown_signal;

enum { LINE_END = 0, LINE_OK = 1, LINE_INVALID = -1 };

void reset_termios(void);

static void cleanup(void) {
    reset_termios();
    free(records.items);
    memset(&records, 0, sizeof(records));
}

static void request_shutdown(int number) {
    shutdown_signal = number; /* No stdio or terminal operations in a handler. */
}

static int install_shutdown_handlers(void) {
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = request_shutdown;
    sigemptyset(&action.sa_mask);
    const int signals[] = {SIGINT, SIGTERM, SIGHUP};
    for (size_t i = 0; i < sizeof(signals) / sizeof(signals[0]); i++) {
        if (sigaction(signals[i], &action, NULL) != 0) {
            perror("종료 신호 설정 실패"); return 0;
        }
    }
    return 1;
}

/* Consume the entire line even if it is oversized or contains a NUL.
   This keeps the next prompt synchronized with input after an error. */
static int read_line(FILE *stream, char *buffer, size_t capacity) {
    size_t length = 0;
    int invalid = 0, received = 0;
    while (!shutdown_signal) {
        int ch = fgetc(stream);
        if (ch == EOF) {
            if (ferror(stream)) {
                if (!shutdown_signal) perror("입력 읽기 실패");
                return LINE_END;
            }
            if (!received) return LINE_END;
            break;
        }
        received = 1;
        if (ch == '\n') break;
        if (ch == '\0') invalid = 1;
        if (length + 1 < capacity) buffer[length++] = (char)ch;
        else invalid = 1;
    }
    if (shutdown_signal) return LINE_END;
    if (length && buffer[length - 1] == '\r') length--;
    buffer[length] = '\0';
    return invalid ? LINE_INVALID : LINE_OK;
}

static int parse_number(const char *text, long minimum, long maximum, long *value) {
    char *end;
    errno = 0;
    long parsed = strtol(text, &end, 10);
    if (end == text || errno == ERANGE) return 0;
    while (isspace((unsigned char)*end)) end++;
    if (*end || parsed < minimum || parsed > maximum) return 0;
    *value = parsed;
    return 1;
}

static int parse_choice(const char *text, int minimum, int maximum, int *choice) {
    long value;
    if (!parse_number(text, minimum, maximum, &value)) return 0;
    *choice = (int)value;
    return 1;
}

static int valid_name(const char *name) {
    size_t length = strlen(name);
    if (!length || length >= sizeof(((struct result *)0)->name)) return 0;
    int has_text = 0;
    for (size_t i = 0; i < length; i++) {
        unsigned char ch = (unsigned char)name[i];
        if (ch < 32 || ch == 127 || ch == ',') return 0;
        if (!isspace(ch)) has_text = 1;
    }
    return has_text;
}

static int take_from_bag(void) {
    if (state.bag.index >= PIECE_TYPES) {
        for (int i = 0; i < PIECE_TYPES; i++) state.bag.pieces[i] = i;
        for (int i = PIECE_TYPES - 1; i > 0; i--) {
            int j = rand() % (i + 1);
            int swap = state.bag.pieces[i];
            state.bag.pieces[i] = state.bag.pieces[j];
            state.bag.pieces[j] = swap;
        }
        state.bag.index = 0;
    }
    return state.bag.pieces[state.bag.index++];
}


/* Two spaces per cell: no glyph-dependent size or gaps. */
#define UI_BORDER "\033[48;5;60m  \033[0m"
#define UI_GHOST "\033[45m  \033[0m"
static const char *piece_colors[7] = {
    "\033[106m", "\033[105m", "\033[102m", "\033[101m",
    "\033[48;5;208m", "\033[104m", "\033[103m"
};

static long long now_ms(void) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (long long)now.tv_sec * 1000 + now.tv_nsec / 1000000;
}

static int get_level(void) {
    return state.lines / 10 + 1;
}

static const char (*get_block(int number))[4][4] {
    static const char (*const shapes[PIECE_TYPES])[4][4] = {
        i_block, t_block, s_block, z_block, l_block, j_block, o_block
    };
    return shapes[number >= 0 && number < PIECE_TYPES ? number : I_BLOCK];
}

static void draw_piece_cell(int number) {
    if (number < 0 || number > 6) number = I_BLOCK;
    printf("%s  \033[0m", piece_colors[number]);
}


/* ---------함수 프로토타입----------- */
int display_menu(void); // 메뉴 표시
int game_start(void); // 게임 시작: 초기화 및 메인 루프 호출
void init_game(void); // 게임 상태 초기화
int init_termios(void); // 터미널 입력 설정
void reset_termios(void); // 원래 터미널 설정 복원
int kbhit(void); // 키 입력 대기 없이 검사
int getch_noblock(void); // 논블록킹 단일 문자 읽기
void spawn_piece(void); // 다음 블록 생성
int can_move(int dx, int dy, int rotation); // 움직임, 회전가능 여부 확인
int get_landing_y(void); // 현재 블록의 착지 위치 계산 (게임 상태 변경 없음)
void rotate_piece(void); // 블록 회전
void move_piece(int dx, int dy); // 블록 이동
void drop_piece(void); // 블록 즉시 낙하
void lock_piece(void); // 블록 고정
void clear_lines(void); // 완성 줄 제거 & 위 블록들 내리기
void draw_board(void); // 보드 출력 (ANSI 이스케이프 이용)
void input_handler(void); // 사용자 입력 처리
void game_loop(void); // 메인 게임 루프
void search_result(void); // 기록 검색
void print_result(void); // 기록 출력
int save_game_result(void); // 결과 저장
size_t read_results_from_file(const char* filename); // 파일에서 결과 읽기
void sort_results_by_score(void); // 점수 내림차순으로 sort
void check_terminal_size(void); // 터미널 크기 확인
void draw_next_block(int row); // 사이드 패널의 미리보기 한 행

int main(void) {
    setvbuf(stdin, NULL, _IONBF, 0);
    if (!install_shutdown_handlers()) return 1;
    atexit(cleanup);
    srand((unsigned)time(NULL));
    while (!shutdown_signal) {
        check_terminal_size();
        int menu = display_menu();
        if (!menu || menu == 4) break;
        if (menu == 1) {
            if (!game_start()) break;
        } else {
            if (menu == 2) search_result();
            else print_result();
            if (!shutdown_signal) {
                printf("\n계속하려면 Enter키를 누르세요...");
                fflush(stdout);
                char line[64];
                if (read_line(stdin, line, sizeof(line)) == LINE_END) break;
            }
        }
    }
    return shutdown_signal ? 128 + shutdown_signal : 0;
}

void check_terminal_size(void) {
    struct winsize size = {0};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) != 0) {
        fprintf(stderr, "터미널 크기를 읽지 못했습니다. 터미널에서 실행하세요.\n");
        exit(1);
    }
    if (size.ws_row < 30 || size.ws_col < 80) {
        fprintf(stderr, "터미널을 최소 80x30으로 늘려주세요. 현재: %dx%d\n",
                size.ws_col, size.ws_row);
        exit(1);
    }
}

int display_menu(void)
{
   int menu = 0;

   while(1)
   {
      printf("\033[2J\033[H");
      printf("\n\n\t\t\t\tText Tetris");
      printf("\n\t\t\t============================");
      printf("\n\t\t\t\tGAME MENU\t\n");
      printf("\n\t\t\t============================");
      printf("\n\t\t\t   1) Game Start");
      printf("\n\t\t\t   2) Search history");
      printf("\n\t\t\t   3) Record Output");
      printf("\n\t\t\t   4) QUIT");
      printf("\n\t\t\t============================");
      printf("\n\t\t\t\t\t SELECT : ");
      fflush(stdout);

      char line[64];
      int status = read_line(stdin, line, sizeof(line));
      if (status == LINE_END) return 0;
      if (status == LINE_OK && parse_choice(line, 1, 4, &menu)) return menu;
      printf("1~4 중 하나를 입력하세요.\n");

   }
   return 0;
}

int game_start(void) {
    while (!shutdown_signal) {
        if (init_termios() != 0) {
            printf("터미널 설정 실패. 게임을 시작할 수 없습니다.\n");
            return GAME_END;
        }
        init_game();
        game_loop();
        reset_termios();
        if (shutdown_signal) return 0;
        printf("\033[0m\033[2J\033[H\n  \033[1;96mSESSION COMPLETE\033[0m\n");
        printf("  ----------------------------\n");
        printf("  최종 점수   %ld\n  제거한 줄   %d\n  도달 레벨   %d\n",
               state.score, state.lines, get_level());
        int saved = 0;
        for (;;) {
            printf("\n  1) 기록 저장%s  2) 다시 시작  3) 메뉴\n  선택: ",
                   saved ? " (완료)" : "");
            fflush(stdout);
            char choice[64];
            int status = read_line(stdin, choice, sizeof(choice)), selection;
            if (status == LINE_END) return 0;
            if (status != LINE_OK || !parse_choice(choice, 1, 3, &selection)) continue;
            if (selection == 1 && !saved) {
                saved = save_game_result();
            } else if (selection == 2) {
                break;
            } else if (selection == 3) {
                return GAME_END;
            }
        }
    }
    return 0;
}
void init_game(void) {
    memset(&state, 0, sizeof(state));
    state.bag.index = PIECE_TYPES;
    state.next = take_from_bag();
    state.status = GAME_START;
    spawn_piece();
    printf("\033[2J\033[H");
    draw_board();
}

int init_termios(void) {
    struct termios settings;
    if (tcgetattr(STDIN_FILENO, &terminal.original) != 0) {
        perror("터미널 설정 읽기 실패"); return -1;
    }
    settings = terminal.original;
    settings.c_lflag &= ~(ICANON | ECHO);
    settings.c_cc[VMIN] = 0;
    settings.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &settings) != 0) {
        perror("터미널 설정 변경 실패"); return -1;
    }
    terminal.active = 1;
    printf("\033[?25l");
    fflush(stdout);
    return 0;
}

void reset_termios(void) {
    if (!terminal.active) return;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &terminal.original) != 0) {
        perror("터미널 복구 실패");
    } else {
        terminal.active = 0;
    }
    printf("\033[0m\033[?25h");
    fflush(stdout);
}

int kbhit(void) {
    int available = 0;
    if (ioctl(STDIN_FILENO, FIONREAD, &available) != 0) {
        if (errno != EINTR) shutdown_signal = SIGHUP;
        return 0;
    }
    return available;
}

int getch_noblock(void) {
    char ch;

    if (kbhit()) {
        if (read(STDIN_FILENO, &ch, 1) == 1) {
            return (int)ch;
        }
    }

    return -1;
}

void spawn_piece(void) {
    state.generation++;
    state.current = state.next;
    state.next = take_from_bag();
    state.rotation = 0;
    state.x = 3;
    state.y = 0;

    if (!can_move(0, 0, 0)) {
        state.status = GAME_END;
    }
}

int can_move(int dx, int dy, int rotation) {
    if (state.current < 0 || state.current >= PIECE_TYPES) return 0;
    int new_state = ((state.rotation + rotation) % 4 + 4) % 4;
    const char (*cells)[4][4] = get_block(state.current);
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            if (!cells[new_state][row][col]) continue;
            int bx = state.x + dx + col, by = state.y + dy + row;
            if (bx < 0 || bx >= BOARD_COLS || by < 0 || by >= BOARD_ROWS ||
                state.board[by][bx]) return 0;
        }
    }
    return 1;
}

int get_landing_y(void) {
    int landing_y = state.y;

    if (!can_move(0, 0, 0)) {
        return state.y;
    }

    while (can_move(0, landing_y - state.y + 1, 0)) {
        landing_y++;
    }

    return landing_y;
}

void move_piece(int dx, int dy) {
    if (can_move(dx, dy, 0)) {
        state.x += dx;
        state.y += dy;
    }
}

void rotate_piece(void) {
    if (state.current == O_BLOCK) return;
    /* Simple collision-tested kicks for the existing rotation matrices.
       These are custom kicks, not the SRS rotation system. */
    static const int kicks[][2] = {
        {0, 0}, {-1, 0}, {1, 0}, {-2, 0}, {2, 0}, {-3, 0}, {3, 0},
        {0, -1}, {-1, -1}, {1, -1}, {0, -2}, {0, -3}
    };
    for (size_t i = 0; i < sizeof(kicks) / sizeof(kicks[0]); i++) {
        if (can_move(kicks[i][0], kicks[i][1], 1)) {
            state.x += kicks[i][0]; state.y += kicks[i][1];
            state.rotation = (state.rotation + 1) % 4;
            return;
        }
    }
}

void drop_piece(void) {
    int landing_y = get_landing_y();
    state.score += (landing_y - state.y) * 2;
    state.y = landing_y;

    lock_piece();
}

void lock_piece(void) {
    if (!can_move(0, 0, 0)) { state.status = GAME_END; return; }
    const char (*cells)[4][4] = get_block(state.current);
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            if (cells[state.rotation][row][col])
                state.board[state.y + row][state.x + col] = state.current + 1;
        }
    }
    clear_lines();
    spawn_piece();
}

void clear_lines(void) {
    int lines_cleared = 0;

    for (int row = 18; row >= 0; row--) {
        int is_full_line = 1;

        for (int col = 0; col < 8; col++) {
            if (state.board[row][col] == 0) {
                is_full_line = 0;
                break;
            }
        }

        if (is_full_line) {
            for (int move_y = row; move_y > 0; move_y--) {
                for (int col = 0; col < 8; col++) {
                    state.board[move_y][col] = state.board[move_y - 1][col];
                }
            }

            for (int col = 0; col < 8; col++) {
                state.board[0][col] = 0;
            }

            lines_cleared++;
            row++;
        }
    }

    if (lines_cleared > 0) {
        long before = state.score;
        state.lines += lines_cleared;
        switch (lines_cleared) {
            case 1: state.score += 100; break;
            case 2: state.score += 300; break;
            case 3: state.score += 500; break;
            case 4: state.score += 800; break;
            default: state.score += lines_cleared * 100; break;
        }
        state.last_lines = lines_cleared;
        state.last_points = state.score - before;
        state.notice_until = now_ms() + 1800;
    }
}

static void draw_side_panel(int row) {
    printf("    ");
    switch (row) {
        case 0: printf("\033[1;96mSCORE\033[0m"); break;
        case 1: printf("\033[1;97m%ld\033[0m", state.score); break;
        case 3: printf("LEVEL  %02d", get_level()); break;
        case 4: printf("LINES  %02d", state.lines); break;
        case 6: printf("\033[1;96mNEXT\033[0m"); break;
        case 7: case 8: case 9: case 10:
            draw_next_block(row - 7); break;
        case 12: printf("\033[1;96mSTATUS\033[0m"); break;
        case 13: printf(state.paused ? "\033[1;93mPAUSED\033[0m" : "PLAYING"); break;
        case 15: printf(UI_GHOST " 착지 미리보기"); break;
        case 18:
            if (now_ms() < state.notice_until)
                printf("\033[1;93m%d줄 제거! +%ld\033[0m",
                       state.last_lines, state.last_points);
            break;
        case 19:
            if (state.paused) printf("SPACE를 눌러 계속");
            break;
    }
}

void draw_board(void) {
    int ghost_y = get_landing_y();
    const char (*current)[4][4] = get_block(state.current);
    printf("\033[H\033[0m");
    printf("  \033[1;96mT E T R I S\033[0m\033[K\n");
    printf("  색을 쌓고, 줄을 지우세요.\033[K\n\033[K\n");
    for (int row = -1; row <= 19; row++) {
        printf("  " UI_BORDER);
        for (int col = 0; col < 8; col++) {
            if (row == -1 || row == 19) {
                printf(UI_BORDER);
            } else if (!state.paused && col >= state.x && col < state.x + 4 &&
                       row >= state.y && row < state.y + 4 &&
                       current[state.rotation][row - state.y][col - state.x]) {
                draw_piece_cell(state.current);
            } else if (!state.paused && state.board[row][col]) {
                draw_piece_cell(state.board[row][col] - 1);
            } else if (!state.paused && col >= state.x && col < state.x + 4 &&
                       row >= ghost_y && row < ghost_y + 4 &&
                       current[state.rotation][row - ghost_y][col - state.x]) {
                printf(UI_GHOST);
            } else {
                printf("  ");
            }
        }
        printf(UI_BORDER);
        draw_side_panel(row + 1);
        printf("\033[K\n");
    }
    printf("\033[K\n  J/L 이동  I 회전  K 내리기  A 즉시낙하\033[K\n");
    printf("  SPACE 일시정지/계속  P 종료\033[K\n\033[J");
    if (state.paused) printf("\033[14;8H\033[1;93mPAUSED\033[0m");
    fflush(stdout);
}

static void handle_key(int key) {
    if (key == 4) { shutdown_signal = SIGHUP; return; }
    if (key == 'p' || key == 'P') {
        state.status = GAME_END;
        return;
    }
    if (key == ' ') {
        state.paused = !state.paused;
        return;
    }
    if (state.paused) return;
    switch (key) {
        case 'j': case 'J': move_piece(-1, 0); break;
        case 'l': case 'L': move_piece(1, 0); break;
        case 'k': case 'K': move_piece(0, 1); break;
        case 'i': case 'I': rotate_piece(); break;
        case 'a': case 'A': drop_piece(); break;
    }
}

void input_handler(void) {
    handle_key(getch_noblock());
}

void game_loop(void) {
    long long last_fall = now_ms();
    while (state.status == GAME_START && !shutdown_signal) {
        int was_paused = state.paused;
        unsigned int previous_generation = state.generation;
        input_handler();
        if (state.status != GAME_START || shutdown_signal) break;
        long long now = now_ms();
        long fall_interval = 500 - (long)(get_level() - 1) * 40;
        if (fall_interval < 100) fall_interval = 100;
        if (state.paused || was_paused || previous_generation != state.generation) {
            last_fall = now;
        } else if (now - last_fall >= fall_interval) {
            if (can_move(0, 1, 0)) state.y++;
            else lock_piece();
            last_fall = now;
        }
        draw_board();
        usleep(33000); /* About 30 frames per second. */
    }
}

static int parse_record(char *line, struct result *record) {
    char *fields[8];
    fields[0] = line;
    for (int i = 1; i < 8; i++) {
        char *comma = strchr(fields[i - 1], ',');
        if (!comma) return 0;
        *comma = '\0';
        fields[i] = comma + 1;
    }
    if (strchr(fields[7], ',') || !valid_name(fields[0])) return 0;
    const long minimum[7] = {0, 1900, 1, 1, 0, 0, 0};
    const long maximum[7] = {LONG_MAX, 9999, 12, 31, 23, 59, LONG_MAX};
    long values[7];
    for (int i = 0; i < 7; i++)
        if (!parse_number(fields[i + 1], minimum[i], maximum[i], &values[i])) return 0;
    static const int month_days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int year = (int)values[1], month = (int)values[2];
    int leap = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
    int days = month_days[month - 1] + (month == 2 && leap);
    if (values[3] > days) return 0;
    memset(record, 0, sizeof(*record));
    strcpy(record->name, fields[0]);
    record->score = values[0]; record->year = year; record->month = month;
    record->day = (int)values[3]; record->hour = (int)values[4];
    record->minute = (int)values[5]; record->rank = (size_t)values[6];
    return 1;
}

static int append_record(RecordStore *store, struct result record) {
    if (store->count == store->capacity) {
        size_t capacity = store->capacity ? store->capacity * 2 : 64;
        if (capacity < store->capacity || capacity > SIZE_MAX / sizeof(record)) return 0;
        struct result *items = realloc(store->items, capacity * sizeof(record));
        if (!items) return 0;
        store->items = items;
        store->capacity = capacity;
    }
    record.order = store->count;
    store->items[store->count++] = record;
    return 1;
}

int save_game_result(void) {
    struct result record = {0};
    printf("\n게임 종료! 이름을 입력하세요: ");
    fflush(stdout);
    if (read_line(stdin, record.name, sizeof(record.name)) != LINE_OK ||
        !valid_name(record.name)) {
        if (!shutdown_signal) printf("이름은 제어문자·쉼표 없이 1~29바이트로 입력하세요.\n");
        return 0;
    }
    time_t now = time(NULL);
    struct tm *date = localtime(&now);
    if (!date) { perror("현재 시각 읽기 실패"); return 0; }
    record.score = state.score;
    record.year = date->tm_year + 1900; record.month = date->tm_mon + 1;
    record.day = date->tm_mday; record.hour = date->tm_hour; record.minute = date->tm_min;
    FILE *file = fopen("result.txt", "a+");
    if (!file) { perror("결과 파일 열기 실패"); return 0; }
    /* Keep a valid new record separate if an existing file lacks its last newline. */
    int ok = 1;
    if (fseek(file, 0, SEEK_END) != 0) ok = 0;
    long end = ok ? ftell(file) : -1;
    if (end < 0) ok = 0;
    if (ok && end > 0) {
        if (fseek(file, -1, SEEK_END) != 0) ok = 0;
        int last = ok ? fgetc(file) : EOF;
        if (last == EOF || fseek(file, 0, SEEK_END) != 0) ok = 0;
        if (ok && last != '\n' && fputc('\n', file) == EOF) ok = 0;
    }
    if (ok && fprintf(file, "%s,%ld,%d,%d,%d,%d,%d,0\n", record.name, record.score,
                      record.year, record.month, record.day, record.hour, record.minute) < 0) ok = 0;
    if (fclose(file) != 0) ok = 0;
    if (!ok) { fprintf(stderr, "결과 저장에 실패했습니다.\n"); return 0; }
    printf("결과가 저장되었습니다!\n최종 점수: %ld점\n", record.score);
    return 1;
}

size_t read_results_from_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        if (errno == ENOENT) printf("저장된 기록이 없습니다.\n");
        else perror("결과 파일 열기 실패");
        return 0;
    }
    RecordStore loaded = {0};
    char line[512];
    int status;
    while ((status = read_line(file, line, sizeof(line))) != LINE_END) {
        struct result record;
        if (status != LINE_OK || !parse_record(line, &record)) {
            loaded.skipped++;
            continue;
        }
        if (!append_record(&loaded, record)) {
            free(loaded.items); fclose(file);
            fprintf(stderr, "기록을 불러올 메모리가 부족합니다.\n");
            return 0;
        }
    }
    int failed = ferror(file);
    if (fclose(file) != 0) failed = 1;
    if (failed || shutdown_signal) { free(loaded.items); return 0; }
    free(records.items);
    records = loaded;
    printf("총 %zu개의 기록을 읽었습니다.\n", records.count);
    if (records.skipped) printf("형식이 잘못된 기록 %zu개를 제외했습니다.\n", records.skipped);
    return records.count;
}

static int compare_results(const void *left, const void *right) {
    const struct result *a = left, *b = right;
    if (a->score != b->score) return a->score > b->score ? -1 : 1;
    return (a->order > b->order) - (a->order < b->order);
}

void sort_results_by_score(void) {
    if (records.count > 1)
        qsort(records.items, records.count, sizeof(*records.items), compare_results);
    for (size_t i = 0; i < records.count; i++) records.items[i].rank = i + 1;
}

static void print_record(const struct result *record) {
    printf("%zu위. %s : %ldpoint (%d-%02d-%02d)\n", record->rank, record->name,
           record->score, record->year, record->month, record->day);
}

void search_result(void) {
    if (!read_results_from_file("result.txt")) return;
    sort_results_by_score();
    printf("검색할 이름을 입력하세요: ");
    fflush(stdout);
    char name[30];
    if (read_line(stdin, name, sizeof(name)) != LINE_OK || !valid_name(name)) {
        if (!shutdown_signal) printf("이름은 제어문자·쉼표 없이 1~29바이트로 입력하세요.\n");
        return;
    }
    int found = 0;
    printf("\n=== 검색 결과 ===\n\n");
    for (size_t i = 0; i < records.count && !shutdown_signal; i++) {
        if (strcmp(records.items[i].name, name) == 0) {
            print_record(&records.items[i]); found = 1;
        }
    }
    if (!found && !shutdown_signal) printf("'%s' 이름의 기록을 찾을 수 없습니다.\n", name);
}

void print_result(void) {
    if (!read_results_from_file("result.txt")) return;
    sort_results_by_score();
    printf("\n=== 게임 기록 순위 ===\n\n");
    for (size_t i = 0; i < records.count && !shutdown_signal; i++) print_record(&records.items[i]);
    printf("\n=======================\n");
}


void draw_next_block(int row) {
    const char (*next)[4][4] = get_block(state.next);
    for (int col = 0; col < 4; col++) {
        if (next[0][row][col]) draw_piece_cell(state.next);
        else printf("  ");
    }
}
