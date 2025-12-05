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
    int id;
    int busy;
} Player;

typedef struct RequestNode {
    Player *requester;
    struct RequestNode *next;
} RequestNode;

typedef struct {
    Player *participants[2];
    RequestNode *requests_head;
    RequestNode *requests_tail;
    char grid[3][3];
    int free_slots;
    int state;
    int play_again_counter;
    Player *play_again[2];
    int id;
} Match;

typedef struct MatchList {
    Match *val;
    struct MatchList *next;
} MatchList;

extern MatchList *matches;
extern short curr_matches_size;

void add_match(Match *match);
void remove_match(Match *match);
Match *get_match_by_id(int id);
int find_free_id();
void add_requester(Match *match, RequestNode *node);
void delete_from_head(Match *match);

#endif