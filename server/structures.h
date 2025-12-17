#ifndef STRUCTURES_H
#define STRUCTURES_H

#include "../common/models.h"
#include "../common/protocol.h"
#include <netinet/in.h>
#include <pthread.h>

typedef struct {
    int conn; //serve a indicare la scoket per comuncare col client
    struct sockaddr_in addr; // indirizzo di rete del client (IP + porta)
    Player *player; // un puntatore al gicocatore assciato a questo client
} Client;

typedef struct ClientNode {
    Client *val;
    struct ClientNode *next;
} ClientNode; // E una struttura che tuebe traccia di tutti i cllienti conessi al server, fatta come lista

extern ClientNode *clients;
extern short curr_clients_size;

// Mutex per thread safety
extern pthread_mutex_t clients_mutex;
extern pthread_mutex_t matches_mutex;

void broadcast_packet(ClientNode *head, Packet *packet, int except);
int get_socket_by_player_id(int player_id);
void remove_client_from_list(Client *client);

// Thread-safe wrappers per operazioni su matches
void safe_add_match(Match *match);
void safe_remove_match(Match *match);
Match *safe_get_match_by_id(int id);

#endif