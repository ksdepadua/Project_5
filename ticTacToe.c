#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <mosquitto.h>
#include "ticTacToe.h"

#define PLAYER1 1
#define PLAYER2 2
#define NUM_SIDES 3
#define PLAYER1MOVE 'X'
#define PLAYER2MOVE 'O'
#define PLAYER1TURNSTATUS "Player 1's Turn"
#define PLAYER2TURNSTATUS "Player 2's Turn"
#define PLAYER1WINSTATUS "Player 1 wins!"
#define PLAYER2WINSTATUS "Player 2 wins!"
#define DRAWSTATUS "It's a draw!"

struct mosquitto *mosq = NULL;
volatile int received = 0; // Flag, Set to 1 when message is received
int esp32input; // Stores payload

struct Move {
    int row, col;
};

char player1 = 'x', player2 = 'o';

bool isMovesLeft(char board[3][3])
{
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            if (board[i][j] == '_')
                return true;
    return false;
}


int evaluate(char b[3][3])
{
    // Checking for Rows for X or O victory.
    for (int row = 0; row < 3; row++) {
        if (b[row][0] == b[row][1]
            && b[row][1] == b[row][2]) {
            if (b[row][0] == player1)
                return +10;
            else if (b[row][0] == player2)
                return -10;
        }
    }

    // Checking for Columns for X or O victory.
    for (int col = 0; col < 3; col++) {
        if (b[0][col] == b[1][col]
            && b[1][col] == b[2][col]) {
            if (b[0][col] == player1)
                return +10;

            else if (b[0][col] == player2)
                return -10;
        }
    }

    // Checking for Diagonals for X or O victory.
   if (b[0][0] == b[1][1] && b[1][1] == b[2][2]) {
        if (b[0][0] == player1)
            return +10;
        else if (b[0][0] == player2)
            return -10;
    }

    if (b[0][2] == b[1][1] && b[1][1] == b[2][0]) {
        if (b[0][2] == player1)
            return +10;
        else if (b[0][2] == player2)
            return -10;
    }

    // Else if neither of them have won, return 0
    return 0;
}

void showBoard(char board[][NUM_SIDES])
{
    printf("\n\n");
    printf("\t\t\t %c | %c | %c \n", board[0][0],
           board[0][1], board[0][2]);
    printf("\t\t\t--------------\n");
    printf("\t\t\t %c | %c | %c \n", board[1][0],
           board[1][1], board[1][2]);
    printf("\t\t\t--------------\n");
    printf("\t\t\t %c | %c | %c \n\n", board[2][0],
           board[2][1], board[2][2]);
}

void showInstructions()
{
    printf("\t\t\t Tic-Tac-Toe\n\n");
    printf("Choose a cell numbered from 1 to 9 as below "
           "and play\n\n");

    printf("\t\t\t 1 | 2 | 3 \n");
    printf("\t\t\t--------------\n");
    printf("\t\t\t 4 | 5 | 6 \n");
    printf("\t\t\t--------------\n");
    printf("\t\t\t 7 | 8 | 9 \n\n");

    printf("-\t-\t-\t-\t-\t-\t-\t-\t-\t-\n\n");
}

void initialize(char board[][NUM_SIDES], int moves[])
{
    // Initially, the board is empty
    for (int i = 0; i < NUM_SIDES; i++) {
        for (int j = 0; j < NUM_SIDES; j++)
            board[i][j] = ' ';
    }

    // Fill the moves with numbers
    for (int i = 0; i < NUM_SIDES * NUM_SIDES; i++)
        moves[i] = i;
}

void declareWinner(int whoseTurn)
{
    if (whoseTurn == PLAYER2) {
	sendStatus(PLAYER2WINSTATUS);
        printf("Player 2 has won\n");
    }
    else {
	sendStatus(PLAYER1WINSTATUS);
        printf("Player 1 has won\n");
    }
}

int rowCrossed(char board[][NUM_SIDES])
{
    for (int i = 0; i < NUM_SIDES; i++) {
        if (board[i][0] == board[i][1]
            && board[i][1] == board[i][2]
            && board[i][0] != ' ')
            return 1;
    }
    return 0;
}

int columnCrossed(char board[][NUM_SIDES])
{
    for (int i = 0; i < NUM_SIDES; i++) {
        if (board[0][i] == board[1][i]
            && board[1][i] == board[2][i]
            && board[0][i] != ' ')
            return 1;
    }
    return 0;
}

int diagonalCrossed(char board[][NUM_SIDES])
{
    if ((board[0][0] == board[1][1]
         && board[1][1] == board[2][2]
         && board[0][0] != ' ')
        || (board[0][2] == board[1][1]
            && board[1][1] == board[2][0]
            && board[0][2] != ' '))
        return 1;

    return 0;
}

int promptGameMode() {
    int input;
    printf("Would you like to play Single-Player (1) or Multi-Player (2) Mode? ");
    scanf("%d", &input);
    return input;
}

int gameOver(char board[][NUM_SIDES])
{
    return (rowCrossed(board) || columnCrossed(board)
            || diagonalCrossed(board));
}

void getAvailSpaces(const char board[][NUM_SIDES], char *availSpaces) {
    char spaces[16];

    int j = 0;
    for(int i = 0; i < 9; i++) {
	int row = i / 3, col = i % 3;
	    if (board[row][col] == ' ') {
    	        spaces[j] = '0' + (i + 1);        // store as ASCII digit
    	        j++;
	    }
    }

    spaces[j] = '\0'; // null terminate
    memcpy(availSpaces, spaces, strlen(spaces) + 1);
}

