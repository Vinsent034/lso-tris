#ifndef MODELS_H
#define MODELS_H

#define MSG_INFO    "[\x1b[34mINFO\x1b[0m]"
#define MSG_WARNING "[\x1b[33mWARNING\x1b[0m]"
#define MSG_ERROR   "[\x1b[31mERROR\x1b[0m]"
#define MSG_DEBUG   "[\x1b[32mDEBUG\x1b[0m]"

#define DEBUG 1
#define MAX_MATCHES 255
#define MAX_CLIENTS 255

#define STATE_TERMINATED    0
#define STATE_INPROGRESS    1
#define STATE_WAITING       2
#define STATE_CREATED       3
#define STATE_TURN_PLAYER1  4
#define STATE_TURN_PLAYER2  5
#define STATE_WIN           6
#define STATE_LOSE          7
#define STATE_DRAW          8

typedef struct {
    int id;                        // ID univoco assegnato dal server
    int busy;                      // 1 = impegnato in partita, 0 = libero
} Player;

typedef struct RequestNode {
    Player *requester;             // Player che ha fatto richiesta di join
    struct RequestNode *next;      // Puntatore al prossimo in coda
} RequestNode;

typedef struct {
    Player *participants[2];       // [0]=Player1(X), [1]=Player2(O)
    RequestNode *requests_head;    // Testa della coda FIFO richieste
    RequestNode *requests_tail;    // Coda della coda FIFO richieste
    char grid[3][3];               // Griglia di gioco (0=libera, 'X', 'O')
    int free_slots;                // Celle libere (inizia a 9)
    int state;                     // Stato corrente (STATE_*)
    int play_again_counter;        // Quanti player vogliono rigiocare
    Player *play_again[2];         // Giocatori che vogliono rigiocare
    int id;                        // ID univoco della partita
    int winner_index;              // 0=player1 vince, 1=player2 vince, -1=pareggio
} Match;

typedef struct MatchList {
    Match *val;                    // Puntatore alla partita
    struct MatchList *next;        // Prossimo nodo della lista
} MatchList;

extern MatchList *matches;         // Lista globale delle partite
extern short curr_matches_size;    // Numero attuale di partite

void add_match(Match *match);                          // Aggiunge una partita alla lista
void remove_match(Match *match);                       // Rimuove una partita dalla lista
Match *get_match_by_id(MatchList *head, int id);       // Cerca partita per ID
int find_free_id();                                    // Trova un ID libero
void add_requester(Match *match, RequestNode *node);   // Aggiunge richiesta di join
void delete_from_head(Match *match);                   // Rimuove la prima richiesta dalla coda

#endif