#include "structures.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

ClientNode *clients = NULL;
short curr_clients_size = 0;

// Mutex globali per thread safety
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t matches_mutex = PTHREAD_MUTEX_INITIALIZER;



//invia un pachetti escluso una socket
void broadcast_packet(ClientNode *head, Packet *packet, int except) {  //except e la socket da escludere
    pthread_mutex_lock(&clients_mutex); // blocco il mutex per i clients
    ClientNode *current = head;
    while(current != NULL) {
        if(current->val->conn != except) { // invio a tutti tranne che a quello da escludere
            send_packet(current->val->conn, packet);// invio il pacchetto
        }
        current = current->next;
    }
    pthread_mutex_unlock(&clients_mutex);
}









//funzione che restitutisce la socket di un giocatore preso il suo id
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











// Thread-safe wrappers per operazioni su matches
void safe_add_match(Match *match) {
    pthread_mutex_lock(&matches_mutex);
    add_match(match);
    pthread_mutex_unlock(&matches_mutex);
}


// Thread-safe wrappers per operazioni su matches
void safe_remove_match(Match *match) {
    pthread_mutex_lock(&matches_mutex);
    remove_match(match);
    pthread_mutex_unlock(&matches_mutex);
}
// Thread-safe wrappers per operazioni su matches
Match *safe_get_match_by_id(int id) {
    pthread_mutex_lock(&matches_mutex);
    Match *result = get_match_by_id(matches, id);
    pthread_mutex_unlock(&matches_mutex);
    return result;
}