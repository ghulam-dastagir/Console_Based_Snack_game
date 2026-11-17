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