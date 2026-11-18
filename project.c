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