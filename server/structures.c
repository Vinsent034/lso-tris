#include "structures.h"
#include <stdio.h>

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