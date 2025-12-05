#ifndef PROTOCOL_H
#define PROTOCOL_H

typedef struct {
    int id;
    int size;
    void *content;
} Packet;

#define CLIENT_HANDSHAKE        0
#define CLIENT_CREATEMATCH      1
#define CLIENT_JOINMATCH        3
#define CLIENT_MODIFYREQUEST    4
#define CLIENT_MAKEMOVE         5
#define CLIENT_PLAYAGAIN        6

#define SERVER_HANDSHAKE        20
#define SERVER_SUCCESS          21
#define SERVER_ERROR            22
#define SERVER_MATCHREQUEST     23
#define SERVER_NOTICESTATE      24
#define SERVER_NOTICEMOVE       25
#define SERVER_BROADCASTMATCH   26
#define SERVER_UPDATEONREQUEST  30

void send_packet(int sockfd, Packet *packet);
void *serialize_packet(Packet *packet);

#endif