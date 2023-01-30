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

// Tetris game constants
#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20
#define MIN_WIDTH (BOARD_WIDTH + 2)
#define MIN_HEIGHT (BOARD_HEIGHT + 2)

// Globals
static double frame_time = 1000.0; // (in ms) start at 1sec between updates

// Function signatures
void initialize();
double calculate_frame();
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
	}
}

/* initializes the board and pieces needed to run the game
 */
void initialize() {
	// handle inadequate window dimensions
	if ((tb_width() < MIN_WIDTH) || (tb_height() < MIN_HEIGHT)) {
		quit("Window dimensions are too small!");
	}

	// Register sigint handler to gracefully shut down on Ctrl+C
	signal(SIGINT, sigint_handler);
	return;
}

/* Progress the game by one frame.
 * Returns the time taken to calculate frame in ms
 */ 
double calculate_frame() {
	static int counter = 0; // TODO: REMOVE ME
	clock_t start = clock();
	
	// Draw the outside frame of the board
	for (int i = 0; i < BOARD_WIDTH + 2; i++) {
		// Top row
		tb_print(i, 0, TB_WHITE, 0, "█");
		// Bottom row
		tb_print(i, BOARD_HEIGHT+1, TB_WHITE, 0, "█");
	}
	for (int i = 1; i <= BOARD_HEIGHT; i++) {
		// Left side
		tb_print(0, i, TB_WHITE, 0, "█");
		// Right side
		tb_print(BOARD_WIDTH+1, i, TB_WHITE, 0, "█");
	}

	tb_printf(2, 2, TB_WHITE, TB_DEFAULT, "%d", counter++); // TODO: REMOVE ME

	clock_t end = clock();
	return ((double) (end - start) * 1000.0) / CLOCKS_PER_SEC;
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
