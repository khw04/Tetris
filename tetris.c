#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <time.h>

/* 타이머  */
#define CCHAR 0
#ifdef CTIME
#undef CTIME
#endif
#define CTIME 1

/* 왼쪽, 오른쪽, 아래, 회전  */
#define LEFT 0
#define RIGHT 1
#define DOWN 2
#define ROTATE 3

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

char i_block[4][4][4] = {
    {  // 첫 번째 회전 상태
        {1, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} },
    {  // 두 번째 회전 상태
        {0, 0, 0, 1}, {0, 0, 0, 1}, {0, 0, 0, 1}, {0, 0, 0, 1} },
    {  // 세 번째 회전 상태
        {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {1, 1, 1, 1} },
    {  // 네 번째 회전 상태
        {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0} }
};

char t_block[4][4][4] =
   {
         {{1, 0, 0, 0},   {1, 1, 0, 0},   {1, 0, 0, 0},   {0, 0, 0, 0}},
         {{1, 1, 1, 0},   {0, 1, 0, 0},   {0, 0, 0, 0},   {0, 0, 0, 0}},
         {{0, 0, 1, 0},   {0, 1, 1, 0},   {0, 0, 1, 0},   {0, 0, 0, 0}},
         {{0, 0, 0, 0},   {0, 1, 0, 0},   {1, 1, 1, 0},   {0, 0, 0, 0}}
   };

char s_block[4][4][4] =
   {
         {{1, 0, 0, 0},   {1, 1, 0, 0},   {0, 1, 0, 0},   {0, 0, 0, 0}},
         {{0, 1, 1, 0},   {1, 1, 0, 0},   {0, 0, 0, 0},   {0, 0, 0, 0}},
         {{0, 1, 0, 0},   {0, 1, 1, 0},   {0, 0, 1, 0},   {0, 0, 0, 0}},
         {{0, 0, 0, 0},   {0, 1, 1, 0},   {1, 1, 0, 0},   {0, 0, 0, 0}}
   };

char z_block[4][4][4] =
   {
         {{0, 1, 0, 0},   {1, 1, 0, 0},   {1, 0, 0, 0},   {0, 0, 0, 0}},
         {{1, 1, 0, 0},   {0, 1, 1, 0},   {0, 0, 0, 0},   {0, 0, 0, 0}},
         {{0, 0, 1, 0},   {0, 1, 1, 0},   {0, 1, 0, 0},   {0, 0, 0, 0}},
         {{0, 0, 0, 0},   {1, 1, 0, 0},   {0, 1, 1, 0},   {0, 0, 0, 0}}
   };

char l_block[4][4][4] =
   {
         {{1, 0, 0, 0},   {1, 0, 0, 0},   {1, 1, 0, 0},   {0, 0, 0, 0}},
         {{1, 1, 1, 0},   {1, 0, 0, 0},   {0, 0, 0, 0},   {0, 0, 0, 0}},
         {{0, 1, 1, 0},   {0, 0, 1, 0},   {0, 0, 1, 0},   {0, 0, 0, 0}},
         {{0, 0, 0, 0},   {0, 0, 1, 0},   {1, 1, 1, 0},   {0, 0, 0, 0}}
   };

char j_block[4][4][4] =
   {
         {{0, 1, 0, 0},{0, 1, 0, 0},{1, 1, 0, 0},{0, 0, 0, 0}},
        {{1, 0, 0, 0},{1, 1, 1, 0},{0, 0, 0, 0},{0, 0, 0, 0}},
        {{1, 1, 0, 0},{1, 0, 0, 0},{1, 0, 0, 0},{0, 0, 0, 0}},
        {{1, 1, 1, 0},{0, 0, 1, 0},{0, 0, 0, 0},{0, 0, 0, 0}}
   };

char o_block[4][4][4] =
   {
         {{1, 1, 0, 0},   {1, 1, 0, 0},   {0, 0, 0, 0},   {0, 0, 0, 0}},
         {{1, 1, 0, 0},   {1, 1, 0, 0},   {0, 0, 0, 0},   {0, 0, 0, 0}},
         {{1, 1, 0, 0},   {1, 1, 0, 0},   {0, 0, 0, 0},   {0, 0, 0, 0}},
         {{1, 1, 0, 0},   {1, 1, 0, 0},   {0, 0, 0, 0},   {0, 0, 0, 0}}
   };

char tetris_table[19][8];

static struct result
{
  char name[30];
  long point;
  int year;
  int month;
  int day;
  int hour;
  int min;
  int rank;
} temp_result;

// 원래 터미널 설정 저장용 전역 변수
struct termios old_termios;

// 결과를 저장할 구조체 배열 (전역 변수)
struct result game_results[100];
int result_count = 0;

int block_number = 0;  /*블록 번호*/
int next_block_number = 0; /*다음 블록 번호 */
int block_state = 0; /*블록 상태, 왼쪽, 오른쪽, 아래, 회전  */

int x = 3, y = 0; /*블록의 위치*/

int game = GAME_END; /*게임 시작, 게임 종료*/
int best_point = 0; /* 최고 점수*/

long point = 0; /* 현재 점수*/

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
void save_game_result(void); // 결과 저장
int read_results_from_file(const char* filename); // 파일에서 결과 읽기
void sort_results_by_score(void); // 점수 내림차순으로 sort
void check_terminal_size(void); // 터미널 크기 확인
void draw_next_block(void); // 다음 블록을 그려주는

int main(void)
{
   int menu = 1;

   while(menu)
   {  
      check_terminal_size();
      menu = display_menu();

      if(menu == 1)
      {
         game = GAME_START;
         menu = game_start();

      }
      else if(menu == 2)
      {
//         reset_termios();
         search_result();
         printf("\n계속하려면 Enter키를 누르세요...");
         getchar();
      }
      else if(menu == 3)
      {
//         reset_termios();
         print_result();
         printf("\n계속하려면 Enter키를 누르세요...");
         getchar();
      }
      else if(menu == 4)
      {
         exit(0);
      }
   }

   return 0;
}

void check_terminal_size(void) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    
    if (w.ws_row < 30 || w.ws_col < 80) {
        printf("터미널 크기를 늘려주세요! (최소 80x30)\n");
        printf("현재 크기: %dx%d\n", w.ws_col, w.ws_row);
        exit(1);
    }
    
    // test terminal size
    /*else{ 
        printf("현재 크기: %dx%d\n", w.ws_col, w.ws_row);
        exit(1);
    }*/
    
}

