#include <stdbool.h>

/** 
 *  Returns true if there's moves remaining on the board.
 *  False if there's no more moves to play.
 */
bool isMovesLeft(char[3][3]);

/** Checks for any victories */
int evaluate(char[3][3]);

/** Prints the game board */
void showBoard(char[3][3]);

/** Prints the instructions */
void showInstructions();

/** Initializes the game */
void initialize(char[][3], int[]);

/** Prints the winner of the game */
void declareWinner(int);

/** Checks if any row is crossed with the same player's move */
int rowCrossed(char[][3]);

/** Checks if any column is crossed with teh same player's move */
int columnCrossed(char[][3]);

/** Checks if any diagonal is crossed with the same player's move */
int diagonalCrossed(char[][3]);

/** Checks if the game is over */
int gameOver(char[][3]);

/** Starts the game */
void playTicTacToe(int);
