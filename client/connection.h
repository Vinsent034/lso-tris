#ifndef CLIENT_CONNECTION_H
#define CLIENT_CONNECTION_H

#include "../common/protocol.h"

extern int player_id;

void handle_packet(int sockfd, Packet *packet);
void create_match(int sockfd);
void join_match(int sockfd, int match_id);
void make_move(int sockfd, int match_id, int x, int y);
void respond_to_request(int sockfd, int accepted);
void play_again(int sockfd, int match_id, int choice);
void quit_match(int sockfd, int match_id);
void print_grid();

// Game state tracking
extern char client_grid[3][3];
extern int current_match_id;
extern int current_state;
extern int match_ended;
extern int am_i_player1;  // 1 se sono Player1 (X), 0 se sono Player2 (O)
extern int my_turn_flag;  // 1 quando è il mio turno e devo giocare
extern int clear_stdin_flag;  // 1 se dobbiamo pulire stdin prima di leggere

// Pending match request
extern int pending_request_player;
extern int pending_request_match;

#endif