#ifndef CLIENT_CONNECTION_H
#define CLIENT_CONNECTION_H

#include "../common/protocol.h"

extern int player_id;

void handle_packet(int sockfd, Packet *packet);
void create_match(int sockfd);
void join_match(int sockfd, int match_id);

#endif