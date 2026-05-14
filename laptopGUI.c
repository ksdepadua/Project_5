/*
 * laptopGUI.c  —  Passive MQTT Monitor with hostname entry screen
 *
 * Build:
 *   gcc laptopGUI.c -o laptopGUI \
 *       -lraylib -lmosquitto -lm -lpthread -lX11
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <mosquitto.h>
#include "raylib.h"

/* ─── Layout ─────────────────────────────────────────────────────────────── */
#define NUM_SIDES       3
#define CELL_SIZE       180
#define BOARD_OFFSET_X  90
#define BOARD_OFFSET_Y  200
#define WIN_W           720
#define WIN_H           740

/* ─── Palette ────────────────────────────────────────────────────────────── */
static const Color BG_COLOR     = {  10,  10,  20, 255 };
static const Color GRID_COLOR   = {  60,  60,  90, 255 };
static const Color X_COLOR      = {  80, 200, 255, 255 };
static const Color O_COLOR      = { 255, 120,  80, 255 };
static const Color WIN_LINE_CLR = { 255, 230,  60, 255 };
static const Color PANEL_COLOR  = {  18,  18,  32, 255 };
static const Color TEXT_COLOR   = { 220, 220, 240, 255 };
static const Color DIM_COLOR    = { 100, 100, 130, 255 };
static const Color CONN_OK      = {  80, 220, 120, 255 };
static const Color CONN_ERR     = { 220,  80,  80, 255 };
static const Color INPUT_BG     = {  25,  25,  45, 255 };
static const Color INPUT_ACTIVE = {  50,  50,  90, 255 };
static const Color BTN_COLOR    = {  40,  40,  70, 255 };
static const Color BTN_HOV      = {  70,  70, 110, 255 };
static const Color WARN_COLOR   = { 255, 180,  60, 255 };

/* ─── Screens ────────────────────────────────────────────────────────────── */
typedef enum { SCREEN_CONNECT, SCREEN_CONNECTING, SCREEN_GAME } Screen;
static Screen screen = SCREEN_CONNECT;

/* ─── Shared state ───────────────────────────────────────────────────────── */
static pthread_mutex_t gMutex = PTHREAD_MUTEX_INITIALIZER;

static char gBoard[NUM_SIDES][NUM_SIDES];
static char gStatus[64]  = "Waiting for game to start...";
static int  gWinCellA    = -1;
static int  gWinCellB    = -1;
static int  gConnected   = 0;   /* 1 = broker confirmed via on_connect     */
static int  gConnFailed  = 0;   /* 1 = connection attempt failed           */

/* ─── Win-line detection ─────────────────────────────────────────────────── */
static void find_win_line(void)
{
    gWinCellA = gWinCellB = -1;
    char (*b)[3] = gBoard;
    for (int i = 0; i < 3; i++) {
        if (b[i][0]!=' ' && b[i][0]==b[i][1] && b[i][1]==b[i][2])
            { gWinCellA=i*3;  gWinCellB=i*3+2; return; }
        if (b[0][i]!=' ' && b[0][i]==b[1][i] && b[1][i]==b[2][i])
            { gWinCellA=i;    gWinCellB=6+i;   return; }
    }
    if (b[0][0]!=' ' && b[0][0]==b[1][1] && b[1][1]==b[2][2])
        { gWinCellA=0; gWinCellB=8; return; }
    if (b[0][2]!=' ' && b[0][2]==b[1][1] && b[1][1]==b[2][0])
        { gWinCellA=2; gWinCellB=6; }
}

static void apply_move(int cell, char piece)
{
    if (cell < 1 || cell > 9) return;
    int r = (cell-1)/3, c = (cell-1)%3;
    pthread_mutex_lock(&gMutex);
    if (gBoard[r][c] == ' ') { gBoard[r][c] = piece; find_win_line(); }
    pthread_mutex_unlock(&gMutex);
}

/* ─── MQTT ───────────────────────────────────────────────────────────────── */
static struct mosquitto *mosq = NULL;

static void on_connect(struct mosquitto *m, void *ud, int rc)
{
    (void)ud;
    pthread_mutex_lock(&gMutex);
    if (rc == 0) {
        gConnected  = 1;
        gConnFailed = 0;
        mosquitto_subscribe(m, NULL, "player1Move",       0);
        mosquitto_subscribe(m, NULL, "player2Move",       0);
        mosquitto_subscribe(m, NULL, "statusOutput",      0);
        mosquitto_subscribe(m, NULL, "availSpacesOutput", 0);
    } else {
        gConnFailed = 1;
    }
    pthread_mutex_unlock(&gMutex);
}

