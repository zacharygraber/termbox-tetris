/*********************************************************************
 * File: tetris.c                                                    *
 * Description: uses termbox2 to play tetris in the terminal         *
 * Author: Zachary E Graber (zachgraber27@gmail.com)                 *
 * GitHub: @zacharygraber (https://github.com/zacharygraber)         *
 * Created: 1/29/2023                                                *
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
#include <pthread.h>

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
static double drop_speed = 1000.0; // (in ms) start by moving piece down every second
static uintattr_t board[BOARD_WIDTH][BOARD_HEIGHT]; // board is 2D grid of colors (the active piece is NOT part of the board)
static piece_t active_piece = {0};
static struct tb_event event = {0};
static pthread_mutex_t board_mutex, active_piece_mutex;
static pthread_t drop_piece_pt;

// Set to true to signal for pthreads handling keyboard input and game progression to exit
static bool STOP_GAME_LOOP = false;
//////////////////////

void initialize();
void *drop_piece_pthread_routine(void *args);
void render();
void draw_block(int x, int y, uintattr_t color);
void create_new_active_piece();
void move_active_piece(direc_t d);
void settle_active_piece();
void sigint_handler(int sig);
void quit(int status, const char *exit_msg);

int main(void) {
	tb_init();
	initialize();
	
	/* The main loop of the game calls `calculate_frame` which performs the
	 * logic for the next frame of the game. If that call took less than `frame_time`,
	 * it waits for the remainder of that time (so that the entire process takes *roughly* frame_time).
	 * TODO: UPDATE THIS
	 */
	while (true) {
		// Handle input: TODO
		tb_peek_event(&event, 10);
		if (event.type == TB_EVENT_KEY) {
			if (event.key == TB_KEY_CTRL_C || event.key == TB_KEY_ESC || event.ch == 'q') {
				quit(EXIT_SUCCESS, "keyboard quit-out");
			}
		}
		sleep(1);
	}
}

/* initializes the resources and threads needed to run the game
 */
void initialize() {
	// handle inadequate window dimensions
	if ((tb_width() < MIN_WIDTH) || (tb_height() < MIN_HEIGHT)) {
		quit(EXIT_FAILURE, "Window dimensions are too small!");
	}

	// Register sigint handler to gracefully shut down
	signal(SIGINT, sigint_handler);

	// Initialize the board to all black
	for (uint8_t i = 0; i < BOARD_WIDTH; i++) {
		for (uint8_t j = 0; j < BOARD_HEIGHT; j++) {
			board[i][j] = TB_BLACK;
		}
	}

	// Create mutexes for board and active piece
	if (pthread_mutex_init(&board_mutex, NULL) || pthread_mutex_init(&active_piece_mutex, NULL)) {
		quit(EXIT_FAILURE, "Couldn't initialize mutex");
	}

	create_new_active_piece();

	// Spawn pthreads for dropping piece and (TODO: keyboard input/controls)
	if (pthread_create(&drop_piece_pt, NULL, drop_piece_pthread_routine, NULL)) {
		quit(EXIT_FAILURE, "Failed to create pthread worker");
	}

	return;
}

/* Routine for a pthread to execute. As long as this pthread is alive, it will loop
 * infinitely, moving the active piece down every `drop_speed` ms.
 *
 * Set STOP_GAME_LOOP to true to signal for this routine to exit
 */ 
void *drop_piece_pthread_routine(void *args) {
	double remaining_wait_time_ms = drop_speed, loop_time_taken_ms = 0; // wait the full time in the first loop
    struct timespec sleep_ts;
	clock_t start, stop;
	while (!STOP_GAME_LOOP) {
        // Sleep if necessary
        if (remaining_wait_time_ms > 0) {
            sleep_ts.tv_sec = ((long) remaining_wait_time_ms) / 1000; // cast is safe to truncate since guaranteed > 0
            sleep_ts.tv_nsec = (long) (fmod(remaining_wait_time_ms, 1000.0) * 1000000);
            nanosleep(&sleep_ts, &sleep_ts);
        }
		
		start = clock();
		// Move piece down and re-render to display change
    	move_active_piece(DOWN);
    	render();
		stop = clock();

		loop_time_taken_ms = ((double) (stop - start) * 1000.0) / CLOCKS_PER_SEC;
		remaining_wait_time_ms = drop_speed - loop_time_taken_ms;
	}
	pthread_exit(NULL);
}

/* draws a square(ish) block of `color` at x,y in GAME GRID COORDINATES */
void draw_block(int x, int y, uintattr_t color) {
	// x coordinate is doubled since each "block" is 2 chars wide
	tb_print(2 * x, y, color, 0, "██");
}

/* Re-renders everything on the screen
 */
