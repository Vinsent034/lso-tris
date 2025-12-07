#ifndef SERVER_CONNECTION_H
#define SERVER_CONNECTION_H

#include "structures.h"
#include "../common/protocol.h"
#include <pthread.h>

typedef struct {
    Client *client;
    pthread_t thread;
} JoinerThreadArgs;

void *server_thread(void *args);
void *joiner_thread(void *args);
void handle_packet(Client *client, Packet *packet);

// Game logic helpers
int check_winner(Match *match);
int is_board_full(Match *match);
void end_match(Match *match, int winner_index);

#endif