static void on_disconnect(struct mosquitto *m, void *ud, int rc)
{
    (void)m; (void)ud; (void)rc;
    pthread_mutex_lock(&gMutex);
    gConnected = 0;
    pthread_mutex_unlock(&gMutex);
}

static void on_message(struct mosquitto *m, void *ud,
                       const struct mosquitto_message *msg)
{
    (void)m; (void)ud;
    if (!msg->payloadlen) return;

    char payload[256];
    int len = msg->payloadlen < 255 ? msg->payloadlen : 255;
    memcpy(payload, msg->payload, len);
    payload[len] = '\0';

    const char *topic = msg->topic;

    if (strcmp(topic, "player1Move") == 0) {
        apply_move(payload[0] - '0', 'X');
    } else if (strcmp(topic, "player2Move") == 0) {
        apply_move(payload[0] - '0', 'O');
    } else if (strcmp(topic, "statusOutput") == 0) {
        pthread_mutex_lock(&gMutex);
        strncpy(gStatus, payload, sizeof(gStatus)-1);
        gStatus[sizeof(gStatus)-1] = '\0';
        pthread_mutex_unlock(&gMutex);
    } else if (strcmp(topic, "availSpacesOutput") == 0) {
        int commas = 0;
        for (int i = 0; payload[i]; i++)
            if (payload[i] == ',') commas++;
        if (commas == 8) {
            pthread_mutex_lock(&gMutex);
            for (int i = 0; i < 3; i++)
                for (int j = 0; j < 3; j++)
                    gBoard[i][j] = ' ';
            gWinCellA = gWinCellB = -1;
            strncpy(gStatus, "Game started!", sizeof(gStatus)-1);
            pthread_mutex_unlock(&gMutex);
        }
    }
}

/* Attempt connection to given hostname. Returns 0 on success. */
static int mqtt_connect(const char *hostname)
{
    if (mosq) {
        mosquitto_loop_stop(mosq, true);
        mosquitto_destroy(mosq);
        mosq = NULL;
    }
    mosquitto_lib_init();
    mosq = mosquitto_new("ttt_monitor", true, NULL);
    if (!mosq) return -1;

    mosquitto_connect_callback_set(mosq,    on_connect);
    mosquitto_disconnect_callback_set(mosq, on_disconnect);
    mosquitto_message_callback_set(mosq,    on_message);

    int rc = mosquitto_connect(mosq, hostname, 1883, 60);
    if (rc != MOSQ_ERR_SUCCESS) return rc;
    mosquitto_loop_start(mosq);
    return 0;
}

static void mqtt_cleanup(void)
{
    if (mosq) {
        mosquitto_loop_stop(mosq, true);
        mosquitto_destroy(mosq);
        mosq = NULL;
    }
    mosquitto_lib_cleanup();
}

/* ─── Drawing helpers ────────────────────────────────────────────────────── */
static Vector2 cell_center(int idx)
{
    return (Vector2){
        BOARD_OFFSET_X + (idx%3)*CELL_SIZE + CELL_SIZE/2.0f,
        BOARD_OFFSET_Y + (idx/3)*CELL_SIZE + CELL_SIZE/2.0f
    };
}

static void draw_x(Vector2 ctr, float sz, Color col)
{
    float h = sz*0.30f;
    DrawLineEx((Vector2){ctr.x-h,ctr.y-h},(Vector2){ctr.x+h,ctr.y+h},5.f,col);
    DrawLineEx((Vector2){ctr.x+h,ctr.y-h},(Vector2){ctr.x-h,ctr.y+h},5.f,col);
}

static void draw_o(Vector2 ctr, float sz, Color col)
{
    float r = sz*0.28f;
    DrawCircleLinesV(ctr,r,    col);
    DrawCircleLinesV(ctr,r-1.f,col);
    DrawCircleLinesV(ctr,r-2.f,col);
    DrawCircleLinesV(ctr,r-3.f,col);
}

