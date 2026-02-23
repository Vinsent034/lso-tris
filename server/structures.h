#ifndef STRUCTURES_H
#define STRUCTURES_H

#include "../common/models.h"
#include "../common/protocol.h"
#include <netinet/in.h>
#include <pthread.h>

typedef struct {
    int conn;                      // File descriptor della socket TCP
    struct sockaddr_in addr;       // Indirizzo IP del client
    Player *player;                // Puntatore al Player associato
} Client;

typedef struct ClientNode {
    Client *val;                   // Dati del client
    struct ClientNode *next;       // Prossimo nodo della lista
} ClientNode;

extern ClientNode *clients;        // Lista globale dei client connessi
extern short curr_clients_size;    // Numero attuale di client connessi

// Mutex globali (structures.c)
extern pthread_mutex_t clients_mutex;  // Mutex per accesso alla lista client
extern pthread_mutex_t matches_mutex;  // Mutex per accesso alla lista partite

void broadcast_packet(ClientNode *head, Packet *packet, int except); // Invia un pacchetto a tutti i client
int get_socket_by_player_id(int player_id);                          // Cerca la socket di un player per ID
void remove_client_from_list(Client *client);                        // Rimuove un client dalla lista

// Funzioni thread-safe per gestione partite (usano matches_mutex)
void safe_add_match(Match *match);         // Aggiunge una partita in modo thread-safe
void safe_remove_match(Match *match);      // Rimuove una partita in modo thread-safe
Match *safe_get_match_by_id(int id);       // Cerca una partita per ID in modo thread-safe

#endif