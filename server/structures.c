#include "structures.h"
#include <stdio.h>
#include <stdlib.h>

ClientNode *clients = NULL;
short curr_clients_size = 0;

void broadcast_packet(ClientNode *head, Packet *packet, int except) {
    ClientNode *current = head;
    while(current != NULL) {
        if(current->val->conn != except) {
            send_packet(current->val->conn, packet);
        }
        current = current->next;
    }
}

int get_socket_by_player_id(int player_id) {
    ClientNode *current = clients;
    while(current != NULL) {
        if(current->val->player != NULL && current->val->player->id == player_id) {
            return current->val->conn;
        }
        current = current->next;
    }
    return -1; // Not found
}

void remove_client_from_list(Client *client) {
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
            return;
        }
        prev = current;
        current = current->next;
    }
}