int display_menu(void)
{
   int menu = 0;

   while(1)
   {
      system("clear");
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

      if (scanf("%d", &menu) != 1) {
         while (getchar() != '\n');
         continue;
      }

      while (getchar() != '\n');

      if(menu < 1 || menu > 4)
      {
         continue;
      }
      else
      {
         return menu;
      }
   }
   return 0;
}

int game_start(void) {
    srand((unsigned)time(NULL));

    // check_terminal_size();

    if (init_termios() != 0) {
        printf("터미널 설정 실패. 게임을 시작할 수 없습니다.\n");
        return GAME_END;
    }

    init_game();
    spawn_piece();

    next_block_number = rand() % 7;
    if (next_block_number < 0 || next_block_number > 6) {
        next_block_number = 0;
    }

    game_loop();
    reset_termios();
    printf("\n게임 오버!\n");
    printf("최종 점수: %ld\n", point);
    save_game_result();
    return GAME_END;
}

void init_game(void) {
    for (int i = 0; i < 19; i++) {
        for (int j = 0; j < 8; j++) {
            tetris_table[i][j] = 0;
        }
    }

    point = 0;

    block_number = rand() % 7;
    if (block_number < 0 || block_number > 6) {
        block_number = 0;
    }

    next_block_number = rand() % 7;
    if (next_block_number < 0 || next_block_number > 6) {
        next_block_number = 0;
    }

    block_state = 0;
    x = 3;
    y = 0;

    if (x < 0 || x >= 8) x = 3;
    if (y < 0 || y >= 19) y = 0;

    game = GAME_START;

    printf("\033[2J\033[H");

    draw_board();
}

int init_termios(void) {
    struct termios new_termios;

    if (tcgetattr(0, &old_termios) != 0) {
        printf("터미널 설정 읽기 실패\n");
        return -1;
    }

    new_termios = old_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO);
    new_termios.c_cc[VMIN] = 0;
    new_termios.c_cc[VTIME] = 0;

    if (tcsetattr(0, TCSANOW, &new_termios) != 0) {
        printf("터미널 설정 변경 실패\n");
        return -1;
    }

    printf("\033[?25l");
    return 0;
}

void reset_termios(void) {
    if (tcsetattr(0, TCSANOW, &old_termios) != 0) {
        printf("터미널 복구 실패\n");
    }
    printf("\033[?25h");
}

int kbhit(void) {
    int k;
    ioctl(STDIN_FILENO, FIONREAD, &k);
    return k;
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
    block_number = next_block_number;
    next_block_number = rand() % 7;
    block_state = 0;
    x = 3;
    y = 0;

    if (!can_move(0, 0, 0)) {
        game = GAME_END;
    }
}

