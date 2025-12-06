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

#endif