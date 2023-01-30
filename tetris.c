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
#define BOARD_WIDTH 10 
#define BOARD_HEIGHT 20
#define MIN_WIDTH (BOARD_WIDTH + 2)*2
#define MIN_HEIGHT (BOARD_HEIGHT + 2)

// A block is a "pixel" in the game board
typedef struct block {
	bool occupied;
	uintattr_t color;
} block_t;


// Globals ///////////
static double frame_time = 1000.0; // (in ms) start at 1sec between updates
static block_t board[BOARD_WIDTH][BOARD_HEIGHT];
static struct tb_event event = {0};
//////////////////////

void initialize();
double calculate_frame();
void draw_block(int x, int y, uintattr_t color);
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
			if (event.key == TB_KEY_CTRL_C || event.key == TB_KEY_ESC) {
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

	for (int i = 0; i < BOARD_WIDTH; i++) {
		for (int j = 0; j < BOARD_HEIGHT; j++) {
			board[i][j].occupied = false;
			board[i][j].color = TB_BLACK;
		}
	}

	return;
}

/* Progress the game by one frame.
 * Returns the time taken to calculate frame in ms
 */ 
double calculate_frame() {
	clock_t start = clock();
	
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
	for (int i = 0; i < BOARD_WIDTH; i++) {
		for (int j = 0; j < BOARD_HEIGHT; j++) {
			draw_block(i+1, j+1, board[i][j].color); // +1 to account for frame
		}
	}

	clock_t end = clock();
	return ((double) (end - start) * 1000.0) / CLOCKS_PER_SEC;
}

/* draws a square(ish) block of `color` at x,y in GAME GRID COORDINATES */
void draw_block(int x, int y, uintattr_t color) {
	// x coordinate is doubled since each "block" is 2 chars wide
	tb_print(2 * x, y, color, 0, "██");
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