int can_move(int dx, int dy, int rotation) {
    int new_x = x + dx;
    int new_y = y + dy;
    int new_state = (block_state + rotation) % 4;

    char (*current_block)[4][4];

    switch(block_number) {
        case I_BLOCK: current_block = i_block; break;
        case T_BLOCK: current_block = t_block; break;
        case S_BLOCK: current_block = s_block; break;
        case Z_BLOCK: current_block = z_block; break;
        case L_BLOCK: current_block = l_block; break;
        case J_BLOCK: current_block = j_block; break;
        case O_BLOCK: current_block = o_block; break;
        default: return 0;
    }

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (current_block[new_state][i][j] == 1) {
                int board_x = new_x + j;
                int board_y = new_y + i;

                if (board_x < 0 || board_x >= 8) {
                    return 0;
                }
                if (board_y < 0 || board_y >= 19) {
                    return 0;
                }

                if (tetris_table[board_y][board_x] != 0) {
                    return 0;
                }
            }
        }
    }

    return 1;
}

void move_piece(int dx, int dy) {
    if (can_move(dx, dy, 0)) {
        x += dx;
        y += dy;
    }
}

void rotate_piece(void) {
    if (can_move(0, 0, 1)) {
        block_state = (block_state + 1) % 4;
    }
}

void drop_piece(void) {
    while (can_move(0, 1, 0)) {
        y++;
        point += 2;
    }

    lock_piece();
}

void lock_piece(void) {
    char (*current_block)[4][4];

    switch(block_number) {
        case I_BLOCK: current_block = i_block; break;
        case T_BLOCK: current_block = t_block; break;
        case S_BLOCK: current_block = s_block; break;
        case Z_BLOCK: current_block = z_block; break;
        case L_BLOCK: current_block = l_block; break;
        case J_BLOCK: current_block = j_block; break;
        case O_BLOCK: current_block = o_block; break;
        default: current_block = i_block; break;
    }

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (current_block[block_state][i][j] == 1) {
                int board_x = x + j;
                int board_y = y + i;

                if (board_x >= 0 && board_x < 8 && board_y >= 0 && board_y < 19) {
                    tetris_table[board_y][board_x] = 1;
                }
            }
        }
    }

    clear_lines();
    spawn_piece();
}

void clear_lines(void) {
    int lines_cleared = 0;

    for (int y = 18; y >= 0; y--) {
        int is_full_line = 1;

        for (int x = 0; x < 8; x++) {
            if (tetris_table[y][x] == 0) {
                is_full_line = 0;
                break;
            }
        }

        if (is_full_line) {
            for (int move_y = y; move_y > 0; move_y--) {
                for (int x = 0; x < 8; x++) {
                    tetris_table[move_y][x] = tetris_table[move_y - 1][x];
                }
            }

            for (int x = 0; x < 8; x++) {
                tetris_table[0][x] = 0;
            }

            lines_cleared++;
            y++;
        }
    }

    if (lines_cleared > 0) {
        switch (lines_cleared) {
            case 1: point += 100; break;
            case 2: point += 300; break;
            case 3: point += 500; break;
            case 4: point += 800; break;
            default: point += lines_cleared * 100; break;
        }
    }
}

