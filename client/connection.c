#include "connection.h"
#include "../common/protocol.h"
#include "../common/models.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int player_id = -1;

void handle_packet(int sockfd __attribute__((unused)), Packet *packet) {
    void *serialized = serialize_packet(packet);
    
    if(packet->id == SERVER_HANDSHAKE) {
        if(serialized != NULL) {
            Server_Handshake *response = (Server_Handshake *)serialized;
            player_id = response->player_id;
            if(DEBUG) {
                printf("%s Assegnato player_id=%d dal server\n", MSG_DEBUG, player_id);
            }
        }
    }
    
    if(packet->id == SERVER_SUCCESS) {
        printf("%s Operazione completata con successo\n", MSG_INFO);
    }
    
    if(packet->id == SERVER_ERROR) {
        printf("%s Errore dal server\n", MSG_ERROR);
    }
    
    if(packet->id == SERVER_BROADCASTMATCH) {
        if(serialized != NULL) {
            Server_BroadcastMatch *bc = (Server_BroadcastMatch *)serialized;
            printf("%s Nuova partita disponibile: #%d (creata da player #%d)\n", 
                   MSG_INFO, bc->match, bc->player_id);
        }
    }
    
    if(serialized != NULL) {
        free(serialized);
    }
}

void create_match(int sockfd) {
    Packet *packet = malloc(sizeof(Packet));
    packet->id = CLIENT_CREATEMATCH;
    packet->content = NULL;
    send_packet(sockfd, packet);
    free(packet);
    printf("%s Richiesta creazione partita inviata\n", MSG_DEBUG);
}

void join_match(int sockfd, int match_id) {
    Client_JoinMatch *join = malloc(sizeof(Client_JoinMatch));
    join->match = match_id;
    
    Packet *packet = malloc(sizeof(Packet));
    packet->id = CLIENT_JOINMATCH;
    packet->content = join;
    send_packet(sockfd, packet);
    
    free(packet);
    free(join);
    
    printf("%s Richiesta join partita #%d inviata\n", MSG_DEBUG, match_id);
}