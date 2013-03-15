#include "terminibbles.h"
#include <ctype.h>
#include <curses.h>
#include <menu.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "gameboard.h"

WINDOW *game_win;
struct game_board board;
bool paused = false;
bool game_over = false;
int score = 0;

void kbd_events()
{
    int key = getch();

    if (paused) {
        /* Ignore other keys during pause */
        if (key != 'p' && key != 'q') {
            usleep(500);
            return;
        }
    }

    switch (key) {
    case 'k':
    case 'w':
    case KEY_UP:
        change_direction(&board, DIR_UP);
        break;

    case 'j':
    case 's':
    case KEY_DOWN:
        change_direction(&board, DIR_DOWN);
        break;

    case 'h':
    case 'a':
    case KEY_LEFT:
        change_direction(&board, DIR_LEFT);
        break;

    case 'l':
    case 'd':
    case KEY_RIGHT:
        change_direction(&board, DIR_RIGHT);
        break;

    case 'p':
        paused = !paused;
        break;

    case 'q':
        if (paused) {
            paused = false;
        }
        game_over = true;
        break;
    }
}

/*
 * Draws a tile on the screen.  Each tile is actually drawn twice to create a
 * semi-square board since we assume most terminals are much wider than they are
 * tall.
 */
void draw_tile(int y, int x, int tile)
{
    unsigned long display;

    if (tile == TILE_FOOD) {
        display = FOOD_CHAR | A_NORMAL;
    } else if (tile == TILE_HEAD) {
        display = HEAD_CHAR | A_BOLD;
    } else if (tile == TILE_EMPTY) {
        display = BLANK_CHAR | A_NORMAL;
    } else {
        /* Snake body tiles don't have a specific identifier, so if nothing else
         * matches we can assume this is a body tile. */
        display = BODY_CHAR | A_NORMAL;
        tile = TILE_BODY;
    }

    (void) wattrset(game_win, COLOR_PAIR(-tile));
    (void) mvwaddch(game_win, y + 1, x * 2 + 1, display);
    (void) waddch(game_win, display);
}

void draw_board()
{
    int y, x, tile;
    for (y = 0; y < BOARD_H; ++y) {
        for (x = 0; x < BOARD_W; ++x) {
            tile = board.tile[board_idx(y, x)];
            draw_tile(y, x, tile);
        }
    }
}

void draw_score()
{
    (void) wattrset(game_win, COLOR_PAIR(1));

    int y, x;
    getyx(game_win, y, x);

    mvwprintw(game_win, 0, x - 20, "| Score: %4d |", score);
}

void splash()
{
    char splash[] = {
        0xed, 0x95, 0x65, 0x99, 0x36, 0x92, 0xba,
        0xaa, 0xaa, 0x49, 0x36, 0x55, 0x56, 0x64,
        0xda, 0x4a, 0xaa, 0xaa, 0xa9, 0x14, 0xd5,
        0x55, 0x59, 0x9b, 0x68, 0x00, 0x00, 0x00,
        0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x20,
        0x00, 0x00, 0x00, 0x00, 0x7F, 0x80, 0x00
    };

    int offset = BOARD_W * 3; /* three lines from the top */
    for (int i = 0; i < sizeof(splash) * 8; ++i) {
        if ((splash[i / 8] << i % 8) & 0x80) {
            board.tile[offset + i] = TILE_BODY;
        }
    }
}

int winy()
{
    int max_row, max_col;
    getmaxyx(stdscr, max_row, max_col);
    return (max_row - BOARD_H) / 2 - 1;
}

int winx()
{
    int max_row, max_col;
    getmaxyx(stdscr, max_row, max_col);
    return (max_col - BOARD_W * 2) / 2 - 1;
}

void checksize()
{
    if (winy() < 0 || winx() < 0) {
        endwin();
        printf("Terminal size not supported.\n");
        exit(EXIT_FAILURE);
    }
}

void refreshwin()
{
    clear();
    endwin();
    refresh();
    checksize();
    mvwin(game_win, winy(), winx());
    box(game_win, 0, 0);
    draw_board();
    draw_score();
    refresh();
    wrefresh(game_win);
}

void cleanup()
{
    endwin();
    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
    initscr();

    if (has_colors()) {
        start_color();
    }
    /* Grab the default colors.
     * We assume that -1 means the default foreground/background colors. */
    use_default_colors();
    init_pair(1, -1, -1);
    init_pair(-TILE_EMPTY, -1, -1);
    init_pair(-TILE_BODY, COLOR_GREEN, COLOR_GREEN);
    init_pair(-TILE_FOOD, COLOR_RED, COLOR_RED);
    init_pair(-TILE_HEAD, COLOR_GREEN, COLOR_GREEN);

    /* Don't display the cursor */
    curs_set(0);

    keypad(stdscr, true);

    /* Non-blocking getch() */
    nodelay(stdscr, true);

    /* Don't echo typed characters */
    noecho();

    signal(SIGWINCH, refreshwin);
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

    srandom(time(NULL));

    checksize();
    game_win = newwin(BOARD_H + 2, BOARD_W * 2 + 2, winy(), winx());
    box(game_win, 0, 0);
    draw_score();

    load_level(NULL, &board);
    init_board(&board);
    splash();
    draw_board();

    wrefresh(game_win);

    for (int i = 3; i > 0; --i) {
        mvwprintw(game_win, 15, BOARD_W - 2, "%d...", i);
        wrefresh(game_win);
        sleep(1);
    }

    load_level(NULL, &board);
    init_board(&board);
    draw_board();
    box(game_win, 0, 0);
    draw_score();

    game_over = false;
    int state = 0;
    while (!game_over) {
        kbd_events();

        if (paused) {
            continue;
        }

        state = update_board(&board);

        if (state < 0) {
            game_over = true;
        } else if (state > 0) {
            score += state;
            draw_score();
        }

        draw_board();
        wrefresh(game_win);

        usleep(100000);
    }

    endwin();
    printf("Game Over!\n");
    printf("Score: %d\n", score);
    return EXIT_SUCCESS;
}