void draw_board(void) {
    static int first_draw = 1;

    if (first_draw) {
        printf("\033[2J\033[H");
        first_draw = 0;
    } else {
        printf("\033[H");
    }

    printf("점수: %ld\t", point);
    printf("\n조작법: j(왼쪽) l(오른쪽) k(아래) i(회전) a(즉시낙하) p(종료)\n");
    
    draw_next_block();

#ifdef __linux__
    // 리눅스용 - 상단 테두리
    printf("□ ");
    for (int i = 0; i < 8; i++) {
        printf("□ ");
    }
    printf("□  \n");

    for (int board_y = 0; board_y < 19; board_y++) {
        printf("□ ");

        for (int board_x = 0; board_x < 8; board_x++) {
            int is_current_block = 0;

            char (*current_block)[4][4];
            switch(block_number) {
                case I_BLOCK: current_block = i_block; break;
                case T_BLOCK: current_block = t_block; break;
                case S_BLOCK: current_block = s_block; break;
                case Z_BLOCK: current_block = z_block; break;
                case L_BLOCK: current_block = l_block; break;
                case J_BLOCK: current_block = j_block; break;
                case O_BLOCK: current_block = o_block; break;
                default: current_block = i_block; break;
            }

            if (board_x >= x && board_x < x + 4 && board_y >= y && board_y < y + 4) {
                int block_x = board_x - x;
                int block_y = board_y - y;
                if (current_block[block_state][block_y][block_x] == 1) {
                    is_current_block = 1;
                }
            }

            if (is_current_block) {
                printf("■ ");
            } else if (tetris_table[board_y][board_x] != 0) {
                printf("■ ");
            } else {
                printf("  ");
            }
        }

        printf("□ \n");
    }

    // 하단 테두리
    printf("□ ");
    for (int i = 0; i < 8; i++) {
        printf("□ ");
    }
    printf("□  \n");

#else
    // macOS/Windows용 - 상단 테두리
    printf("⏹️ ");
    for (int i = 0; i < 8; i++) {
        printf("⏹️ ");
    }
    printf("⏹️ \n");

    for (int board_y = 0; board_y < 19; board_y++) {
        printf("⏹️ ");

        for (int board_x = 0; board_x < 8; board_x++) {
            int is_current_block = 0;

            char (*current_block)[4][4];
            switch(block_number) {
                case I_BLOCK: current_block = i_block; break;
                case T_BLOCK: current_block = t_block; break;
                case S_BLOCK: current_block = s_block; break;
                case Z_BLOCK: current_block = z_block; break;
                case L_BLOCK: current_block = l_block; break;
                case J_BLOCK: current_block = j_block; break;
                case O_BLOCK: current_block = o_block; break;
                default: current_block = i_block; break;
            }

            if (board_x >= x && board_x < x + 4 && board_y >= y && board_y < y + 4) {
                int block_x = board_x - x;
                int block_y = board_y - y;
                if (current_block[block_state][block_y][block_x] == 1) {
                    is_current_block = 1;
                }
            }

            if (is_current_block) {
                printf("⬜");
            } else if (tetris_table[board_y][board_x] != 0) {
                printf("⬜");
            } else {
                printf("  ");
            }
        }

        printf ("⏹️ \n");
    }

    // 하단 테두리
    printf("⏹️ ");
    for (int i = 0; i < 8; i++) {
        printf("⏹️ ");
    }
    printf("⏹️ \n");
#endif
}

void input_handler(void) {
    if (kbhit()) {
        int key = getch_noblock();

        switch (key) {
            case 'j':
            case 'J':
                move_piece(-1, 0);
                break;

            case 'l':
            case 'L':
                move_piece(1, 0);
                break;

            case 'k':
            case 'K':
                move_piece(0, 1);
                break;

            case 'i':
            case 'I':
                rotate_piece();
                break;

            case 'a':
            case 'A':
                drop_piece();
                break;

            case 'p':
            case 'P':
                game = GAME_END;
                break;

            default:
                break;
        }
    }
}

void game_loop(void) {
    struct timeval last_fall, current_time;
    long fall_interval = 500000;

    gettimeofday(&last_fall, NULL);

    while (game == GAME_START) {
        gettimeofday(&current_time, NULL);

        long elapsed = (current_time.tv_sec - last_fall.tv_sec) * 1000000 +
                      (current_time.tv_usec - last_fall.tv_usec);

        input_handler();

        if (elapsed >= fall_interval) {
            if (can_move(0, 1, 0)) {
                y++;
            } else {
                lock_piece();
            }

            gettimeofday(&last_fall, NULL);
        }

        draw_board();
        usleep(10000);
    }
}

void save_game_result(void) {
    FILE *fp;
    time_t now;
    struct tm *timeinfo;

    reset_termios();

    time(&now);
    timeinfo = localtime(&now);

    printf("\n게임 종료! 이름을 입력하세요: ");
    fflush(stdout);

    if (scanf("%29s", temp_result.name) != 1) {
        strcpy(temp_result.name, "Unknown");
    }

    temp_result.point = point;
    temp_result.year = timeinfo->tm_year + 1900;
    temp_result.month = timeinfo->tm_mon + 1;
    temp_result.day = timeinfo->tm_mday;
    temp_result.hour = timeinfo->tm_hour;
    temp_result.min = timeinfo->tm_min;
    temp_result.rank = 0;

    fp = fopen("result.txt", "a");
    if (fp == NULL) {
        printf("결과 파일 저장에 실패했습니다.\n");
        return;
    }

    fprintf(fp, "%s,%ld,%d,%d,%d,%d,%d,%d\n",
            temp_result.name, temp_result.point,
            temp_result.year, temp_result.month, temp_result.day,
            temp_result.hour, temp_result.min, temp_result.rank);

    fclose(fp);

    printf("결과가 저장되었습니다!\n");
    printf("최종 점수: %ld점\n", temp_result.point);
    printf("계속하려면 Enter키를 누르세요...");
    
    getchar(); // 버퍼에 enter가 입력되므로 getchar() 2회
    getchar();
}

