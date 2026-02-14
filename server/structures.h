#ifndef STRUCTURES_H
#define STRUCTURES_H

#include "../common/models.h"
#include "../common/protocol.h"
#include <netinet/in.h>
#include <pthread.h>

typedef struct {
    int conn; // file relativo alla socket (connessione della sua rete)
    struct sockaddr_in addr; // indireizzo del cliente (IP)
    Player *player;
} Client;

typedef struct ClientNode {
    Client *val; 
    struct ClientNode *next;
} ClientNode;

extern ClientNode *clients;
extern short curr_clients_size;

// Mutex per thread safety
extern pthread_mutex_t clients_mutex;
extern pthread_mutex_t matches_mutex;

void broadcast_packet(ClientNode *head, Packet *packet, int except); // invia un pachetto a tutti i nodi
int get_socket_by_player_id(int player_id); // ricerca una socked di un player con id specifico
void remove_client_from_list(Client *client); //rimoszione di un cliente dalla lista

// Funzioni che usano un mutex per fare in modo che le aggiunte e rimozioni non danno problemi
void safe_add_match(Match *match);
void safe_remove_match(Match *match);
Match *safe_get_match_by_id(int id);

#endif