/* START MQTT FOR ESP32 */
static void on_connect(struct mosquitto *mosq, void *userdata, int rc)
{
    if(rc) {
        printf("Error with result code: %d\n", rc);
        exit(-1);
    }

    mosquitto_subscribe(mosq, NULL, "keypadInput", 0);
}

static void on_message(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *msg) {
    char *payload = (char *)msg->payload;
    printf("%s\n", payload);

    esp32input = payload[0] - '0';
    received = 1;
}

void sendStatus(const char* status) {
    mosquitto_publish(mosq, NULL, "statusOutput", strlen(status), status, 0, false);
}

void sendAvailSpaces(const char* spaces) {
    char mssg[32];
    int j = 0;
    for(int i = 0; i < strlen(spaces); ++i) {
        if (i > 0) {
            mssg[j++] = ',';
        }
        mssg[j++] = spaces[i];
    }
    mssg[j] = '\0';
    mosquitto_publish(mosq, NULL, "availSpacesOutput", strlen(mssg), mssg, 0, false);
}
/* END MQTT FOR ESP32*/

void playTicTacToe(int whoseTurn)
{
    // A 3*3 Tic-Tac-Toe board for playing
    char board[NUM_SIDES][NUM_SIDES];
    int moves[NUM_SIDES * NUM_SIDES];
    char status[32];
    char availSpaces[16];

    // Initialise the game
    initialize(board, moves);

    // Show the instructions before playing
    showInstructions();

    int moveIndex = 0, x, y;
    mosquitto_lib_init();
    mosq = mosquitto_new("esp32", true, NULL);
    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_message_callback_set(mosq, on_message);
    mosquitto_connect(mosq, "localhost", 1883, 60);
    mosquitto_loop_start(mosq);

    // Wait until connected
    while(mosquitto_socket(mosq) < 0)
        usleep(10000);
    usleep(100000); // small extra delay for on_connect to fire

    // Keep playing until the game is over or it is a draw
    while (!gameOver(board) && moveIndex != NUM_SIDES * NUM_SIDES) {
        if (whoseTurn == PLAYER1) {
	    sendStatus(PLAYER1TURNSTATUS);
            memset(availSpaces, 0, sizeof(availSpaces));
	    getAvailSpaces(board, availSpaces);
	    sendAvailSpaces(availSpaces);

            int move;
            printf("Enter your move Player 1 (1-9): ");
            scanf("%d", &move);
            if (move < 1 || move > 9) {
                printf("Invalid input! Please enter a "
                       "number between 1 and 9.\n");
                continue;
            }
            x = (move - 1) / NUM_SIDES;
            y = (move - 1) % NUM_SIDES;
            if (board[x][y] == ' ') {
                board[x][y] = PLAYER1MOVE;
                showBoard(board);
                moveIndex++;
                if (gameOver(board)) {
		    //mosquitto_destroy(mosq);
		    //mosquitto_lib_cleanup();
		    declareWinner(PLAYER1);
                    return;
                }

		// End turn
                whoseTurn = PLAYER2;
            }
            else {
                printf("Cell %d is already occupied. Try "
                       "again.\n",
                       move);
            }
        }
        else if (whoseTurn == PLAYER2) {
            sendStatus(PLAYER2TURNSTATUS);
            memset(availSpaces, 0, sizeof(availSpaces));
	    getAvailSpaces(board, availSpaces);
	    sendAvailSpaces(availSpaces);

	    int move;
            printf("Enter your move Player 2 (1-9): ");

	    // Wait for message to be published by ESP32
	    while(!received)
	        usleep(10000);

	    move = esp32input;
	    received = 0; // reset flag

            if (move < 1 || move > 9) {
                printf("Invalid input! Please enter a "
                       "number between 1 and 9.\n");
                continue;
            }
            x = (move - 1) / NUM_SIDES;
            y = (move - 1) % NUM_SIDES;
            if (board[x][y] == ' ') {
                board[x][y] = PLAYER2MOVE;
                showBoard(board);
                moveIndex++;
                if (gameOver(board)) {
		    //mosquitto_loop_stop(mosq, true);
		    //mosquitto_destroy(mosq);
                    //mosquitto_lib_cleanup();
                    declareWinner(PLAYER2);
                    return;
                }
                whoseTurn = PLAYER1;
            }
            else {
                printf("Cell %d is already occupied. Try "
                       "again.\n",
                       move);
            }
        }
    }

    // If the game has drawn
    if (!gameOver(board) && moveIndex == NUM_SIDES * NUM_SIDES) {
        //mosquitto_destroy(mosq);
        //mosquitto_lib_cleanup();
	sendStatus(DRAWSTATUS);
	printf("It's a draw\n");
    }
    else {
        // Toggling the user to declare the actual winner
        if (whoseTurn == PLAYER2)
            whoseTurn = PLAYER1;
        else if (whoseTurn == PLAYER2)
            whoseTurn = PLAYER2;

        // Declare the winner
	//mosquitto_loop_stop(mosq, true);
	//mosquitto_destroy(mosq);
	//mosquitto_lib_cleanup();
        declareWinner(whoseTurn);
    }
}
