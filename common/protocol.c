#include "protocol.h"
#include "models.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <stdlib.h>

void send_packet(int sockfd, Packet *packet) {
    // Validazione parametri
    if(packet == NULL) {
        fprintf(stderr, "%s send_packet: packet è NULL\n", MSG_ERROR);
        return;
    }

    char serialized[65536 + 3]; // 65536 sono 2 byte , i 3 sono byte per header , codifica litle endian, byte 0 contine packet -> Id, 1 e 2 dimensioni in litle endian
    serialized[0] = packet->id;
    
    // ===== SERVER PACKETS =====
    if(packet->id == SERVER_HANDSHAKE) {
        packet->size = 1;
        serialized[3] = ((Server_Handshake *)packet->content)->player_id;
    }
    
    if(packet->id == SERVER_SUCCESS || packet->id == SERVER_ERROR || packet->id == SERVER_INVALID_MOVE) {
        packet->size = 0;
    }
    
    if(packet->id == SERVER_MATCHREQUEST) {
        packet->size = 2;
        serialized[3] = ((Server_MatchRequest *)packet->content)->other_player;
        serialized[4] = ((Server_MatchRequest *)packet->content)->match;
    }
    
    if(packet->id == SERVER_NOTICESTATE) {
        packet->size = 2;
        serialized[3] = ((Server_NoticeState *)packet->content)->state;
        serialized[4] = ((Server_NoticeState *)packet->content)->match;
    }
    
    if(packet->id == SERVER_NOTICEMOVE) {
        packet->size = 3;
        serialized[3] = ((Server_NoticeMove *)packet->content)->moveX;
        serialized[4] = ((Server_NoticeMove *)packet->content)->moveY;
        serialized[5] = ((Server_NoticeMove *)packet->content)->match;
    }
    
    if(packet->id == SERVER_BROADCASTMATCH) {
        packet->size = 2;
        serialized[3] = ((Server_BroadcastMatch *)packet->content)->player_id;
        serialized[4] = ((Server_BroadcastMatch *)packet->content)->match;
    }
    
    if(packet->id == SERVER_UPDATEONREQUEST) {
        packet->size = 2;
        serialized[3] = ((Server_UpdateOnRequest *)packet->content)->accepted;
        serialized[4] = ((Server_UpdateOnRequest *)packet->content)->match;
    }
    
    // ===== CLIENT PACKETS =====
    if(packet->id == CLIENT_HANDSHAKE || packet->id == CLIENT_CREATEMATCH) {
        packet->size = 0;
    }
    
    if(packet->id == CLIENT_JOINMATCH) {
        packet->size = 1;
        serialized[3] = ((Client_JoinMatch *)packet->content)->match;
    }
    
    if(packet->id == CLIENT_MODIFYREQUEST) {
        packet->size = 2;
        serialized[3] = ((Client_ModifyRequest *)packet->content)->accepted;
        serialized[4] = ((Client_ModifyRequest *)packet->content)->match;
    }
    
    if(packet->id == CLIENT_MAKEMOVE) {
        packet->size = 3;
        serialized[3] = ((Client_MakeMove *)packet->content)->moveX;
        serialized[4] = ((Client_MakeMove *)packet->content)->moveY;
        serialized[5] = ((Client_MakeMove *)packet->content)->match;
    }
    
    if(packet->id == CLIENT_PLAYAGAIN) {
        packet->size = 2;
        serialized[3] = ((Client_PlayAgain *)packet->content)->choice;
        serialized[4] = ((Client_PlayAgain *)packet->content)->match;
    }

    if(packet->id == CLIENT_QUITMATCH) {
        packet->size = 1;
        serialized[3] = ((Client_QuitMatch *)packet->content)->match;
    }

    // Little endian size
    serialized[1] = packet->size & 0xFF;
    serialized[2] = (packet->size >> 8) & 0xFF;

    // Invio con gestione di send() parziale
    size_t total_bytes = packet->size + 3;
    size_t sent = 0;

    while(sent < total_bytes) {
        ssize_t n = send(sockfd, serialized + sent, total_bytes - sent, MSG_NOSIGNAL);
        if(n < 0) {
            fprintf(stderr, "%s Errore invio pacchetto (fd=%d, id=%d): %s\n",
                    MSG_ERROR, sockfd, packet->id, strerror(errno));
            return;
        }
        if(n == 0) {
            fprintf(stderr, "%s Connessione chiusa durante invio (fd=%d)\n",
                    MSG_ERROR, sockfd);
            return;
        }
        sent += n;
    }
}











