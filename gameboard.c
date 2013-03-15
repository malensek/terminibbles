#include "gameboard.h"

#include <ctype.h>
#include <curses.h>
#include <stdlib.h>
#include <string.h>

/*
 * Clear the given gameboard, then read the given level file and place its tiles
 * onto the gameboard.
 */
int load_level(char *filename, struct game_board *board)
{
    for (int i = 0; i < BOARD_SZ; ++i) {
        board->tile[i] = TILE_EMPTY;
    }

    if (filename == NULL) {
        /* Loading a blank level; we're done here. */
        return 0;
    }

    FILE *f = fopen(filename, "rt");
    if (f == NULL) {
        return -1;
    }

    char line[BOARD_W];
    int linenum = 0;
    while ((fgets(line, sizeof(line), f)) != NULL) {
        for (int ch = 0; ch < sizeof(line); ++ch) {
            int tileno = linenum * BOARD_W + ch;
            char flag = toupper(line[ch]);

            if (flag == 'X') {
                board->tile[tileno] = TILE_WALL;
            }
        }
        ++linenum;

        /* Throw away any additional characters beyond the line size limit */
        while (strchr(line, '\n') == NULL) {
            fgets(line, sizeof(line), f);
        }
    }
    fclose(f);

    return 0;
}

/*
 * Prepare a gameboard for play.  Positions the initial snake and food on the
 * board and intializes state tracking variables.
 */
void init_board(struct game_board *board)
{
    int head_x = BOARD_W / 3;
    int head_y = BOARD_H / 2;
    int food_x = BOARD_W / 3 * 2;
    int food_y = BOARD_H / 2;

    board->tile[board_idx(head_y, head_x)] = TILE_HEAD;
    board->tile[board_idx(food_y, food_x)] = TILE_FOOD;

    /*
     * Initialize snake body parts: each body tile contains the board index of
     * the next body tile.
     */
    int head = board_idx(head_y, head_x);
    board->head = head;
    int last = head;
    for (int i = 1; i < INIT_SNAKE_SZ; ++i) {
        board->tile[head - i] = last;
        last = head - i;
    }

    board->tail = last;
    board->direction = DIR_RIGHT;
    board->size = INIT_SNAKE_SZ;
}

/*
 * Update the gameboard state.  Each time this function is called, the snake
 * moves forward one tile in its current direction.
 *
 * Returns the change in snake size or -1 if the snake has died.
 */
int update_board(struct game_board *board) {
    int new_head = board->head + board->direction;
    int old_size = board->size;

    /* Are we dead yet? */
    if (new_head < 0 || new_head >= BOARD_SZ || board->tile[new_head] >= 0 ||
        abs(board->head % BOARD_W - new_head % BOARD_W) > 1) {
        return -1;
    }

    if (board->tile[new_head] == TILE_FOOD) {
        board->size++;

        /* Ensure that the new food goes on an empty tile. */
        int tile_idx[BOARD_SZ];
        int i, num_empty;
        for (i = 0, num_empty = 0; i < BOARD_SZ; ++i) {
            if (board->tile[i] == TILE_EMPTY) {
                tile_idx[num_empty++] = i;
            }
        }

        int rand = (random() % num_empty) + 1;
        int new_idx = tile_idx[rand];

        board->tile[new_idx] = TILE_FOOD;
    } else {
        /*
         * If no food has been eaten, then the last body tile is cleared to move
         * the snake forward.
         */
        int new_tail = board->tile[board->tail];
        board->tile[board->tail] = TILE_EMPTY;
        board->tail = new_tail;
    }

    /* Update the snake's head position. */
    board->tile[new_head] = TILE_HEAD;
    board->tile[board->head] = new_head;
    board->head = new_head;

    return board->size - old_size;
}

void change_direction(struct game_board *board, int new_dir)
{
    /* Prevent the snake from reversing direction on itself (instant death!) */
    if (board->direction + new_dir != 0) {
        board->direction = new_dir;
    }
}

/*
 * Translate y, x values to an index in the 1D board array
 */
int board_idx(int y, int x)
{
    return y * BOARD_W + x;
}
