#ifndef PROTOCOL_H
#define PROTOCOL_H

typedef struct {
    int id;
    int size;
    void *content;
} Packet;

// ===== PACCHETTI CLIENT -> SERVER =====
#define CLIENT_HANDSHAKE        0
#define CLIENT_CREATEMATCH      1
#define CLIENT_JOINMATCH        3
#define CLIENT_MODIFYREQUEST    4
#define CLIENT_MAKEMOVE         5
#define CLIENT_PLAYAGAIN        6
#define CLIENT_QUITMATCH        7

typedef struct {
    int match;
} Client_JoinMatch;

typedef struct {
    int accepted;
    int match;
} Client_ModifyRequest;

typedef struct {
    int moveX;
    int moveY;
    int match;
} Client_MakeMove;

typedef struct {
    int choice;
    int match;
} Client_PlayAgain;

typedef struct {
    int match;
} Client_QuitMatch;

// ===== PACCHETTI SERVER -> CLIENT =====
#define SERVER_HANDSHAKE        20
#define SERVER_SUCCESS          21
#define SERVER_ERROR            22
#define SERVER_MATCHREQUEST     23
#define SERVER_NOTICESTATE      24
#define SERVER_NOTICEMOVE       25
#define SERVER_BROADCASTMATCH   26
#define SERVER_UPDATEONREQUEST  30
#define SERVER_INVALID_MOVE     31

typedef struct {
    int player_id;
} Server_Handshake;

typedef struct {
    int other_player;
    int match;
} Server_MatchRequest;

typedef struct {
    int accepted;
    int match;
} Server_UpdateOnRequest;

typedef struct {
    int state;
    int match;
} Server_NoticeState;

typedef struct {
    int moveX;
    int moveY;
    int match;
} Server_NoticeMove;

typedef struct {
    int player_id;
    int match;
} Server_BroadcastMatch;

// Funzioni
void send_packet(int sockfd, Packet *packet);
void *serialize_packet(Packet *packet);

#endif