int read_results_from_file(const char* filename) {
    FILE *fp;
    char line[200];
    result_count = 0;

    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("결과 파일을 열 수 없습니다: %s\n", filename);
        printf("현재 디렉토리에 result.txt 파일이 있는지 확인하세요.\n");
        return 0;
    }

    printf("파일을 성공적으로 열었습니다.\n");

    while (fgets(line, sizeof(line), fp) != NULL && result_count < 100) {
        sscanf(line, "%[^,],%ld,%d,%d,%d,%d,%d,%d",
               game_results[result_count].name,
               &game_results[result_count].point,
               &game_results[result_count].year,
               &game_results[result_count].month,
               &game_results[result_count].day,
               &game_results[result_count].hour,
               &game_results[result_count].min,
               &game_results[result_count].rank);

        result_count++;
    }

    fclose(fp);
    printf("총 %d개의 기록을 읽었습니다.\n", result_count);
    return result_count;
}

void sort_results_by_score(void) {
    struct result temp;

    for (int i = 0; i < result_count - 1; i++) {
        for (int j = 0; j < result_count - i - 1; j++) {
            if (game_results[j].point < game_results[j + 1].point) {
                temp = game_results[j];
                game_results[j] = game_results[j + 1];
                game_results[j + 1] = temp;
            }
        }
    }

    for (int i = 0; i < result_count; i++) {
        game_results[i].rank = i + 1;
    }
}

void search_result(void) {
    char search_name[30];
    int found = 0;

    if (read_results_from_file("result.txt") == 0) {
        return;
    }

    sort_results_by_score();

    printf("검색할 이름을 입력하세요: ");
    scanf("%s", search_name);

    printf("\n=== 검색 결과 ===\n\n");

    for (int i = 0; i < result_count; i++) {
        if (strcmp(game_results[i].name, search_name) == 0) {
            printf("%d위. %s : %ldpoint (%d-%02d-%02d)\n",
                   i + 1,
                   game_results[i].name,
                   game_results[i].point,
                   game_results[i].year,
                   game_results[i].month,
                   game_results[i].day);
            found = 1;
        }
    }
    printf("\n==================\n");
    
    if (!found) {
        printf("'%s' 이름의 기록을 찾을 수 없습니다.\n", search_name);
    }
    getchar();
}

void print_result(void) {
    if (read_results_from_file("result.txt") == 0) {
        return;
    }

    sort_results_by_score();

    if (result_count == 0) {
        printf("기록이 없습니다.\n");
        return;
    }

    printf("\n=== 게임 기록 순위 ===\n\n");

    for (int i = 0; i < result_count; i++) {
        printf("%d위. %s : %ldpoint (%d-%02d-%02d)\n",
               i + 1,
               game_results[i].name,
               game_results[i].point,
               game_results[i].year,
               game_results[i].month,
               game_results[i].day);
    }
     printf("\n=======================\n");
}

void draw_next_block(void) {
    char (*next_block)[4][4];
    
    // 다음 블록 타입에 따라 배열 선택
    switch(next_block_number) {
        case I_BLOCK: next_block = i_block; break;
        case T_BLOCK: next_block = t_block; break;
        case S_BLOCK: next_block = s_block; break;
        case Z_BLOCK: next_block = z_block; break;
        case L_BLOCK: next_block = l_block; break;
        case J_BLOCK: next_block = j_block; break;
        case O_BLOCK: next_block = o_block; break;
        default: next_block = i_block; break;
    }
    
    printf("다음 블록:\n");
    
#ifdef __linux__
    // 리눅스용 - 
    for (int i = 0; i < 4; i++) {
        printf("  "); // 들여쓰기
        for (int j = 0; j < 4; j++) {
            if (next_block[0][i][j] == 1) { // 첫 번째 회전 상태만 표시
                printf("■ ");  // draw_board의 채워진 블록과 동일
            } else {
                printf("  ");
            }
        }
        printf("\n");
    }
#else
    // macOS/Windows용 - 
    for (int i = 0; i < 4; i++) {
        printf("  "); // 들여쓰기
        for (int j = 0; j < 4; j++) {
            if (next_block[0][i][j] == 1) { // 첫 번째 회전 상태만 표시
                printf("⬜");  // draw_board의 채워진 블록과 동일
            } else {
                printf("  ");
            }
        }
        printf("\n");
    }
#endif
    printf("\n");
}
