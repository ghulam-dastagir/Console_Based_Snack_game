/* Console-Based Snake Game - Programming Fundamentals Project
   Controls: W A S D to move, X to quit. Works on Windows and Linux. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ---------- OS specific part (Windows / Linux) ---------- */
#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#define sleep_ms(ms) Sleep(ms)
#define clear_screen() system("cls")
void cursor_home() { COORD p = {0, 0}; SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), p); }
void set_cursor(int show) { CONSOLE_CURSOR_INFO ci = {100, show}; SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci); }
void raw_on() {}
void raw_off() {}
#else
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
struct termios old_t;
#define sleep_ms(ms) usleep((ms) * 1000)
#define clear_screen() printf("\033[2J\033[H")
#define cursor_home() printf("\033[H")
void set_cursor(int show) { printf(show ? "\033[?25h" : "\033[?25l"); fflush(stdout); }

  
// DAY 2

void raw_on() { tcgetattr(0, &old_t); struct termios t = old_t; t.c_lflag &= ~(ECHO | ICANON); tcsetattr(0, TCSANOW, &t); }
void raw_off() { tcsetattr(0, &old_t); }
int _kbhit() { struct timeval tv = {0, 0}; fd_set f; FD_ZERO(&f); FD_SET(0, &f); return select(1, &f, NULL, NULL, &tv) > 0; }
int _getch() { return getchar(); }
#endif

/* ---------- Settings ---------- */
#define W 40                 /* game area width  */
#define H 20                 /* game area height */
#define MAX_OBS 30
#define FILE_NAME "highscore.txt"

/* ---------- Structures ---------- */
typedef struct { int x, y; } Point;

typedef struct {
    Point body[W * H];       /* body[0] is the head */
    int length;
    char dir, last;          /* w a s d, 0 = not moving yet */
} Snake;

typedef struct {
    char name[16];
    int score, level, eaten, speed;
    Point food, bonus;
    int bonus_time;          /* moves left for bonus food, 0 = no bonus */

    // day 3
        Point obs[MAX_OBS];
    int obs_count;
} Game;

Snake s;
Game g;

/* ---------- File handling ---------- */
int load_high(char *name)
{
    int sc = 0;
    FILE *f = fopen(FILE_NAME, "r");
    strcpy(name, "---");

    if (f != NULL) {
        if (fscanf(f, "%15s %d", name, &sc) != 2) {
            strcpy(name, "---");
            sc = 0;
        }
        fclose(f);
    }

    return sc;
}

void save_high(char *name, int sc)
{
    FILE *f = fopen(FILE_NAME, "w");

    if (f != NULL) {
        fprintf(f, "%s %d\n", name, sc);
        fclose(f);
    }
}
// day 4 
/* ---------- Helper functions ---------- */
int same(Point a, Point b)
{
    return a.x == b.x && a.y == b.y;
}

/* is cell (x,y) empty? */
int is_free(Point p)
{
    int i;

    for (i = 0; i < s.length; i++)
        if (same(s.body[i], p))
            return 0;

    for (i = 0; i < g.obs_count; i++)
        if (same(g.obs[i], p))
            return 0;

    if (same(g.food, p))
        return 0;

    if (g.bonus_time > 0 && same(g.bonus, p))
        return 0;

    return 1;
} 
// day 5 
/* put p on a random empty cell. keep_away = 1 keeps it away from the snake head */
void random_cell(Point *p, int keep_away)
{
    Point t;
    int tries;

    for (tries = 0; tries < 2000; tries++) {
        t.x = rand() % W;
        t.y = rand() % H;

        if (!is_free(t))
            continue;

        if (keep_away &&
            abs(t.x - s.body[0].x) + abs(t.y - s.body[0].y) < 6)
            continue;

        *p = t;
        return;
    }
}

void setup(char *name)
{
    memset(&g, 0, sizeof(g));
    strcpy(g.name, name);
    g.level = 1;
    g.speed = 150;

    // day 6

        s.length = 1;
    s.body[0].x = W / 2;
    s.body[0].y = H / 2;
    s.dir = s.last = 0;
    g.food.x = g.food.y = -1;
    random_cell(&g.food, 0);
}

/* ---------- Drawing ---------- */
void draw(int high)
{
    static char out[4000];          /* whole screen is built here, printed once */
    char grid[H][W], *p = out;
    int i, x, y;

    for (y = 0; y < H; y++)
        for (x = 0; x < W; x++)
            grid[y][x] = ' ';

    for (i = 0; i < g.obs_count; i++)
        grid[g.obs[i].y][g.obs[i].x] = 'X';

    grid[g.food.y][g.food.x] = 'F';

    // day 7 

        if (g.bonus_time > 0)
        grid[g.bonus.y][g.bonus.x] = 'B';

    for (i = s.length - 1; i > 0; i--)
        grid[s.body[i].y][s.body[i].x] = 'o';

    grid[s.body[0].y][s.body[0].x] = '@';

    p += sprintf(p, "Player:%s   Score:%d   High:%d   Level:%d   Speed:%dms\n",
                 g.name, g.score, high, g.level, g.speed);

    for (x = 0; x < W + 2; x++)
        *p++ = '#';

    *p++ = '\n';

    for (y = 0; y < H; y++) {
        *p++ = '#';

        for (x = 0; x < W; x++)
            *p++ = grid[y][x];

        *p++ = '#';
        *p++ = '\n';
    }

    for (x = 0; x < W + 2; x++)
        *p++ = '#';

    *p++ = '\n';

    // day 8

        if (g.bonus_time > 0)
        p += sprintf(p, "BONUS (%d moves)\n", g.bonus_time);

    *p = '\0';

    clear_screen();
    cursor_home();
    printf("%s", out);
}

/* ---------- Snake movement ---------- */
int move_snake(void)
{
    Point next = s.body[0];
    int i;

    switch (s.dir) {
        case 'w': next.y--; break;
        case 's': next.y++; break;
        case 'a': next.x--; break;
        case 'd': next.x++; break;
        default: return 1;
    }

    if (next.x < 0 || next.x >= W || next.y < 0 || next.y >= H)
        return 0;