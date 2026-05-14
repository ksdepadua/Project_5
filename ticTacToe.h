#include <stdbool.h>
#include <mosquitto.h>

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

/** Prompt users if they want to play single- or multiplayer mode */
int promptGameMode();

/** Checks if the game is over */
int gameOver(char[][3]);

/** Stores available spaces into a string */
void getAvailSpaces(const char[][3], char *);

/* START MQTT FOR ESP32 */
/** Defines how to act upon connection */
static void on_connect(struct mosquitto *, void *, int);

/** Defines how to act when a message is published on a topic */
static void on_message(struct mosquitto *, void *, const struct mosquitto_message *);

/** Send status to ESP32 */
void sendStatus(const char *);

/** Send available spaces to ESP32 */
void sendAvailSpaces(const char *);

/** Sends what space the players have selected per turn */
void sendPlayersTurn(const int, const int);
/* END MQTT FOR ESP32 */

/** Starts the game */
void playTicTacToe(int);
