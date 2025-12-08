#include "connection.h"
#include "../common/protocol.h"
#include "../common/models.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int player_id = -1;
char client_grid[3][3];
int current_match_id = -1;
int current_state = -1;
int match_ended = 0;
int am_i_player1 = -1;
int my_turn_flag = 0;
int clear_stdin_flag = 0;
int pending_request_player = -1;
int pending_request_match = -1;

void handle_packet(int sockfd, Packet *packet) {
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

    if(packet->id == SERVER_INVALID_MOVE) {
        printf("\n%s ================================\n", MSG_ERROR);
        printf("%s COORDINATA GIÀ OCCUPATA!\n", MSG_ERROR);
        printf("%s ================================\n", MSG_ERROR);
        printf("%s Scegli un'altra casella\n\n", MSG_INFO);

        // Mostra la griglia corrente
        printf("     0   1   2\n");
        printf("   +---+---+---+\n");
        for(int i = 0; i < 3; i++) {
            printf(" %d |", i);
            for(int j = 0; j < 3; j++) {
                char c = client_grid[i][j];
                if(c == 0) c = ' ';
                printf(" %c |", c);
            }
            printf("\n   +---+---+---+\n");
        }

        // Riabilita my_turn_flag per chiedere di nuovo le coordinate
        my_turn_flag = 1;
        // Non pulire stdin - l'utente deve inserire nuove coordinate
        clear_stdin_flag = 0;
    }

    if(packet->id == SERVER_MATCHREQUEST) {
        if(serialized != NULL) {
            Server_MatchRequest *req = (Server_MatchRequest *)serialized;
            pending_request_player = req->other_player;
            pending_request_match = req->match;
            printf("\n\n%s ===============================================\n", MSG_INFO);
            printf("%s RICHIESTA DI GIOCO!\n", MSG_INFO);
            printf("%s Il player #%d vuole unirsi alla tua partita #%d\n",
                   MSG_INFO, req->other_player, req->match);
            printf("%s Usa l'opzione 5 del menu per accettare o rifiutare\n", MSG_INFO);
            printf("%s ===============================================\n\n", MSG_INFO);
        }
    }

    if(packet->id == SERVER_UPDATEONREQUEST) {
        if(serialized != NULL) {
            Server_UpdateOnRequest *update = (Server_UpdateOnRequest *)serialized;
            if(update->accepted) {
                printf("\n%s La tua richiesta è stata ACCETTATA! Partita #%d inizia!\n",
                       MSG_INFO, update->match);
            } else {
                printf("\n%s La tua richiesta è stata RIFIUTATA per la partita #%d\n",
                       MSG_ERROR, update->match);
            }
        }
    }

    if(packet->id == SERVER_BROADCASTMATCH) {
        if(serialized != NULL) {
            Server_BroadcastMatch *bc = (Server_BroadcastMatch *)serialized;
            printf("%s Nuova partita disponibile: #%d (creata da player #%d)\n",
                   MSG_INFO, bc->match, bc->player_id);
        }
    }

    if(packet->id == SERVER_NOTICESTATE) {
        if(serialized != NULL) {
            Server_NoticeState *state = (Server_NoticeState *)serialized;
            current_state = state->state;
            current_match_id = state->match;

            switch(state->state) {
                case STATE_TURN_PLAYER1:
                case STATE_TURN_PLAYER2:
                    // Se match_ended era 1 e ora riceviamo TURN, significa che è un riavvio
                    if(match_ended == 1 && state->state == STATE_TURN_PLAYER1) {
                        printf("\n%s NUOVA PARTITA! Match #%d riavviato\n", MSG_INFO, state->match);
                        printf("=========================\n");
                        memset(client_grid, 0, sizeof(client_grid));
                        match_ended = 0;
                    }

                    printf("\n%s Partita #%d - %s\n", MSG_INFO, state->match,
                           (state->state == STATE_TURN_PLAYER1) ? "Turno Player 1 (X)" : "Turno Player 2 (O)");
                    // Visualizza griglia
                    printf("     0   1   2\n");
                    printf("   +---+---+---+\n");
                    for(int i = 0; i < 3; i++) {
                        printf(" %d |", i);
                        for(int j = 0; j < 3; j++) {
                            char c = client_grid[i][j];
                            if(c == 0) c = ' ';
                            printf(" %c |", c);
                        }
                        printf("\n   +---+---+---+\n");
                    }

                    // Indica se è il tuo turno
                    if((state->state == STATE_TURN_PLAYER1 && am_i_player1 == 1) ||
                       (state->state == STATE_TURN_PLAYER2 && am_i_player1 == 0)) {
                        my_turn_flag = 1;
                        clear_stdin_flag = 1; // Pulisci stdin prima di chiedere coordinate
                        printf("\n%s ================================\n", MSG_INFO);
                        printf("%s È IL TUO TURNO!\n", MSG_INFO);
                        printf("%s ================================\n", MSG_INFO);
                    } else {
                        my_turn_flag = 0;
                        printf("%s In attesa della mossa dell'avversario...\n", MSG_INFO);
                    }
                    break;

                case STATE_WIN:
                    printf("\n%s HAI VINTO! Partita #%d\n", MSG_INFO, state->match);
                    printf("=========================\n");
                    printf("%s Usa opzione 6 del menu per giocare ancora!\n", MSG_INFO);
                    match_ended = 1;
                    my_turn_flag = 0;
                    break;

                case STATE_LOSE:
                    printf("\n%s Hai perso. Partita #%d\n", MSG_ERROR, state->match);
                    printf("=========================\n");
                    printf("%s Usa opzione 6 del menu per giocare ancora!\n", MSG_INFO);
                    match_ended = 1;
                    my_turn_flag = 0;
                    break;

                case STATE_DRAW:
                    printf("\n%s PAREGGIO! Partita #%d\n", MSG_INFO, state->match);
                    printf("=========================\n");
                    printf("%s Usa opzione 6 del menu per giocare ancora!\n", MSG_INFO);
                    match_ended = 1;
                    my_turn_flag = 0;
                    break;

                case STATE_TERMINATED:
                    printf("%s Partita #%d terminata definitivamente\n", MSG_INFO, state->match);
                    current_match_id = -1;
                    match_ended = 0;
                    my_turn_flag = 0;
                    memset(client_grid, 0, sizeof(client_grid));
                    break;

                case STATE_INPROGRESS:
                    printf("%s Partita #%d in corso...\n", MSG_INFO, state->match);
                    current_match_id = state->match;
                    match_ended = 0;
                    memset(client_grid, 0, sizeof(client_grid));
                    break;
            }
        }
    }

    if(packet->id == SERVER_NOTICEMOVE) {
        if(serialized != NULL) {
            Server_NoticeMove *move = (Server_NoticeMove *)serialized;
            printf("%s Mossa: (%d,%d)\n", MSG_INFO, move->moveX, move->moveY);

            // Aggiorna griglia locale
            // Il turno corrente indica chi HA APPENA GIOCATO (il precedente)
            // Se current_state è TURN_PLAYER1, vuol dire che player2 ha appena giocato
            if(current_state == STATE_TURN_PLAYER1) {
                client_grid[move->moveX][move->moveY] = 'O'; // Player 2 ha giocato
            } else if(current_state == STATE_TURN_PLAYER2) {
                client_grid[move->moveX][move->moveY] = 'X'; // Player 1 ha giocato
            }
        }
    }

    if(serialized != NULL) {
        free(serialized);
    }
}

