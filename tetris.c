/*********************************************************************
 * File: tetris.c                                                    *
 * Description: uses termbox2 to play tetris in the terminal         *
 * Author: Zachary E Graber (zachgraber27@gmail.com)                 *
 * GitHub: @zacharygraber (https://github.com/zacharygraber)         *
 * Date: 1/29/2023                                                   *
 *********************************************************************/
#define TB_IMPL

#include "termbox.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <stdbool.h>

// Widths are effectively doubled, since a single "cell" is too skinny to be square
#define BOARD_WIDTH 10 // MAX OF 255 
#define BOARD_HEIGHT 20 // MAX OF 255
#define MIN_WIDTH (BOARD_WIDTH + 2)*2
#define MIN_HEIGHT (BOARD_HEIGHT + 2)

// A game piece (a tetromino) is made of 4 blocks
typedef struct block_t {
	uint8_t x;
	uint8_t y;
} block_t;

typedef struct piece_t {
	block_t blocks[4];
	uintattr_t color;
} piece_t;

typedef enum direc_t {
	LEFT,
	RIGHT,
	DOWN // pieces can never move up
} direc_t;

// Globals ///////////
static double frame_time = 1000.0; // (in ms) start at 1sec between updates
static uintattr_t board[BOARD_WIDTH][BOARD_HEIGHT]; // board is 2D grid of colors (the active piece is NOT part of the board)
static piece_t active_piece;
static struct tb_event event = {0};
//////////////////////

void initialize();
double calculate_frame();
void draw_block(int x, int y, uintattr_t color);
void create_new_active_piece();
bool move_active_piece(direc_t d);
void sigint_handler(int sig);
void quit(const char *exit_msg);

int main(void) {
	tb_init();
	initialize();
	
	/* The main loop of the game calls `calculate_frame` which performs the
	 * logic for the next frame of the game. If that call took less than `frame_time`,
	 * it waits for the remainder of that time (so that the entire process takes *roughly* frame_time).
	 */
	double remaining_frame_time_ms, render_time;
	struct timespec sleep_ts;
	while (1) {
		tb_clear();
		render_time = calculate_frame();
		remaining_frame_time_ms = frame_time - render_time;
		tb_present();
			
		// Sleep if necessary
		if (remaining_frame_time_ms > 0) {
			sleep_ts.tv_sec = ((long) remaining_frame_time_ms) / 1000; // cast is safe to truncate since guaranteed > 0
			sleep_ts.tv_nsec = (long) (fmod(remaining_frame_time_ms, 1000.0) * 1000000);
			nanosleep(&sleep_ts, &sleep_ts);
		}

		// Handle input: TODO
		tb_peek_event(&event, 10);
		if (event.type == TB_EVENT_KEY) {
			if (event.key == TB_KEY_CTRL_C || event.key == TB_KEY_ESC || event.ch == 'q') {
				quit("keyboard quit-out");
			}
		}
	}
}

/* initializes the board and pieces needed to run the game
 */
void initialize() {
	// handle inadequate window dimensions
	if ((tb_width() < MIN_WIDTH) || (tb_height() < MIN_HEIGHT)) {
		quit("Window dimensions are too small!");
	}

	// Register sigint handler to gracefully shut down
	signal(SIGINT, sigint_handler);

	for (uint8_t i = 0; i < BOARD_WIDTH; i++) {
		for (uint8_t j = 0; j < BOARD_HEIGHT; j++) {
			board[i][j] = TB_BLACK;
		}
	}

	create_new_active_piece();

	return;
}

/* Progress the game by one frame.
 * Returns the time taken to calculate frame in ms
 */ 
double calculate_frame() {
	clock_t start = clock();
	
	// Calculate new positions of active piece
	// TODO
	move_active_piece(DOWN); // TODO: actually use return

	// Draw the outside frame of the board
	for (int i = 0; i < BOARD_WIDTH + 2; i++) {
		draw_block(i, 0, TB_WHITE); // Top row
		draw_block(i, BOARD_HEIGHT+1, TB_WHITE); // Bottom row
	}
	for (int i = 1; i <= BOARD_HEIGHT; i++) {
		draw_block(0, i, TB_WHITE); // left
		draw_block(BOARD_WIDTH+1, i, TB_WHITE); // right
	}

	// Draw the board
	for (uint8_t i = 0; i < BOARD_WIDTH; i++) {
		for (uint8_t j = 0; j < BOARD_HEIGHT; j++) {
			draw_block(i+1, j+1, board[i][j]); // +1 to account for frame
		}
	}

	// Draw active piece
    for (uint8_t i = 0; i < 4; i++) {
        draw_block(active_piece.blocks[i].x+1, active_piece.blocks[i].y+1, active_piece.color);
    }

	clock_t end = clock();
	return ((double) (end - start) * 1000.0) / CLOCKS_PER_SEC;
}

/* draws a square(ish) block of `color` at x,y in GAME GRID COORDINATES */
void draw_block(int x, int y, uintattr_t color) {
	// x coordinate is doubled since each "block" is 2 chars wide
	tb_print(2 * x, y, color, 0, "██");
}

void create_new_active_piece() {
	// TODO: implement actual tetrominos
	active_piece.color = TB_CYAN;
	active_piece.blocks[0].x = 3;
	active_piece.blocks[0].y = 0;
	active_piece.blocks[1].x = 4;
	active_piece.blocks[1].y = 0;
	active_piece.blocks[2].x = 5;
	active_piece.blocks[2].y = 0;
	active_piece.blocks[3].x = 6;
	active_piece.blocks[3].y = 0;
}

/* Moves the active piece in the direction specified
 * returns true if the piece is still in play after the move
 * returns false if moving the piece would "settle" it on the board
 */
bool move_active_piece(direc_t d) {
	block_t new_blocks[4];
	int new_x=0, new_y=0;

	// Check for each block in the piece if the new positions are valid
	for (uint8_t i = 0; i < 4; i++) {
		switch (d) {
			case LEFT:
				new_x = active_piece.blocks[i].x - 1;
                new_y = active_piece.blocks[i].y; // y doesn't change

                // Make sure moving left wouldn't put us through the left wall or in an already occupied space
                if (new_x < 0 || board[new_x][new_y] != TB_BLACK) {
                    return true; // This is a "valid" move, but the piece doesn't change positions
                }
				break;

			case RIGHT:
				new_x = active_piece.blocks[i].x + 1;
                new_y = active_piece.blocks[i].y; // y doesn't change
                if (new_x >= BOARD_WIDTH || board[new_x][new_y] != TB_BLACK) {
                    return true; // This is a "valid" move, but the piece doesn't change positions
                }
				break;

			case DOWN:
				new_x = active_piece.blocks[i].x; // x doesn't change
                new_y = active_piece.blocks[i].y + 1;
				if (new_y >= BOARD_HEIGHT || board[new_x][new_y] != TB_BLACK) {
					return false; // Piece hit the bottom of the board or another "settled" piece
				}
				break;
		}
		new_blocks[i].x = new_x;
    	new_blocks[i].y = new_y;
	}

	// All guard clauses passed: move to new positions
	for (uint8_t i = 0; i < 4; i++) {
		active_piece.blocks[i] = new_blocks[i];
	}
	return true;
}

void sigint_handler(int sig) {
	quit("Received SIGINT");
}

/* Quit the game gracefully, releasing any memory
 * and shutting down termbox2
 */
void quit(const char *exit_msg) {
	tb_shutdown();
	printf("Tetris exited: %s\n", exit_msg);
	exit(EXIT_SUCCESS);
}
