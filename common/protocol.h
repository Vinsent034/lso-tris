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
} Client_JoinMatch; // avvisa il giocatore di volere entrare per una aprtita

typedef struct {
    int accepted;
    int match;
} Client_ModifyRequest; // qundo un giocatore riceve una richiesta di join , puo accetarla o rifiutarla 

typedef struct {
    int moveX;
    int moveY;
    int match;
} Client_MakeMove; //Il client invia le coordinate della cella dove vuole piazzare il proprio simbolo

typedef struct {
    int choice;
    int match;
} Client_PlayAgain; //A fine partita, indica se il giocatore vuole fare un'altra partita

typedef struct {
    int match;
} Client_QuitMatch; // Il client comunica al server che vuole uscire dalla partita

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
} Server_Handshake; // Il server assegna un ID univoco al client connesso

typedef struct {
    int other_player;
    int match;
} Server_MatchRequest; //  Il server chiede al creatore della partita se accetta l'avversario

typedef struct {
    int accepted;
    int match;
} Server_UpdateOnRequest; // Informa il giocatore richiedente se la sua richiesta è stata accettata o rifiutata

typedef struct {
    int state;
    int match;
} Server_NoticeState; // Notifica cambiamenti di stato (inizio, vittoria, pareggio, sconfitta)

typedef struct {
    int moveX;
    int moveY;
    int match;
} Server_NoticeMove; // Sincronizzazione dello stato della griglia tra i due giocatori

typedef struct {
    int player_id;
    int match;
} Server_BroadcastMatch; // Il server informa tutti i client connessi delle partite a cui possono unirsi

// Funzioni
void send_packet(int sockfd, Packet *packet);
void *serialize_packet(Packet *packet);

#endif