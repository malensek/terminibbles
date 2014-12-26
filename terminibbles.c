/* Copyright (c) 2013 Matthew Malensek.  See LICENSE file for details. */
#include <ctype.h>
#include <curses.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <menu.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "gameboard.h"

#ifndef VERSION
#define VERSION "0.0"
#endif

#define BLANK_CHAR ' '
#define BODY_CHAR  '|'
#define FOOD_CHAR  ':'
#define HEAD_CHAR  'O'
#define WALL_CHAR  'X'

/* Increase difficulty level every NEXT_DIFFICULTY points in progressive
 * difficulty mode */
#define NEXT_DIFFICULTY 25

/* System level directory */
#ifndef LEVEL_DIR
#define LEVEL_DIR "./levels"
#endif

/* Highscore files */
#define SCORE_FILE_EASY "score_easy"
#define SCORE_FILE_MEDIUM "score_medium"
#define SCORE_FILE_HARD "score_hard"
#define SCORE_FILE_EZSCC "score_ezscc"

char *difficulties[] = {
    "Easy",
    "Medium",
    "Hard",
    "Extreme Zesty Sour Cream and Cheddar",
};

char *score_files[] = {
    SCORE_FILE_EASY,
    SCORE_FILE_MEDIUM,
    SCORE_FILE_HARD,
    SCORE_FILE_EZSCC
};

WINDOW *game_win;
struct game_board board;
bool paused = false;
bool game_over = false;
int score = 0;

/*
 * Handles the actions associated with keypresses.
 */
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
    } else if (tile == TILE_WALL) {
        display = WALL_CHAR | A_NORMAL;
    } else {
        /* Snake body tiles don't have a specific identifier, so if nothing else
         * matches we can assume this is a body tile. */
        display = BODY_CHAR | A_NORMAL;
        tile = TILE_BODY;
    }

    (void) wattrset(game_win, COLOR_PAIR(-tile));
    (void) mvwaddch(game_win, y + 1, x * 2 + 1, display);
    (void) waddch(game_win, display);
    (void) wattrset(game_win, COLOR_PAIR(-TILE_EMPTY));
}

/*
 * Draws the entire "gameboard" -- snake, food, obstacles.
 */
void draw_board()
{
    int y, x, tile;
    for (y = 0; y < BOARD_H; ++y) {
        for (x = 0; x < BOARD_W; ++x) {
            tile = board.tile[board_idx(y, x)];
            draw_tile(y, x, tile);
        }
    }
    (void) wattrset(game_win, COLOR_PAIR(-TILE_EMPTY));
}

/*
 * Draws the player's current score (number of food items eaten)
 */
void draw_score()
{
    mvwprintw(game_win, 0, BOARD_W * 2 - 20, "| Score: %4d |", score);
}

/*
 * Save the player's score to disk
 */
void save_score(int difficulty)
{
    char *fp = score_files[difficulty];
    FILE *f = fopen(fp, "w");
    fprintf(f, "%d", score);
    fclose(f);
}

/*
 * Read the player's highscore
 */
int get_highscore(int difficulty)
{
    int highscore;
    char *fp = score_files[difficulty];
    FILE *f = fopen(fp, "r");
    if (f == NULL) {
        return 0;
    }

    fscanf(f, "%d", &highscore);
    fclose(f);

    return highscore;
}

/*
 * Write a fancy title splash screen using gameboard pieces.
 */
void splash()
{
    unsigned char splash[] = {
        0xed, 0x95, 0x65, 0x99, 0x36, 0x92, 0xba,
        0xaa, 0xaa, 0x49, 0x36, 0x55, 0x56, 0x64,
        0xda, 0x4a, 0xaa, 0xaa, 0xa9, 0x14, 0xd5,
        0x55, 0x59, 0x9b, 0x68, 0x00, 0x00, 0x00,
        0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x20,
        0x00, 0x00, 0x00, 0x00, 0x7F, 0x80, 0x00
    };

    int offset = BOARD_W * 3; /* three lines from the top */
    int i;
    for (i = 0; i < sizeof splash * 8; ++i) {
        if ((splash[i / 8] << i % 8) & 0x80) {
            board.tile[offset + i] = TILE_BODY;
        }
    }
}

/*
 * Retrieve the game window's y coordinate.
 */
int winy()
{
    int max_row = getmaxy(stdscr);
    return (max_row - BOARD_H) / 2 - 1;
}

/*
 * Retrieve the game window's x coordinate.
 */
int winx()
{
    int max_col = getmaxx(stdscr);
    return (max_col - BOARD_W * 2) / 2 - 1;
}

/*
 * Make sure we've got enough room to work with in the terminal.
 */
