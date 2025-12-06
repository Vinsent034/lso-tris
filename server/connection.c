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
    int player_id = client->conn % 255;
    Player *player = client->player;
    
    void *serialized = serialize_packet(packet);
    
    if(DEBUG) {
        printf("%s Packet(fd=%d, id=%d, size=%d)\n", 
               MSG_DEBUG, player_id, packet->id, packet->size);
    }
    
    // ===== HANDSHAKE =====
    if(packet->id == CLIENT_HANDSHAKE) {
        if(client->player->id == -1) {
            player->id = player_id;
        }
        
        Server_Handshake *handshake = malloc(sizeof(Server_Handshake));
        handshake->player_id = player_id;
        
        Packet *response = malloc(sizeof(Packet));
        response->id = SERVER_HANDSHAKE;
        response->content = handshake;
        
        send_packet(client->conn, response);
        
        printf("%s Assegnato player_id=%d al client fd=%d\n", 
               MSG_INFO, player_id, client->conn);
        
        free(response);
        free(handshake);
        
        if(serialized != NULL) free(serialized);
        return;
    }
    
    // Si deve passare prima per l'handshake per i comandi successivi
    if(client->player->id == -1) {
        printf("%s Client fd=%d non ha fatto handshake\n", MSG_WARNING, client->conn);
        if(serialized != NULL) free(serialized);
        return;
    }
    
    // ===== CREATE MATCH =====
    if(packet->id == CLIENT_CREATEMATCH) {
        printf("%s DEBUG: Ricevuto CLIENT_CREATEMATCH da player_id=%d\n", MSG_DEBUG, player_id);
        
        if(curr_matches_size < MAX_MATCHES) {
            Match *new_match = malloc(sizeof(Match));
            new_match->participants[0] = player;
            new_match->participants[1] = NULL;
            new_match->requests_head = NULL;
            new_match->requests_tail = NULL;
            new_match->state = STATE_CREATED;
            new_match->free_slots = 9;
            new_match->play_again_counter = 0;
            new_match->id = find_free_id();
            memset(new_match->grid, 0, sizeof(new_match->grid[0][0]) * 9);
            
            add_match(new_match);
            
            printf("%s Player id=%d ha creato partita #%d\n", 
                   MSG_INFO, player_id, new_match->id);
            
            // Invia SUCCESS al creatore
            Packet *success = malloc(sizeof(Packet));
            success->id = SERVER_SUCCESS;
            success->content = NULL;
            send_packet(client->conn, success);
            free(success);
            
            // Broadcast a tutti i client
            Server_BroadcastMatch *broadcast = malloc(sizeof(Server_BroadcastMatch));
            broadcast->player_id = player_id;
            broadcast->match = new_match->id;
            
            Packet *bc_packet = malloc(sizeof(Packet));
            bc_packet->id = SERVER_BROADCASTMATCH;
            bc_packet->content = broadcast;
            
            broadcast_packet(clients, bc_packet, player_id);
            
            free(bc_packet);
            free(broadcast);
        } else {
            printf("%s Limite massimo partite raggiunto\n", MSG_WARNING);
            
            Packet *error = malloc(sizeof(Packet));
            error->id = SERVER_ERROR;
            error->content = NULL;
            send_packet(client->conn, error);
            free(error);
        }
    }
    
    if(serialized != NULL) {
        free(serialized);
    }
}

void *server_thread(void *args) {
    Client *client = (Client *)args;
    char buffer[BUFFER_SIZE];
    size_t total = 0;
    
    while(1) {
        ssize_t received = recv(client->conn, buffer, BUFFER_SIZE, 0);
        if(received <= 0) {
            break;
        }
        
        total = received;
        while(total > 0) {
            char *block = buffer + (received - total);
            Packet *packet = malloc(sizeof(Packet));
            packet->id = block[0];
            packet->size = block[1] + (block[2] << 8);
            packet->content = malloc(sizeof(char) * packet->size);
            memcpy(packet->content, block + 3, packet->size);
            
            handle_packet(client, packet);
            
            total -= packet->size + 3;
            free(packet->content);
            free(packet);
        }
    }
    
    return NULL;
}