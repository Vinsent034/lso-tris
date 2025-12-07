#ifndef STRUCTURES_H
#define STRUCTURES_H

#include "../common/models.h"
#include "../common/protocol.h"
#include <netinet/in.h>

typedef struct {
    int conn;
    struct sockaddr_in addr;
    Player *player;
} Client;

typedef struct ClientNode {
    Client *val;
    struct ClientNode *next;
} ClientNode;

extern ClientNode *clients;
extern short curr_clients_size;

void broadcast_packet(ClientNode *head, Packet *packet, int except);
int get_socket_by_player_id(int player_id);
void remove_client_from_list(Client *client);

#endif