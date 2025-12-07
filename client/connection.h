#ifndef CLIENT_CONNECTION_H
#define CLIENT_CONNECTION_H

#include "../common/protocol.h"

extern int player_id;

void handle_packet(int sockfd, Packet *packet);
void create_match(int sockfd);
void join_match(int sockfd, int match_id);
void make_move(int sockfd, int match_id, int x, int y);
void respond_to_request(int sockfd, int accepted);

// Game state tracking
extern char client_grid[3][3];
extern int current_match_id;
extern int current_state;

// Pending match request
extern int pending_request_player;
extern int pending_request_match;

#endif