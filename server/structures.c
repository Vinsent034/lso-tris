#include "structures.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

ClientNode *clients = NULL;
short curr_clients_size = 0;

// Mutex globali per thread safety
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t matches_mutex = PTHREAD_MUTEX_INITIALIZER;

//invia un pachetto a tutti i client tranne quello ,  in maniere sicura col mutex
// Non invia ai player busy (in partita attiva)
void broadcast_packet(ClientNode *head, Packet *packet, int except) {
    pthread_mutex_lock(&clients_mutex);
    ClientNode *current = head;
    while(current != NULL) {
        if(current->val->conn != except &&
           current->val->player != NULL &&
           current->val->player->busy == 0) {
            send_packet(current->val->conn, packet);
        }
        current = current->next;
    }
    pthread_mutex_unlock(&clients_mutex);
}

// funzione che permette la socket del palyer tramite il suo id
int get_socket_by_player_id(int player_id) {
    pthread_mutex_lock(&clients_mutex);
    ClientNode *current = clients;
    int result = -1;
    while(current != NULL) {
        if(current->val->player != NULL && current->val->player->id == player_id) {
            result = current->val->conn;
            break;
        }
        current = current->next;
    }
    pthread_mutex_unlock(&clients_mutex);
    return result;
}


// rimozione di un cliente nella lista 
void remove_client_from_list(Client *client) {
    pthread_mutex_lock(&clients_mutex);
    ClientNode *current = clients;
    ClientNode *prev = NULL;

    while(current != NULL) {
        if(current->val == client) {
            if(prev == NULL) {
                // Rimuovere dalla testa
                clients = current->next;
            } else {
                prev->next = current->next;
            }
            free(current);
            pthread_mutex_unlock(&clients_mutex);
            return;
        }
        prev = current;
        current = current->next;
    }
    pthread_mutex_unlock(&clients_mutex);
}

// Thread-safe wrappers per operazioni su matches, in questo caso aggiunta
void safe_add_match(Match *match) {
    pthread_mutex_lock(&matches_mutex);
    add_match(match);
    pthread_mutex_unlock(&matches_mutex);
}


// funzione per rimuovere un match in sicurezza
void safe_remove_match(Match *match) {
    pthread_mutex_lock(&matches_mutex);
    remove_match(match);
    pthread_mutex_unlock(&matches_mutex);
}

// funzione per ritorna un match prendendo il suo id
Match *safe_get_match_by_id(int id) {
    pthread_mutex_lock(&matches_mutex);
    Match *result = get_match_by_id(matches, id);
    pthread_mutex_unlock(&matches_mutex);
    return result;
}