// prende un pachetto e converte i byte ricevuti dalla rete in una struttura C allocata in memoria 
//in base all'id  alloca la struttyra e copia i byte nei campi giusti 
// ritorna un puntatore void alla struttura deserializzata

void *serialize_packet(Packet *packet) {
    // Validazione generale: packet non NULL e content non NULL per pacchetti con size > 0
    if(packet == NULL) return NULL;
    if(packet->size > 0 && packet->content == NULL) return NULL;

    // ===== CLIENT PACKETS =====
    if(packet->id == CLIENT_JOINMATCH) {
        if(packet->size < 1) return NULL;
        Client_JoinMatch *new = malloc(sizeof(Client_JoinMatch));
        new->match = ((char *)packet->content)[0];
        return new;
    }
    
    if(packet->id == CLIENT_MODIFYREQUEST) {
        if(packet->size < 2) return NULL;
        Client_ModifyRequest *new = malloc(sizeof(Client_ModifyRequest));
        new->accepted = ((char *)packet->content)[0];
        new->match = ((char *)packet->content)[1];
        return new;
    }
    
    if(packet->id == CLIENT_MAKEMOVE) {
        if(packet->size < 3) return NULL;
        Client_MakeMove *new = malloc(sizeof(Client_MakeMove));
        new->moveX = ((char *)packet->content)[0];
        new->moveY = ((char *)packet->content)[1];
        new->match = ((char *)packet->content)[2];
        return new;
    }
    
    if(packet->id == CLIENT_PLAYAGAIN) {
        if(packet->size < 2) return NULL;
        Client_PlayAgain *new = malloc(sizeof(Client_PlayAgain));
        new->choice = ((char *)packet->content)[0];
        new->match = ((char *)packet->content)[1];
        return new;
    }

    if(packet->id == CLIENT_QUITMATCH) {
        if(packet->size < 1) return NULL;
        Client_QuitMatch *new = malloc(sizeof(Client_QuitMatch));
        new->match = ((char *)packet->content)[0];
        return new;
    }

    // ===== SERVER PACKETS =====
    if(packet->id == SERVER_HANDSHAKE) {
        if(packet->size < 1) return NULL;
        Server_Handshake *new = malloc(sizeof(Server_Handshake));
        new->player_id = ((char *)packet->content)[0];
        return new;
    }
    
    if(packet->id == SERVER_MATCHREQUEST) {
        if(packet->size < 2) return NULL;
        Server_MatchRequest *new = malloc(sizeof(Server_MatchRequest));
        new->other_player = ((char *)packet->content)[0];
        new->match = ((char *)packet->content)[1];
        return new;
    }
    
    if(packet->id == SERVER_UPDATEONREQUEST) {
        if(packet->size < 2) return NULL;
        Server_UpdateOnRequest *new = malloc(sizeof(Server_UpdateOnRequest));
        new->accepted = ((char *)packet->content)[0];
        new->match = ((char *)packet->content)[1];
        return new;
    }
    
    if(packet->id == SERVER_NOTICESTATE) {
        if(packet->size < 2) return NULL;
        Server_NoticeState *new = malloc(sizeof(Server_NoticeState));
        new->state = ((char *)packet->content)[0];
        new->match = ((char *)packet->content)[1];
        return new;
    }
    
    if(packet->id == SERVER_NOTICEMOVE) {
        if(packet->size < 3) return NULL;
        Server_NoticeMove *new = malloc(sizeof(Server_NoticeMove));
        new->moveX = ((char *)packet->content)[0];
        new->moveY = ((char *)packet->content)[1];
        new->match = ((char *)packet->content)[2];
        return new;
    }
    
    if(packet->id == SERVER_BROADCASTMATCH) {
        if(packet->size < 2) return NULL;
        Server_BroadcastMatch *new = malloc(sizeof(Server_BroadcastMatch));
        new->player_id = ((char *)packet->content)[0];
        new->match = ((char *)packet->content)[1];
        return new;
    }
    
    return NULL;
}