// SHOULD BE [MOSTLY] THREAD SAFE
void render() {
	tb_clear();
	// The order in which things get rendered is important!
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
	pthread_mutex_lock(&board_mutex);
    for (uint8_t i = 0; i < BOARD_WIDTH; i++) {
        for (uint8_t j = 0; j < BOARD_HEIGHT; j++) {
            draw_block(i+1, j+1, board[i][j]); // +1 to account for frame
        }
    }
	pthread_mutex_unlock(&board_mutex);

    // Draw active piece
	pthread_mutex_lock(&active_piece_mutex);
    for (uint8_t i = 0; i < 4; i++) {
        draw_block(active_piece.blocks[i].x+1, active_piece.blocks[i].y+1, active_piece.color);
    }
	pthread_mutex_unlock(&active_piece_mutex);
	tb_present();
    return;
}

// THREAD SAFE
void create_new_active_piece() {
	pthread_mutex_lock(&active_piece_mutex);

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

	pthread_mutex_unlock(&active_piece_mutex);
}

/* Moves the active piece in the direction specified
 * returns true if the piece is still in play after the move
 * returns false if moving the piece would "settle" it on the board
 */
// THREAD SAFE
void move_active_piece(direc_t d) {
	block_t new_blocks[4];
	int new_x=0, new_y=0;

	pthread_mutex_lock(&active_piece_mutex);
	pthread_mutex_lock(&board_mutex); // not writing to board, but want to make sure it doesn't change
	// Check for each block in the piece if the new positions are valid
	for (uint8_t i = 0; i < 4; i++) {
		switch (d) {
			case LEFT:
				new_x = active_piece.blocks[i].x - 1;
                new_y = active_piece.blocks[i].y; // y doesn't change

                // Make sure moving left wouldn't put us through the left wall or in an already occupied space
                if (new_x < 0 || board[new_x][new_y] != TB_BLACK) {
					pthread_mutex_unlock(&board_mutex);
					pthread_mutex_unlock(&active_piece_mutex);
                    return; // This is a "valid" move, but the piece doesn't change positions
                }
				break;

			case RIGHT:
				new_x = active_piece.blocks[i].x + 1;
                new_y = active_piece.blocks[i].y; // y doesn't change
                if (new_x >= BOARD_WIDTH || board[new_x][new_y] != TB_BLACK) {
					pthread_mutex_unlock(&board_mutex);
                    pthread_mutex_unlock(&active_piece_mutex);
                    return; // This is a "valid" move, but the piece doesn't change positions
                }
				break;

			case DOWN:
				new_x = active_piece.blocks[i].x; // x doesn't change
                new_y = active_piece.blocks[i].y + 1;
				if (new_y >= BOARD_HEIGHT || board[new_x][new_y] != TB_BLACK) {
					pthread_mutex_unlock(&board_mutex);
                    pthread_mutex_unlock(&active_piece_mutex);
					settle_active_piece();
					return; // Piece hit the bottom of the board or another "settled" piece
				}
				break;
		}
		new_blocks[i].x = new_x;
    	new_blocks[i].y = new_y;
	}
    pthread_mutex_unlock(&board_mutex);

	// All guard clauses passed: move to new positions
	for (uint8_t i = 0; i < 4; i++) {
		active_piece.blocks[i] = new_blocks[i];
	}
	pthread_mutex_unlock(&active_piece_mutex);
	return;
}

/* "Settles" the active piece by writing its current position
 * to the board and creating a new active piece
 */
// THREAD SAFE
void settle_active_piece() {
	pthread_mutex_lock(&active_piece_mutex);
    pthread_mutex_lock(&board_mutex);
	for (uint8_t i = 0; i < 4; i++) {
		board[active_piece.blocks[i].x][active_piece.blocks[i].y] = active_piece.color;
	}
	pthread_mutex_unlock(&active_piece_mutex);
    pthread_mutex_unlock(&board_mutex);
	
	create_new_active_piece();
}

void sigint_handler(int sig) {
	quit(EXIT_FAILURE, "Received SIGINT");
}

/* Quit the game gracefully, releasing any memory
 * and shutting down termbox2
 * Takes in an exit status and a message to print to stdout/stderr
 */
void quit(int status, const char *exit_msg) {
	// Destroying a locked mutex results in undefined behavior, so unlock them first
	pthread_mutex_unlock(&board_mutex);
    pthread_mutex_destroy(&board_mutex);
	pthread_mutex_unlock(&active_piece_mutex);
    pthread_mutex_destroy(&active_piece_mutex);

	// Signal for pthreads to exit, then wait for them
	STOP_GAME_LOOP = true;
	pthread_join(drop_piece_pt, NULL);

	tb_shutdown();
	fprintf((status == EXIT_SUCCESS) ? stdout : stderr, "Tetris exited: %s\n", exit_msg);
	exit(status);
}
