#ifndef PROTOCOL_H
#define PROTOCOL_H

//è il cuore di come client e sever comunicanno fra di loro, definisce i pachetti scambiati



typedef struct { // struttuda del pacchetto 
    int id; // ogni pachetto deve avere un identifficativo
    int size; // una dimensione 
    void *content; //è un rifereimtno vero ad esso in caso di modificarlo
} Packet;

//Nota come codifica uso " Size litle Endian" , comodo perche si adatta ad architetture del codice 


//  PACCHETTI CLIENT -> SERVER (come in model gestisco i casi del pachetto)
#define CLIENT_HANDSHAKE        0 // ????????????
#define CLIENT_CREATEMATCH      1 // pachetto per creare una nuova pertita
#define CLIENT_JOINMATCH        3 // pachetto per entrare in una partitta esistetnte (farlo tramire richiesta)
#define CLIENT_MODIFYREQUEST    4 //pachetto per rsipondere ad una richiesta (implemntarloa dopo con Y / N)
#define CLIENT_MAKEMOVE         5 // pachetto per fare una mossa
#define CLIENT_PLAYAGAIN        6 // pachcchetto per chiedere di giocar ancora
#define CLIENT_QUITMATCH        7 // pachetto per uscire da un match


// Strutture relative ai pachetti del client -> sever

typedef struct { //struttura per il pachetto di join del match
    int match; // id della partita a cui si  vuole unire
} Client_JoinMatch;

typedef struct { //struttura per il pachetto di modifica della richiesta 
    int accepted; //accettazione
    int match; // ide della pertita
} Client_ModifyRequest;

typedef struct { // struttura per fare una mossa
    int moveX; //coridnata x
    int moveY; // cordinata y
    int match;
} Client_MakeMove;

typedef struct { // strutta per richiedere di giocare una ltra volta (per il caso di rivinicita)
    int choice; // scelta del giocatore 
    int match;
} Client_PlayAgain;

typedef struct { // struttura per uscire da una partita 
    int match;
} Client_QuitMatch;

//PACCHETTI SERVER -> CLIENT

#define SERVER_HANDSHAKE        20 //pacehtto handshake iniziale 
#define SERVER_SUCCESS          21 // pachetto per conferma del sucesso
#define SERVER_ERROR            22 // pachetto di errore 
#define SERVER_MATCHREQUEST     23 // pachetto di richiesta unirsi alla patita
#define SERVER_NOTICESTATE      24 // pahcetto per indicare lo stato della partita 
#define SERVER_NOTICEMOVE       25 // pachetto per indicare la mossa dell'avversario
#define SERVER_BROADCASTMATCH   26 // pachetto per brodcaet della nuova parita
#define SERVER_UPDATEONREQUEST  30 // pachetto per aggionare le richieste di partecipazione
#define SERVER_INVALID_MOVE     31 // pachetto per indicare una mossa non valida

typedef struct { // stuttura per il pachetto Handshake
    int player_id; // id assegnato al player
} Server_Handshake;

typedef struct { //struttura per il pachetto di richiesta della partita
    int other_player; // id per chiedere di unirsi alla pertita 
    int match;
} Server_MatchRequest;

typedef struct { // struttura per aggiornare la richiesta di partecipazione 
    int accepted;
    int match;
} Server_UpdateOnRequest;

typedef struct { //strutturea per notificare lo stato della partita 
    int state;
    int match;
} Server_NoticeState;

typedef struct { // struttura per noificare la mossa dell'avversario
    int moveX;
    int moveY;
    int match;
} Server_NoticeMove;

typedef struct { // struttura per brodcast della nuova partita
    int player_id;
    int match;
} Server_BroadcastMatch;

// Funzioni da implemntare 
void send_packet(int sockfd, Packet *packet); // funzione di invio del pachetto
void *serialize_packet(Packet *packet); // funzione per seralizare 

#endif