void checksize()
{
    if (winy() < 0 || winx() < 0) {
        endwin();
        printf("Terminal size not supported.\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * Completely redraw the game.  Generally needed when we get a SIGWINCH.
 */
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

/*
 * Avoid breaking terminals when exiting.
 */
void cleanup()
{
    endwin();
    exit(EXIT_SUCCESS);
}

/*
 * Determines where the user configuration directory is located. (Usually
 * $HOME/.config/terminibbles). Includes the trailing slash character.
 */
void conf_dir(char *buffer)
{
    char *dir;
    char *subdir = "/";

    dir = getenv("XDG_CONFIG_HOME");
    if (dir != NULL) {
        subdir = "/terminibbles/";
    } else {
        dir = getenv("HOME");
        if (dir != NULL) {
            subdir = "/.config/terminibbles/";
        } else {
            /* Fallback */
            dir = "./scores/";
        }
    }
    strcat(buffer, dir);
    strcat(buffer, subdir);
}

/*
 * Recursively creates a path (collection of directories/subdirectories).
 * The input path must contain a trailing slash ('/').
 */
void mkpath(const char *path)
{
    char *pathcp = strdup(path);
    size_t len = strlen(pathcp);

    /* Determine umask */
    mode_t mask = umask(0);
    umask(mask);
    mask = 0777 - mask;

    for (int i = 1; i < len; ++i) {
        if (pathcp[i] == '/') {
            pathcp[i] = 0;
            mkdir(pathcp, mask);
            pathcp[i] = '/';
        }
    }

    free(pathcp);
}

/*
 * Print levels installed in the system level directory.
 */
void print_levels() {
    /* Print installed levels */
    printf("Levels installed: (%s)\n", LEVEL_DIR);

    DIR *dir;
    if (!(dir = opendir(LEVEL_DIR))) {
        perror("list_levels");
        return;
    }

    struct dirent *dent;
    while ((dent = readdir(dir))) {
        if (dent->d_name[0] != '.') {
            printf("%s\n", dent->d_name);
        }
    }
}

int main(int argc, char **argv)
{
    int difficulty = 1;
    char *level_name = NULL;
    bool progressive = false;
    bool countdown = true;
    bool sound = false;

    int flag;
    bool error = false; /* flag for showing usage information */
    opterr = 0; /* prevents getopt from displaying its own error messages */

    while ((flag = getopt(argc, argv, "d:l:pqsV")) != -1 && !error) {
        switch (flag) {
        case 'd':
            difficulty = atoi(optarg);
            if (difficulty < 1 || difficulty > 4) {
                error = true;
            }
            break;

        case 'l':
            level_name = optarg;
            break;

        case 'p':
            progressive = true;
            break;

        case 'q':
            countdown = false;
            break;

        case 's':
            sound = true;
            break;

        case 'V':
            printf("terminibbles %s\n", VERSION);
            printf("Compiled on %s at %s\n", __DATE__, __TIME__);
            return EXIT_SUCCESS;

        case '?':
            error = true;

            if (optopt == 'l') {
                print_levels();
                return EXIT_SUCCESS;
            } else if (optopt == 'd') {
                fprintf(stderr, "Option -%c requires an argument.\n", optopt);
            } else if (isprint(optopt)) {
                fprintf(stderr, "Unknown option '-%c'. \n", optopt);
            } else {
                fprintf(stderr, "Unknown option character '\\x%x'.\n", optopt);
            }

            break;

        default:
            abort();
        }
    }

    if (error) {
        printf("Usage: %s [-pqsV] [-d 123] [-l level-file]\n", argv[0]);
        printf("\n" \
               "-d: set difficulty:\n"\
               "    1 easy\n" \
               "    2 medium\n" \
               "    3 hard\n\n" \
               "-p: enable progressive difficulty\n" \
               "    (increases difficulty every 25 points)\n\n" \
               "-l: load level-file.  Leave blank to list levels.\n\n"
               "-q: disable 3, 2, 1 countdown\n\n" \
               "-s: enable sound (terminal bell)\n\n" \
               "Controls:\n" \
               "    Movement: WASD, HJKL, Arrow Keys\n"
               "    Pause:    p\n"
               "    Quit:     q\n\n"
               "");
        return EXIT_FAILURE;
    }

    /* 0 = easy, 1 = medium, 2 = hard */
    difficulty--;

    /* Start up ncurses stuff. */
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
    init_pair(-TILE_WALL, COLOR_BLUE, COLOR_BLUE);

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

    /* Show the splash screen */
    load_level(NULL, &board);
    init_board(&board);
    splash();
    draw_board();

    /* Make sure the level exists before doing the countdown */
    if (!load_level(level_name, &board)) {
        int err1 = errno;

        /* Try the system level directory */
        char name[PATH_MAX] = {};
        strcat(name, LEVEL_DIR);
        strcat(name, "/");
        strcat(name, level_name);
        if (!load_level(name, &board)) {
            int err2 = errno;

            /* Give up */
            endwin();
            errno = err1;
            perror("load_level");
            errno = err2;
            perror("load_level (LEVEL_DIR)");
            exit(EXIT_FAILURE);
        }
    }

    if (countdown) {
        /* The countdown! */
        int i;
        for (i = 3; i > 0; --i) {
            mvwprintw(game_win, 15, BOARD_W - 2, "%d...", i);
            wrefresh(game_win);
            sleep(1);
        }
    }

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

            if (sound) {
                printf("\a");
            }

            if (progressive) {
                if (score % NEXT_DIFFICULTY == 0 && difficulty < 4) {
                    difficulty++;
                }
            }
        }

        draw_board();
        wrefresh(game_win);

        usleep(100000 - difficulty * 25000);
    }

    endwin();

    int highscore = get_highscore(difficulty);
    bool new_highscore = false;
    if (score > highscore) {
        new_highscore = true;
        save_score(difficulty);
    }

    printf("Game Over!\n");
    printf("Difficulty: %s\n", difficulties[difficulty]);
    printf("Score: %d\n", score);

    if (new_highscore) {
        printf("Highscore: %d NEW!\n", score);
    } else {
        printf("Highscore: %d\n", highscore);
    }

    if (score == 0) {
        printf("...seriously?  Zero points?\n");
    }

    return EXIT_SUCCESS;
}
