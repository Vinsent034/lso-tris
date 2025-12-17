#ifndef MODELS_H
#define MODELS_H

// sono delle colaorazioni che ho agigunto io per capire meglio di che tipo di errore si trattava , dato ceh non capisco se sono errori o solo warming in casi particolari
#define MSG_INFO    "[\x1b[34mINFO\x1b[0m]" 
#define MSG_WARNING "[\x1b[33mWARNING\x1b[0m]"
#define MSG_ERROR   "[\x1b[31mERROR\x1b[0m]"
#define MSG_DEBUG   "[\x1b[32mDEBUG\x1b[0m]"

#define DEBUG 1
#define MAX_MATCHES 255
#define MAX_CLIENTS 255




//definisco tutti gli stati possibili di una pertita
#define STATE_TERMINATED    0
#define STATE_INPROGRESS    1 // inidcatore dello stato del gioco
#define STATE_WAITING       2 // stato di attesa per finche l'avversario non termina
#define STATE_CREATED       3
#define STATE_TURN_PLAYER1  4
#define STATE_TURN_PLAYER2  5
#define STATE_WIN           6
#define STATE_LOSE          7
#define STATE_DRAW          8 //caso del pareggio

typedef struct {  // strutture del giocatore deve avere solo un id e indicatore per dire se è attivo
    int id;
    int busy; // falg per indicare se e libero 
} Player;

typedef struct RequestNode { //Struct usate per gestire le diverse richieste di partecipazione alla partita
    Player *requester; // puntatore per tenere traccia del giocatore che richiede la perita
    struct RequestNode *next; // puntatore per il nodo sucessivo della richiesta 
} RequestNode;

typedef struct { // struttura della oartia
    Player *participants[2]; //array di due puntotreu per i due giocatori che giocano
    RequestNode *requests_head; //puntatore per richiesta in testa
    RequestNode *requests_tail; // puntatore per richiesta in coda
    char grid[3][3]; // griglia oer la partita
    int free_slots; //indicarore per gli spazzi libri nella cella della matrice
    int state; // satato della partita
    int play_again_counter; //contatore del numero di giocatori che vorranno giocare dopo
    Player *play_again[2]; //array per tenre traccia dei giocatori che vogliono giocare annocra (rivinicita)
    int id; // ide della partita
} Match;

typedef struct MatchList { // struttura percollegare le partite trmute lista 
    Match *val; //puntatore alla partita
    struct MatchList *next; //puntatore per la partita sucessiva
} MatchList;

extern MatchList *matches; //puntatore alla lista per le partite attive
extern short curr_matches_size; //numero di partite attive 




// funzioni da implementare domani matitna con calma 
void add_match(Match *match); // fuznione per aggiungere una partita alla lista
void remove_match(Match *match); // funzione per rimuovere una paritta dalla lista
Match *get_match_by_id(MatchList *head, int id); // funzuone per entrare in una petita con id , ad esempio quando 
int find_free_id();
void add_requester(Match *match, RequestNode *node);
void delete_from_head(Match *match);

#endif