#include "connection.h"
#include "../common/models.h"
#include "../common/protocol.h"
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#define BUFFER_SIZE 1024

void *joiner_thread(void *args) {
    JoinerThreadArgs *thread_args = (JoinerThreadArgs*)args;
    pthread_join(thread_args->thread, NULL);
    
    printf("%s Connessione chiusa (fd=%d)\n", MSG_INFO, thread_args->client->conn);
    
    // TODO: rimuovere client dalla lista
    curr_clients_size--;
    
    free(thread_args->client->player);
    free(thread_args->client);
    free(thread_args);
    
    return NULL;
}

void handle_packet(Client *client, Packet *packet) {
    printf("%s Ricevuto pacchetto (fd=%d, id=%d, size=%d)\n", 
           MSG_DEBUG, client->conn, packet->id, packet->size);
    // TODO: implementare gestione pacchetti domani
}

void *server_thread(void *args) {
    Client *client = (Client *)args;
    char buffer[BUFFER_SIZE];
    
    while(1) {
        ssize_t received = recv(client->conn, buffer, BUFFER_SIZE, 0);
        if(received <= 0) {
            break;
        }
        
        // TODO: parsing pacchetti domani
        printf("%s Ricevuti %ld bytes da fd=%d\n", MSG_DEBUG, received, client->conn);
    }
    
    return NULL;
}