void print_grid() {
    printf("     0   1   2\n");
    printf("   +---+---+---+\n");
    for(int i = 0; i < 3; i++) {
        printf(" %d |", i);
        for(int j = 0; j < 3; j++) {
            char c = client_grid[i][j];
            if(c == 0) c = ' ';
            printf(" %c |", c);
        }
        printf("\n   +---+---+---+\n");
    }
}

void create_match(int sockfd) {
    Packet *packet = malloc(sizeof(Packet));
    packet->id = CLIENT_CREATEMATCH;

    packet->content = NULL;
    send_packet(sockfd, packet);
    free(packet);

    // Chi crea la partita è sempre Player1 (X)
    am_i_player1 = 1;

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

    // Chi fa join è sempre Player2 (O)
    am_i_player1 = 0;

    printf("%s Richiesta join partita #%d inviata\n", MSG_DEBUG, match_id);
}

void make_move(int sockfd, int match_id, int x, int y) {
    Client_MakeMove *move = malloc(sizeof(Client_MakeMove));
    move->moveX = x;
    move->moveY = y;
    move->match = match_id;

    Packet *packet = malloc(sizeof(Packet));
    packet->id = CLIENT_MAKEMOVE;
    packet->content = move;
    send_packet(sockfd, packet);

    free(packet);
    free(move);

    printf("%s Mossa inviata: (%d,%d) partita #%d\n", MSG_DEBUG, x, y, match_id);
}

void respond_to_request(int sockfd, int accepted) {
    if(pending_request_match == -1) {
        printf("%s Nessuna richiesta pendente!\n", MSG_ERROR);
        return;
    }

    Client_ModifyRequest *modify = malloc(sizeof(Client_ModifyRequest));
    modify->accepted = accepted;
    modify->match = pending_request_match;

    Packet *packet = malloc(sizeof(Packet));
    packet->id = CLIENT_MODIFYREQUEST;
    packet->content = modify;
    send_packet(sockfd, packet);

    free(packet);
    free(modify);

    printf("%s Risposta inviata: %s\n", MSG_INFO, accepted ? "ACCETTATO" : "RIFIUTATO");

    // Reset pending request
    pending_request_player = -1;
    pending_request_match = -1;
}

void play_again(int sockfd, int match_id, int choice) {
    Client_PlayAgain *play = malloc(sizeof(Client_PlayAgain));
    play->choice = choice;
    play->match = match_id;

    Packet *packet = malloc(sizeof(Packet));
    packet->id = CLIENT_PLAYAGAIN;
    packet->content = play;
    send_packet(sockfd, packet);

    free(packet);
    free(play);

    if(choice == 1) {
        printf("%s Richiesta 'Gioca Ancora' inviata. In attesa dell'altro giocatore...\n", MSG_INFO);
    } else {
        printf("%s Hai rifiutato di giocare ancora. Partita terminata.\n", MSG_INFO);
        current_match_id = -1;
        match_ended = 0;
        memset(client_grid, 0, sizeof(client_grid));
    }
}

void quit_match(int sockfd, int match_id) {
    Client_QuitMatch *quit = malloc(sizeof(Client_QuitMatch));
    quit->match = match_id;

    Packet *packet = malloc(sizeof(Packet));
    packet->id = CLIENT_QUITMATCH;
    packet->content = quit;
    send_packet(sockfd, packet);

    free(packet);
    free(quit);

    printf("%s Richiesta uscita dalla partita #%d inviata\n", MSG_INFO, match_id);
}
