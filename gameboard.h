/* Copyright (c) 2013 Matthew Malensek.  See LICENSE file for details. */
#ifndef GAMEBOARD_H
#define GAMEBOARD_H

#define BOARD_W 39
#define BOARD_H 22
#define BOARD_SZ BOARD_W * BOARD_H

#define INIT_SNAKE_SZ 5

#define DIR_UP    -BOARD_W
#define DIR_DOWN   BOARD_W
#define DIR_LEFT  -1
#define DIR_RIGHT  1

#define TILE_EMPTY -5
#define TILE_HEAD  -6
#define TILE_FOOD  -7
#define TILE_BODY  -8
#define TILE_WALL  -9

struct game_board {
    int tile[BOARD_SZ];

    int head;
    int tail;

    int size;
    int direction;
};

int load_level(char *filename, struct game_board *board);
void init_board(struct game_board *board);
int update_board(struct game_board *board);
void change_direction(struct game_board *board, int new_dir);
int board_idx(int y, int x);

#endif