static bool button(Rectangle rect, const char *label, Font font, int fontSize)
{
    Vector2 mp  = GetMousePosition();
    bool    hov = CheckCollisionPointRec(mp, rect);
    DrawRectangleRec(rect, hov ? BTN_HOV : BTN_COLOR);
    DrawRectangleLinesEx(rect, 1.5f, GRID_COLOR);
    Vector2 tsz = MeasureTextEx(font, label, fontSize, 1);
    DrawTextEx(font, label,
               (Vector2){rect.x+(rect.width-tsz.x)*0.5f,
                         rect.y+(rect.height-tsz.y)*0.5f},
               fontSize, 1, TEXT_COLOR);
    return hov && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
}

static void draw_board(char board[NUM_SIDES][NUM_SIDES],
                       int winA, int winB, Font font)
{
    DrawRectangle(BOARD_OFFSET_X-8, BOARD_OFFSET_Y-8,
                  NUM_SIDES*CELL_SIZE+16, NUM_SIDES*CELL_SIZE+16, PANEL_COLOR);

    for (int i = 1; i < NUM_SIDES; i++) {
        DrawLineEx((Vector2){BOARD_OFFSET_X+i*CELL_SIZE, BOARD_OFFSET_Y},
                   (Vector2){BOARD_OFFSET_X+i*CELL_SIZE,
                             BOARD_OFFSET_Y+NUM_SIDES*CELL_SIZE},3.f,GRID_COLOR);
        DrawLineEx((Vector2){BOARD_OFFSET_X,             BOARD_OFFSET_Y+i*CELL_SIZE},
                   (Vector2){BOARD_OFFSET_X+NUM_SIDES*CELL_SIZE,
                             BOARD_OFFSET_Y+i*CELL_SIZE},3.f,GRID_COLOR);
    }

    for (int r = 0; r < 3; r++)
        for (int c = 0; c < 3; c++)
            if (board[r][c] == ' ') {
                char num[3]; sprintf(num,"%d",r*3+c+1);
                DrawTextEx(font, num,
                    (Vector2){BOARD_OFFSET_X+c*CELL_SIZE+8.f,
                              BOARD_OFFSET_Y+r*CELL_SIZE+6.f},22,1,DIM_COLOR);
            }

    for (int r = 0; r < 3; r++)
        for (int c = 0; c < 3; c++) {
            Vector2 ctr = cell_center(r*3+c);
            if      (board[r][c]=='X') draw_x(ctr,CELL_SIZE,X_COLOR);
            else if (board[r][c]=='O') draw_o(ctr,CELL_SIZE,O_COLOR);
        }

    if (winA >= 0 && winB >= 0)
        DrawLineEx(cell_center(winA), cell_center(winB), 7.f, WIN_LINE_CLR);
}

/* ─── Screens ────────────────────────────────────────────────────────────── */

/* hostname text input state */
static char   inputBuf[128] = "";
static int    inputLen      = 0;
static bool   inputActive   = false;
static char   errorMsg[64]  = "";
static float  connectTimer  = 0.0f;  /* seconds since connect attempt */
static bool   connecting    = false;

