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
void raw_on() { tcgetattr(0, &old_t); struct termios t = old_t; t.c_lflag &= ~(ECHO | ICANON); tcsetattr(0, TCSANOW, &t); }
void raw_off() { tcsetattr(0, TCSANOW, &old_t); }
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
        if (fscanf(f, "%15s %d", name, &sc) != 2) { strcpy(name, "---"); sc = 0; }
        fclose(f);
    }
    return sc;
}

void save_high(char *name, int sc)
{
    FILE *f = fopen(FILE_NAME, "w");
    if (f != NULL) { fprintf(f, "%s %d\n", name, sc); fclose(f); }
}

/* ---------- Helper functions ---------- */
int same(Point a, Point b) { return a.x == b.x && a.y == b.y; }

/* is cell (x,y) empty? */
int is_free(Point p)
{
    int i;
    for (i = 0; i < s.length; i++)  if (same(s.body[i], p)) return 0;
    for (i = 0; i < g.obs_count; i++) if (same(g.obs[i], p)) return 0;
    if (same(g.food, p)) return 0;
    if (g.bonus_time > 0 && same(g.bonus, p)) return 0;
    return 1;
}

/* put p on a random empty cell. keep_away = 1 keeps it away from the snake head */
void random_cell(Point *p, int keep_away)
{
    Point t;
    int tries;
    for (tries = 0; tries < 2000; tries++) {
        t.x = rand() % W;
        t.y = rand() % H;
        if (!is_free(t)) continue;
        if (keep_away && abs(t.x - s.body[0].x) + abs(t.y - s.body[0].y) < 6) continue;
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
        for (x = 0; x < W; x++) grid[y][x] = ' ';
    for (i = 0; i < g.obs_count; i++) grid[g.obs[i].y][g.obs[i].x] = 'X';
    grid[g.food.y][g.food.x] = 'F';
    if (g.bonus_time > 0) grid[g.bonus.y][g.bonus.x] = '$';
    for (i = s.length - 1; i >= 0; i--) grid[s.body[i].y][s.body[i].x] = (i == 0) ? 'O' : 'o';

    p += sprintf(p, "  Player: %-15s Level: %-2d Score: %-5d High: %-5d\n", g.name, g.level, g.score, high);
    for (x = 0; x < W + 2; x++) *p++ = '#';
    *p++ = '\n';
    for (y = 0; y < H; y++) {
        *p++ = '#';
        for (x = 0; x < W; x++) *p++ = grid[y][x];
        *p++ = '#';
        *p++ = '\n';
    }
    for (x = 0; x < W + 2; x++) *p++ = '#';
    if (g.bonus_time > 0) p += sprintf(p, "\n  Bonus $ vanishes in %2d moves!      \n", g.bonus_time);
    else                  p += sprintf(p, "\n  W A S D = move, X = quit            \n");

    cursor_home();
    fputs(out, stdout);
    fflush(stdout);
}

/* ---------- Game logic ---------- */

/* move snake one step. returns 0 = alive, 1 = wall, 2 = own body, 3 = obstacle */
int move_snake()
{
    Point n = s.body[0];
    int i, ate, bon, grow, limit;

    if (s.dir == 0) return 0;
    if (s.dir == 'w') n.y--;
    if (s.dir == 's') n.y++;
    if (s.dir == 'a') n.x--;
    if (s.dir == 'd') n.x++;
    s.last = s.dir;

    if (n.x < 0 || n.x >= W || n.y < 0 || n.y >= H) return 1;
    for (i = 0; i < g.obs_count; i++) if (same(g.obs[i], n)) return 3;

    ate  = same(n, g.food);
    bon  = g.bonus_time > 0 && same(n, g.bonus);
    grow = (ate || bon) && s.length < W * H;

    limit = grow ? s.length : s.length - 1;     /* tail cell moves away */
    for (i = 0; i < limit; i++) if (same(s.body[i], n)) return 2;

    if (grow) s.length++;
    for (i = s.length - 1; i > 0; i--) s.body[i] = s.body[i - 1];
    s.body[0] = n;

    if (g.bonus_time > 0) g.bonus_time--;
    if (bon) { g.score += 30; g.bonus_time = 0; }

    if (ate) {
        g.score += 10;
        g.eaten++;
        random_cell(&g.food, 0);
        if (g.eaten % 5 == 0) {                          /* new level */
            g.level++;
            for (i = 0; i < 3 && g.obs_count < MAX_OBS; i++)
                random_cell(&g.obs[g.obs_count++], 1);
        }
        if (g.eaten % 4 == 0 && g.bonus_time == 0) {     /* bonus food */
            random_cell(&g.bonus, 0);
            g.bonus_time = 50;
        }
        g.speed = 150 - g.eaten * 3 - (g.level - 1) * 5; /* faster */
        if (g.speed < 50) g.speed = 50;
    }
    return 0;
}

/* read keys while waiting. returns 1 if player pressed X */
int read_keys()
{
    while (_kbhit()) {
        int k = _getch();
        if (k >= 'A' && k <= 'Z') k += 32;               /* to lower case */
#ifndef _WIN32
        if (k == 27) { _getch(); _getch(); continue; }   /* ignore arrow keys */
#endif
        if (k == 'x') return 1;
        if (k == 'w' && s.last != 's') s.dir = 'w';
        if (k == 's' && s.last != 'w') s.dir = 's';
        if (k == 'a' && s.last != 'd') s.dir = 'a';
        if (k == 'd' && s.last != 'a') s.dir = 'd';
    }
    return 0;
}

void play()
{
    char name[16] = "Player", line[50], oldname[16];
    int status = 0, quit = 0, t, delay, high;

    clear_screen();
    printf("\n  Enter your name: ");
    if (fgets(line, sizeof(line), stdin)) sscanf(line, "%15s", name);

    high = load_high(oldname);
    setup(name);
    clear_screen();
    raw_on();
    set_cursor(0);                  /* hide blinking cursor */

    while (!quit && status == 0) {
        draw(high > g.score ? high : g.score);
        delay = g.speed;
        if (s.dir == 'w' || s.dir == 's') delay = delay * 3 / 2;  /* up/down look faster */
        for (t = 0; t < delay && !quit; t += 10) {
            sleep_ms(10);
            quit = read_keys();
        }
        if (!quit) status = move_snake();
    }
    raw_off();
    set_cursor(1);

    clear_screen();
    printf("\n  ===== GAME OVER =====\n\n");
    if (status == 1) printf("  You hit the wall!\n");
    if (status == 2) printf("  You hit your own body!\n");
    if (status == 3) printf("  You hit an obstacle!\n");
    printf("\n  Player: %s\n  Score : %d\n  Level : %d\n", g.name, g.score, g.level);
    if (g.score > high) { printf("\n  *** NEW HIGH SCORE! ***\n"); save_high(g.name, g.score); }
    else                  printf("\n  High score: %d (%s)\n", high, oldname);
    printf("\n  Press Enter to go back...");
    fgets(line, sizeof(line), stdin);
}

void show_high()
{
    char name[16], line[50];
    int sc = load_high(name);
    clear_screen();
    printf("\n  ===== HIGH SCORE =====\n\n");
    if (sc == 0) printf("  No high score yet.\n");
    else         printf("  %s - %d points\n", name, sc);
    printf("\n  Press Enter to go back...");
    fgets(line, sizeof(line), stdin);
}

int main()
{
    char line[50];
    int choice = 0;

    srand((unsigned)time(NULL));
    while (choice != 3) {
        clear_screen();
        printf("\n  ==== CONSOLE SNAKE GAME ====\n\n");
        printf("  1. Play Game\n  2. High Score\n  3. Exit\n\n  Choose: ");
        if (!fgets(line, sizeof(line), stdin)) break;
        choice = atoi(line);
        if (choice == 1) play();
        if (choice == 2) show_high();
    }
    return 0;
}
