/*********************************************************************
 * File: tetris.h                                                    *
 * Description: provides core structs, functions, and globals        *
 * Author: Zachary E Graber (zachgraber27@gmail.com)                 *
 * GitHub: @zacharygraber (https://github.com/zacharygraber)         *
 * Created: 2/20/2023                                                *
 *********************************************************************/

#define TB_IMPL

#include "termbox.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <math.h>
#include <stdbool.h>

#ifndef PTHREAD_HEADER_INCLUDED
	#include <pthread.h>
	#define PTHREAD_HEADER_INCLUDED
#endif

// widths here are doubled to get character-width (2 characters per board cell)
#define BOARD_WIDTH 10 // MAX OF 255 
#define BOARD_HEIGHT 20 // MAX OF 255
#define MIN_WIDTH (BOARD_WIDTH + 2)*2
#define MIN_HEIGHT (BOARD_HEIGHT + 2)

// A game piece (a tetromino) is made of 4 blocks
// Game board locations need to be signed to account for, e.g., rotating
// a piece right as it spawns, which puts it above the board (negative y-value)
typedef struct {
	int8_t x; 
	int8_t y; 
} block_t;

typedef struct {
	block_t blocks[4];
	uintattr_t color;
} piece_t;

typedef enum {
	LEFT,
	RIGHT,
	DOWN // pieces can never move up
} direc_t;

typedef enum {
	PLAY,
	PAUSE,
	GAME_OVER,
	QUIT
} game_state_t;

// Globals (needed for functions below, but defined elsewhere)
extern double drop_speed;
extern volatile uintattr_t board[BOARD_WIDTH][BOARD_HEIGHT]; // board is 2D grid of colors (the active piece is NOT part of the board)
extern volatile piece_t active_piece;
extern pthread_mutex_t board_mutex, active_piece_mutex;
extern pthread_t event_handler_pt;
extern game_state_t GAME_STATE;

// Helper functions to clean up main game loop's code
void *event_handler_pthread_routine(void *args);
void render();
void draw_block(int x, int y, uintattr_t color);
void show_321_countdown();
void create_new_active_piece();
bool move_active_piece(direc_t d);
void rotate_active_piece();
void hard_drop_active_piece();
void settle_active_piece();
void sigint_handler(int sig);

void initialize();

void pause_game() {
	GAME_STATE = PAUSE;
	return;
}

void resume_game() {
	if (GAME_STATE == PLAY) return;

	show_321_countdown();
	render();
	GAME_STATE = PLAY;
	return;
}

void game_over() {
	GAME_STATE = GAME_OVER;
	tb_print(10, 8, TB_WHITE, TB_RED, "GAME");
    tb_print(10, 9, TB_WHITE, TB_RED, "OVER");
    tb_print(11, 11, TB_WHITE, TB_RED, ":(");
    tb_present();
	return;
}

void setup_new_game();
void quit(int status, const char *exit_msg);