static void screen_connect(Font font)
{
    /* Title */
    const char *title = "TIC-TAC-TOE";
    Vector2 tsz = MeasureTextEx(font, title, 52, 2);
    DrawTextEx(font, title,
               (Vector2){(WIN_W-tsz.x)/2.f, 80}, 52, 2, TEXT_COLOR);

    const char *sub = "MQTT Monitor";
    Vector2 ssz = MeasureTextEx(font, sub, 22, 1);
    DrawTextEx(font, sub,
               (Vector2){(WIN_W-ssz.x)/2.f, 148}, 22, 1, DIM_COLOR);

    /* Label */
    const char *lbl = "Broker hostname or IP:";
    DrawTextEx(font, lbl, (Vector2){BOARD_OFFSET_X, 260}, 20, 1, TEXT_COLOR);

    /* Input box */
    Rectangle inputRect = { BOARD_OFFSET_X, 290, WIN_W - BOARD_OFFSET_X*2, 52 };
    if (CheckCollisionPointRec(GetMousePosition(), inputRect) &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        inputActive = true;
    else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        inputActive = false;

    DrawRectangleRec(inputRect, inputActive ? INPUT_ACTIVE : INPUT_BG);
    DrawRectangleLinesEx(inputRect, 2.f,
                         inputActive ? X_COLOR : GRID_COLOR);

    /* Keyboard input */
    if (inputActive) {
        int ch;
        while ((ch = GetCharPressed()) != 0) {
            if (ch >= 32 && inputLen < 127) {
                inputBuf[inputLen++] = (char)ch;
                inputBuf[inputLen]   = '\0';
            }
        }
        if (IsKeyPressed(KEY_BACKSPACE) && inputLen > 0) {
            inputBuf[--inputLen] = '\0';
        }
        if (IsKeyPressed(KEY_ENTER) && inputLen > 0) {
            /* attempt connection */
            connecting    = true;
            connectTimer  = 0.f;
            errorMsg[0]   = '\0';
            gConnected    = 0;
            gConnFailed   = 0;
            mqtt_connect(inputBuf);
            screen = SCREEN_CONNECTING;
        }
    }

    /* Draw typed text + blinking cursor */
    Vector2 tpos = { inputRect.x + 10, inputRect.y + 12 };
    DrawTextEx(font, inputBuf, tpos, 24, 1, TEXT_COLOR);
    if (inputActive && ((int)(GetTime()*2) % 2 == 0)) {
        Vector2 csz = MeasureTextEx(font, inputBuf, 24, 1);
        DrawTextEx(font, "|",
                   (Vector2){tpos.x + csz.x + 1, tpos.y}, 24, 1, X_COLOR);
    }

    /* Hint */
    DrawTextEx(font, "e.g.  localhost  or  192.168.1.10  or  my.broker.org",
               (Vector2){BOARD_OFFSET_X, 354}, 16, 1, DIM_COLOR);

    /* Connect button */
    Rectangle btnRect = { WIN_W/2.f - 100, 400, 200, 52 };
    if (button(btnRect, "Connect", font, 22) && inputLen > 0) {
        connecting   = true;
        connectTimer = 0.f;
        errorMsg[0]  = '\0';
        gConnected   = 0;
        gConnFailed  = 0;
        mqtt_connect(inputBuf);
        screen = SCREEN_CONNECTING;
    }

    /* Error message */
    if (errorMsg[0] != '\0') {
        Vector2 esz = MeasureTextEx(font, errorMsg, 19, 1);
        DrawTextEx(font, errorMsg,
                   (Vector2){(WIN_W-esz.x)/2.f, 470}, 19, 1, CONN_ERR);
    }
}

static void screen_connecting(Font font)
{
    connectTimer += GetFrameTime();

    /* Snapshot connection state */
    pthread_mutex_lock(&gMutex);
    int connected = gConnected;
    int failed    = gConnFailed;
    pthread_mutex_unlock(&gMutex);

    if (connected) {
        /* Success — switch to game screen */
        screen = SCREEN_GAME;
        return;
    }

    if (failed || connectTimer > 5.0f) {
        /* Failed — go back to connect screen with error */
        snprintf(errorMsg, sizeof(errorMsg),
                 "Could not reach \"%s\". Try again.", inputBuf);
        mqtt_cleanup();
        gConnFailed = 0;
        screen = SCREEN_CONNECT;
        return;
    }

    /* Spinner */
    const char *title = "TIC-TAC-TOE";
    Vector2 tsz = MeasureTextEx(font, title, 52, 2);
    DrawTextEx(font, title,
               (Vector2){(WIN_W-tsz.x)/2.f, 80}, 52, 2, TEXT_COLOR);

    char msg[128];
    snprintf(msg, sizeof(msg), "Connecting to  %s ...", inputBuf);
    Vector2 msz = MeasureTextEx(font, msg, 22, 1);
    DrawTextEx(font, msg,
               (Vector2){(WIN_W-msz.x)/2.f, 300}, 22, 1, DIM_COLOR);

    /* Animated dots */
    int dots = ((int)(connectTimer * 3)) % 4;
    char dotStr[5] = "    ";
    for (int i = 0; i < dots; i++) dotStr[i] = '.';
    dotStr[dots] = '\0';
    Vector2 dsz = MeasureTextEx(font, dotStr, 36, 1);
    DrawTextEx(font, dotStr,
               (Vector2){(WIN_W-dsz.x)/2.f, 340}, 36, 2, X_COLOR);

    /* Timeout bar */
    float progress = connectTimer / 5.0f;
    DrawRectangle(BOARD_OFFSET_X, 420,
                  (int)((WIN_W - BOARD_OFFSET_X*2) * progress), 6,
                  X_COLOR);
    DrawRectangleLines(BOARD_OFFSET_X, 420,
                       WIN_W - BOARD_OFFSET_X*2, 6, GRID_COLOR);

    /* Cancel button */
    Rectangle btnRect = { WIN_W/2.f - 80, 460, 160, 48 };
    if (button(btnRect, "Cancel", font, 20)) {
        mqtt_cleanup();
        gConnected  = 0;
        gConnFailed = 0;
        screen = SCREEN_CONNECT;
    }
}

static void screen_game(Font font)
{
    /* Snapshot */
    char board[NUM_SIDES][NUM_SIDES];
    char status[64];
    int  winA, winB, connected;

    pthread_mutex_lock(&gMutex);
    memcpy(board,  gBoard,  sizeof(board));
    memcpy(status, gStatus, sizeof(status));
    winA      = gWinCellA;
    winB      = gWinCellB;
    connected = gConnected;
    pthread_mutex_unlock(&gMutex);

    /* Title */
    const char *title = "TIC-TAC-TOE";
    Vector2 tsz = MeasureTextEx(font, title, 52, 2);
    DrawTextEx(font, title,
               (Vector2){(WIN_W-tsz.x)/2.f, 28}, 52, 2, TEXT_COLOR);

    /* Connection dot */
    Color connCol       = connected ? CONN_OK : CONN_ERR;
    const char *connLbl = connected ? "connected" : "disconnected";
    DrawCircle(WIN_W-22, 22, 8, connCol);
    Vector2 clsz = MeasureTextEx(font, connLbl, 15, 1);
    DrawTextEx(font, connLbl,
               (Vector2){WIN_W-22-clsz.x-6, 15}, 15, 1, connCol);

    /* Hostname label (top-left, small) */
    char hostLbl[140];
    snprintf(hostLbl, sizeof(hostLbl), "broker: %s", inputBuf);
    DrawTextEx(font, hostLbl, (Vector2){12, 15}, 15, 1, DIM_COLOR);

    /* Status banner */
    Color statusCol = TEXT_COLOR;
    if (strstr(status, "Player 1")) statusCol = X_COLOR;
    if (strstr(status, "Player 2")) statusCol = O_COLOR;
    if (strstr(status, "draw") || strstr(status, "wins"))
        statusCol = WIN_LINE_CLR;

    Vector2 ssz = MeasureTextEx(font, status, 26, 1);
    DrawTextEx(font, status,
               (Vector2){(WIN_W-ssz.x)/2.f, 106}, 26, 1, statusCol);

    /* Legend */
    DrawTextEx(font, "X  Player 1  (terminal)",
               (Vector2){BOARD_OFFSET_X, BOARD_OFFSET_Y-50}, 19, 1, X_COLOR);
    DrawTextEx(font, "O  Player 2  (MQTT)",
               (Vector2){WIN_W-BOARD_OFFSET_X-148, BOARD_OFFSET_Y-50},
               19, 1, O_COLOR);

    /* Board */
    draw_board(board, winA, winB, font);

    /* Footer — disconnect button */
    Rectangle btnRect = { WIN_W/2.f - 80, WIN_H - 52, 160, 36 };
    if (button(btnRect, "Disconnect", font, 18)) {
        mqtt_cleanup();
        gConnected = 0;
        /* reset board for next session */
        pthread_mutex_lock(&gMutex);
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                gBoard[i][j] = ' ';
        gWinCellA = gWinCellB = -1;
        strncpy(gStatus, "Waiting for game to start...", sizeof(gStatus)-1);
        pthread_mutex_unlock(&gMutex);
        screen = SCREEN_CONNECT;
    }
}

/* ─── Main ───────────────────────────────────────────────────────────────── */
int main(void)
{
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(WIN_W, WIN_H, "Tic-Tac-Toe  —  MQTT Monitor");
    SetTargetFPS(60);

    Font font = GetFontDefault();

    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            gBoard[i][j] = ' ';

    /* Pre-fill input with a sensible default */
    strncpy(inputBuf, "localhost", sizeof(inputBuf)-1);
    inputLen = (int)strlen(inputBuf);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BG_COLOR);

        /* Background grid */
        for (int gx = 0; gx < WIN_W; gx += 40)
            DrawLine(gx, 0, gx, WIN_H, (Color){30,30,50,55});
        for (int gy = 0; gy < WIN_H; gy += 40)
            DrawLine(0, gy, WIN_W, gy, (Color){30,30,50,55});

        switch (screen) {
            case SCREEN_CONNECT:    screen_connect(font);    break;
            case SCREEN_CONNECTING: screen_connecting(font); break;
            case SCREEN_GAME:       screen_game(font);       break;
        }

        EndDrawing();
    }

    mqtt_cleanup();
    CloseWindow();
    return 0;
}