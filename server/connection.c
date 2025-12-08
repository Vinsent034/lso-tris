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

    int player_id = thread_args->client->conn % 255;
    printf("%s Connessione chiusa (Player id=%d, fd=%d)\n", MSG_INFO, player_id, thread_args->client->conn);

    // Trova partite a cui partecipa il player
    MatchList *current = matches;
    while(current != NULL) {
        Match *match = current->val;
        MatchList *next = current->next;  // Salva next prima di possibile rimozione

        if(match != NULL) {
            // Verifica se il player disconnesso è in questa partita
            int is_participant = 0;
            int other_player_id = -1;

            if(match->participants[0] != NULL && match->participants[0]->id == player_id) {
                is_participant = 1;
                if(match->participants[1] != NULL) {
                    other_player_id = match->participants[1]->id;
                }
            } else if(match->participants[1] != NULL && match->participants[1]->id == player_id) {
                is_participant = 1;
                if(match->participants[0] != NULL) {
                    other_player_id = match->participants[0]->id;
                }
            }

            if(is_participant) {
                printf("%s Player id=%d disconnesso da Match id=%d\n", MSG_WARNING, player_id, match->id);

                // Notifica l'altro player che la partita è terminata
                if(other_player_id != -1) {
                    int other_fd = get_socket_by_player_id(other_player_id);
                    if(other_fd != -1) {
                        Server_NoticeState *term_state = malloc(sizeof(Server_NoticeState));
                        term_state->state = STATE_TERMINATED;
                        term_state->match = match->id;

                        Packet *term_packet = malloc(sizeof(Packet));
                        term_packet->id = SERVER_NOTICESTATE;
                        term_packet->content = term_state;

                        send_packet(other_fd, term_packet);

                        free(term_packet);
                        free(term_state);

                        printf("%s Notifica disconnessione inviata a Player id=%d\n", MSG_INFO, other_player_id);
                    }

                    // Reset busy flag dell'altro player
                    if(match->participants[0] != NULL && match->participants[0]->id == other_player_id) {
                        match->participants[0]->busy = 0;
                    } else if(match->participants[1] != NULL && match->participants[1]->id == other_player_id) {
                        match->participants[1]->busy = 0;
                    }
                }

                // Rimuovi la partita
                remove_match(match);
                printf("%s Match id=%d rimosso a causa della disconnessione\n", MSG_INFO, match->id);
            }
        }

        current = next;
    }

    // Reset busy flag se il player era in partita
    if(thread_args->client->player != NULL) {
        thread_args->client->player->busy = 0;
    }

    // Rimuovi client dalla lista
    remove_client_from_list(thread_args->client);
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
    

















 // ===== JOIN MATCH =====
    if(packet->id == CLIENT_JOINMATCH) {
        printf("%s DEBUG: Ricevuto CLIENT_JOINMATCH da player_id=%d\n", MSG_DEBUG, player_id);
        if(serialized != NULL) {
             printf("%s DEBUG: serialized OK\n", MSG_DEBUG);
            Client_JoinMatch *_packet = (Client_JoinMatch *)serialized;
            Match *found_match = get_match_by_id(matches, _packet->match);
            
            if(found_match != NULL) {
                // Controlla che non sia già in partita
                if(!player->busy) {
                    // Controlla che non sia il proprietario
                    if(found_match->participants[0]->id != player_id) {
                        // Controlla che la partita non sia già iniziata
                        if(!found_match->participants[0]->busy) {
                            // Aggiungi alla coda richieste
                            RequestNode *node = malloc(sizeof(RequestNode));
                            node->requester = player;
                            node->next = NULL;
                            add_requester(found_match, node);
                            
                            printf("%s Player id=%d richiede join a Match id=%d\n", 
                                   MSG_INFO, player_id, _packet->match);
                            
                            // Invia SUCCESS al richiedente
                            Packet *success = malloc(sizeof(Packet));
                            success->id = SERVER_SUCCESS;
                            success->content = NULL;
                            send_packet(client->conn, success);
                            free(success);
                            
                            // Notifica il proprietario se è il primo in coda
                            if(found_match->requests_head == node) {
                                Server_MatchRequest *request = malloc(sizeof(Server_MatchRequest));
                                request->other_player = player_id;
                                request->match = _packet->match;

                                Packet *req_packet = malloc(sizeof(Packet));
                                req_packet->id = SERVER_MATCHREQUEST;
                                req_packet->content = request;

                                int owner_fd = get_socket_by_player_id(found_match->participants[0]->id);
                                if(owner_fd != -1) {
                                    send_packet(owner_fd, req_packet);
                                    printf("%s Notifica inviata a Player id=%d per richiesta\n",
                                           MSG_INFO, found_match->participants[0]->id);
                                }

                                free(req_packet);
                                free(request);
                            }
                        } else {
                            printf("%s Player id=%d ha provato ad entrare in Match id=%d già iniziato\n", 
                                   MSG_WARNING, player_id, _packet->match);
                            
                            Packet *error = malloc(sizeof(Packet));
                            error->id = SERVER_ERROR;
                            error->content = NULL;
                            send_packet(client->conn, error);
                            free(error);
                        }
                    } else {
                        printf("%s Player id=%d ha provato ad entrare nel suo stesso Match id=%d\n", 
                               MSG_WARNING, player_id, _packet->match);
                        
                        Packet *error = malloc(sizeof(Packet));
                        error->id = SERVER_ERROR;
                        error->content = NULL;
                        send_packet(client->conn, error);
                        free(error);
                    }
                } else {
                    printf("%s Player id=%d è già occupato in un'altra partita\n", 
                           MSG_WARNING, player_id);
                    
                    Packet *error = malloc(sizeof(Packet));
                    error->id = SERVER_ERROR;
                    error->content = NULL;
                    send_packet(client->conn, error);
                    free(error);
                }
            } else {
                printf("%s Player id=%d ha provato ad entrare in Match id=%d non valido\n", 
                       MSG_WARNING, player_id, _packet->match);
                
                Packet *error = malloc(sizeof(Packet));
                error->id = SERVER_ERROR;
                error->content = NULL;
                send_packet(client->conn, error);
                free(error);
            }
        }
    }



// ===== MODIFY REQUEST (Accept/Reject) =====
    if(packet->id == CLIENT_MODIFYREQUEST) {
        if(serialized != NULL) {
            Client_ModifyRequest *_packet = (Client_ModifyRequest *)serialized;
            Match *found_match = get_match_by_id(matches, _packet->match);
            
            if(found_match != NULL && found_match->requests_head != NULL) {
                if(found_match->participants[0]->id == player_id) {
                    Player *requester = found_match->requests_head->requester;

                    // Invia risposta al richiedente
                    Server_UpdateOnRequest *update = malloc(sizeof(Server_UpdateOnRequest));
                    update->accepted = _packet->accepted;
                    update->match = _packet->match;

                    Packet *update_packet = malloc(sizeof(Packet));
                    update_packet->id = SERVER_UPDATEONREQUEST;
                    update_packet->content = update;

                    int requester_fd = get_socket_by_player_id(requester->id);
                    if(requester_fd != -1) {
                        send_packet(requester_fd, update_packet);
                    }

                    free(update_packet);
                    free(update);
                    
                    if(_packet->accepted) {
                        // ACCETTATO - Inizia la partita
                        found_match->participants[1] = requester;
                        found_match->participants[0]->busy = 1;
                        found_match->participants[1]->busy = 1;
                        found_match->state = STATE_INPROGRESS;
                        
                        printf("%s Player id=%d ha accettato Player id=%d nel Match id=%d\n", 
                               MSG_INFO, player_id, requester->id, _packet->match);
                        
                        // Rimuovi dalla coda
                        delete_from_head(found_match);
                        
                        // Rifiuta tutti gli altri in coda
                        while(found_match->requests_head != NULL) {
                            Server_UpdateOnRequest *deny = malloc(sizeof(Server_UpdateOnRequest));
                            deny->accepted = 0;
                            deny->match = _packet->match;

                            Packet *deny_packet = malloc(sizeof(Packet));
                            deny_packet->id = SERVER_UPDATEONREQUEST;
                            deny_packet->content = deny;

                            int deny_fd = get_socket_by_player_id(found_match->requests_head->requester->id);
                            if(deny_fd != -1) {
                                send_packet(deny_fd, deny_packet);
                            }

                            free(deny_packet);
                            free(deny);
                            delete_from_head(found_match);
                        }
                        
                        // Inizializza griglia pulita
                        memset(found_match->grid, 0, sizeof(found_match->grid));
                        found_match->free_slots = 9;

                        // Imposta turno iniziale (Player 1 inizia)
                        found_match->state = STATE_TURN_PLAYER1;

                        printf("%s Match id=%d iniziato tra Player id=%d e id=%d\n",
                               MSG_INFO, _packet->match,
                               found_match->participants[0]->id,
                               found_match->participants[1]->id);

                        // Notifica entrambi i giocatori che la partita è iniziata
                        Server_NoticeState *start_state = malloc(sizeof(Server_NoticeState));
                        start_state->state = STATE_TURN_PLAYER1;
                        start_state->match = _packet->match;

                        Packet *start_packet = malloc(sizeof(Packet));
                        start_packet->id = SERVER_NOTICESTATE;
                        start_packet->content = start_state;

                        int owner_fd = get_socket_by_player_id(found_match->participants[0]->id);
                        if(owner_fd != -1) send_packet(owner_fd, start_packet);
                        if(requester_fd != -1) send_packet(requester_fd, start_packet);

                        free(start_packet);
                        free(start_state);
                        
                    } else {
                        // RIFIUTATO
                        printf("%s Player id=%d ha rifiutato Player id=%d per Match id=%d\n", 
                               MSG_INFO, player_id, requester->id, _packet->match);
                        
                        delete_from_head(found_match);
                        
                        // Se c'è un altro in coda, notifica il proprietario
                        if(found_match->requests_head != NULL) {
                            Server_MatchRequest *next_req = malloc(sizeof(Server_MatchRequest));
                            next_req->other_player = found_match->requests_head->requester->id;
                            next_req->match = _packet->match;

                            Packet *next_packet = malloc(sizeof(Packet));
                            next_packet->id = SERVER_MATCHREQUEST;
                            next_packet->content = next_req;

                            int owner_fd = get_socket_by_player_id(found_match->participants[0]->id);
                            if(owner_fd != -1) {
                                send_packet(owner_fd, next_packet);
                            }

                            free(next_packet);
                            free(next_req);
                        }
                    }
                } else {
                    printf("%s Player id=%d ha provato a modificare Match id=%d senza permessi\n", 
                           MSG_WARNING, player_id, _packet->match);
                    
                    Packet *error = malloc(sizeof(Packet));
                    error->id = SERVER_ERROR;
                    error->content = NULL;
                    send_packet(client->conn, error);
                    free(error);
                }
            } else {
                printf("%s Match id=%d non valido o senza richieste\n", 
                       MSG_WARNING, _packet->match);
                
                Packet *error = malloc(sizeof(Packet));
                error->id = SERVER_ERROR;
                error->content = NULL;
                send_packet(client->conn, error);
                free(error);
            }
        }
    }




    // ===== MAKE MOVE =====
    if(packet->id == CLIENT_MAKEMOVE) {
        if(serialized != NULL) {
            Client_MakeMove *_packet = (Client_MakeMove *)serialized;
            Match *found_match = get_match_by_id(matches, _packet->match);

            if(found_match != NULL) {
                // Verifica che il player sia in questa partita
                int player_index = -1;
                if(found_match->participants[0]->id == player_id) {
                    player_index = 0;
                } else if(found_match->participants[1]->id == player_id) {
                    player_index = 1;
                }

                if(player_index != -1) {
                    // Verifica che sia il suo turno
                    int expected_state = (player_index == 0) ? STATE_TURN_PLAYER1 : STATE_TURN_PLAYER2;
                    if(found_match->state == expected_state) {
                        // Valida coordinate (0-2)
                        if(_packet->moveX >= 0 && _packet->moveX < 3 &&
                           _packet->moveY >= 0 && _packet->moveY < 3) {
                            // Verifica che la casella sia vuota
                            if(found_match->grid[_packet->moveX][_packet->moveY] == 0) {
                                // Mossa valida! Applica
                                char symbol = (player_index == 0) ? 'X' : 'O';
                                found_match->grid[_packet->moveX][_packet->moveY] = symbol;
                                found_match->free_slots--;

                                printf("%s Player id=%d ha giocato %c in (%d,%d) Match id=%d\n",
                                       MSG_INFO, player_id, symbol, _packet->moveX, _packet->moveY, _packet->match);

                                // Notifica entrambi i giocatori della mossa
                                Server_NoticeMove *notice = malloc(sizeof(Server_NoticeMove));
                                notice->moveX = _packet->moveX;
                                notice->moveY = _packet->moveY;
                                notice->match = _packet->match;

                                Packet *notice_packet = malloc(sizeof(Packet));
                                notice_packet->id = SERVER_NOTICEMOVE;
                                notice_packet->content = notice;

                                // Invia a entrambi
                                int p0_fd = get_socket_by_player_id(found_match->participants[0]->id);
                                int p1_fd = get_socket_by_player_id(found_match->participants[1]->id);
                                if(p0_fd != -1) send_packet(p0_fd, notice_packet);
                                if(p1_fd != -1) send_packet(p1_fd, notice_packet);

                                free(notice_packet);
                                free(notice);

                                // Controlla vittoria
                                int winner = check_winner(found_match);
                                if(winner != -1) {
                                    // Abbiamo un vincitore!
                                    end_match(found_match, winner);
                                } else if(is_board_full(found_match)) {
                                    // Pareggio
                                    end_match(found_match, -1);
                                } else {
                                    // Continua - passa il turno
                                    found_match->state = (found_match->state == STATE_TURN_PLAYER1) ?
                                                         STATE_TURN_PLAYER2 : STATE_TURN_PLAYER1;

                                    // Notifica cambio turno
                                    Server_NoticeState *state_notice = malloc(sizeof(Server_NoticeState));
                                    state_notice->state = found_match->state;
                                    state_notice->match = _packet->match;

                                    Packet *state_packet = malloc(sizeof(Packet));
                                    state_packet->id = SERVER_NOTICESTATE;
                                    state_packet->content = state_notice;

                                    if(p0_fd != -1) send_packet(p0_fd, state_packet);
                                    if(p1_fd != -1) send_packet(p1_fd, state_packet);

                                    free(state_packet);
                                    free(state_notice);
                                }
                            } else {
                                printf("%s Player id=%d: casella (%d,%d) già occupata\n",
                                       MSG_WARNING, player_id, _packet->moveX, _packet->moveY);
                                Packet *error = malloc(sizeof(Packet));
                                error->id = SERVER_INVALID_MOVE;
                                error->content = NULL;
                                send_packet(client->conn, error);
                                free(error);
                            }
                        } else {
                            printf("%s Player id=%d: coordinate non valide\n", MSG_WARNING, player_id);
                            Packet *error = malloc(sizeof(Packet));
                            error->id = SERVER_ERROR;
                            error->content = NULL;
                            send_packet(client->conn, error);
                            free(error);
                        }
                    } else {
                        printf("%s Player id=%d: non è il tuo turno\n", MSG_WARNING, player_id);
                        Packet *error = malloc(sizeof(Packet));
                        error->id = SERVER_ERROR;
                        error->content = NULL;
                        send_packet(client->conn, error);
                        free(error);
                    }
                } else {
                    printf("%s Player id=%d non è in Match id=%d\n", MSG_WARNING, player_id, _packet->match);
                }
            }
        }
    }


    // ===== PLAY AGAIN =====
    if(packet->id == CLIENT_PLAYAGAIN) {
        if(serialized != NULL) {
            Client_PlayAgain *_packet = (Client_PlayAgain *)serialized;
            Match *found_match = get_match_by_id(matches, _packet->match);

            if(found_match != NULL && found_match->state == STATE_TERMINATED) {
                // Verifica che il player sia in questa partita
                int player_index = -1;
                if(found_match->participants[0]->id == player_id) {
                    player_index = 0;
                } else if(found_match->participants[1]->id == player_id) {
                    player_index = 1;
                }

                if(player_index != -1) {
                    if(_packet->choice == 1) {
                        // Il player vuole giocare ancora
                        printf("%s Player id=%d vuole giocare ancora Match id=%d\n",
                               MSG_INFO, player_id, _packet->match);

                        // Registra il player come "pronto"
                        found_match->play_again[player_index] = found_match->participants[player_index];
                        found_match->play_again_counter++;

                        // Se entrambi i player vogliono giocare ancora
                        if(found_match->play_again_counter == 2) {
                            printf("%s Entrambi i player vogliono giocare ancora. Riavvio Match id=%d\n",
                                   MSG_INFO, _packet->match);

                            // Reset della partita
                            memset(found_match->grid, 0, sizeof(found_match->grid));
                            found_match->free_slots = 9;
                            found_match->state = STATE_TURN_PLAYER1;
                            found_match->play_again_counter = 0;
                            found_match->play_again[0] = NULL;
                            found_match->play_again[1] = NULL;

                            // Resetta busy flags
                            found_match->participants[0]->busy = 1;
                            found_match->participants[1]->busy = 1;

                            // Notifica entrambi i giocatori che la partita ricomincia
                            Server_NoticeState *restart_state = malloc(sizeof(Server_NoticeState));
                            restart_state->state = STATE_TURN_PLAYER1;
                            restart_state->match = _packet->match;

                            Packet *restart_packet = malloc(sizeof(Packet));
                            restart_packet->id = SERVER_NOTICESTATE;
                            restart_packet->content = restart_state;

                            int p0_fd = get_socket_by_player_id(found_match->participants[0]->id);
                            int p1_fd = get_socket_by_player_id(found_match->participants[1]->id);

                            if(p0_fd != -1) send_packet(p0_fd, restart_packet);
                            if(p1_fd != -1) send_packet(p1_fd, restart_packet);

                            free(restart_packet);
                            free(restart_state);

                            printf("%s Match id=%d riavviato con successo\n", MSG_INFO, _packet->match);
                        } else {
                            // Solo un giocatore ha detto sì, aspetta l'altro
                            printf("%s Match id=%d in attesa dell'altro player\n", MSG_INFO, _packet->match);

                            Packet *success = malloc(sizeof(Packet));
                            success->id = SERVER_SUCCESS;
                            success->content = NULL;
                            send_packet(client->conn, success);
                            free(success);
                        }
                    } else {
                        // Il player NON vuole giocare ancora
                        printf("%s Player id=%d NON vuole giocare ancora Match id=%d\n",
                               MSG_INFO, player_id, _packet->match);

                        // Reset counter e rimuovi la partita
                        found_match->play_again_counter = 0;
                        found_match->play_again[0] = NULL;
                        found_match->play_again[1] = NULL;

                        // Notifica l'altro giocatore che la partita è definitivamente terminata
                        int other_index = (player_index == 0) ? 1 : 0;
                        int other_fd = get_socket_by_player_id(found_match->participants[other_index]->id);

                        Server_NoticeState *end_state = malloc(sizeof(Server_NoticeState));
                        end_state->state = STATE_TERMINATED;
                        end_state->match = _packet->match;

                        Packet *end_packet = malloc(sizeof(Packet));
                        end_packet->id = SERVER_NOTICESTATE;
                        end_packet->content = end_state;

                        if(other_fd != -1) send_packet(other_fd, end_packet);

                        free(end_packet);
                        free(end_state);

                        // Rimuovi la partita
                        remove_match(found_match);
                        printf("%s Match id=%d rimosso definitivamente\n", MSG_INFO, _packet->match);
                    }
                } else {
                    printf("%s Player id=%d non è in Match id=%d\n",
                           MSG_WARNING, player_id, _packet->match);

                    Packet *error = malloc(sizeof(Packet));
                    error->id = SERVER_ERROR;
                    error->content = NULL;
                    send_packet(client->conn, error);
                    free(error);
                }
            } else {
                printf("%s Match id=%d non valido o non terminato\n",
                       MSG_WARNING, _packet->match);

                Packet *error = malloc(sizeof(Packet));
                error->id = SERVER_ERROR;
                error->content = NULL;
                send_packet(client->conn, error);
                free(error);
            }
        }
    }


    // ===== QUIT MATCH =====
    if(packet->id == CLIENT_QUITMATCH) {
        if(serialized != NULL) {
            Client_QuitMatch *_packet = (Client_QuitMatch *)serialized;
            Match *found_match = get_match_by_id(matches, _packet->match);

            if(found_match != NULL) {
                // Verifica che il player sia in questa partita
                int player_index = -1;
                int other_player_id = -1;

                if(found_match->participants[0]->id == player_id) {
                    player_index = 0;
                    if(found_match->participants[1] != NULL) {
                        other_player_id = found_match->participants[1]->id;
                    }
                } else if(found_match->participants[1]->id == player_id) {
                    player_index = 1;
                    if(found_match->participants[0] != NULL) {
                        other_player_id = found_match->participants[0]->id;
                    }
                }

                if(player_index != -1) {
                    printf("%s Player id=%d è uscito dalla partita #%d\n",
                           MSG_INFO, player_id, _packet->match);

                    // Notifica l'altro giocatore che ha vinto per abbandono
                    if(other_player_id != -1) {
                        int other_fd = get_socket_by_player_id(other_player_id);
                        if(other_fd != -1) {
                            Server_NoticeState *win_state = malloc(sizeof(Server_NoticeState));
                            win_state->state = STATE_WIN;
                            win_state->match = _packet->match;

                            Packet *win_packet = malloc(sizeof(Packet));
                            win_packet->id = SERVER_NOTICESTATE;
                            win_packet->content = win_state;

                            send_packet(other_fd, win_packet);

                            free(win_packet);
                            free(win_state);

                            printf("%s Player id=%d vince per abbandono\n", MSG_INFO, other_player_id);
                        }

                        // Reset busy flags
                        if(found_match->participants[0] != NULL) {
                            found_match->participants[0]->busy = 0;
                        }
                        if(found_match->participants[1] != NULL) {
                            found_match->participants[1]->busy = 0;
                        }
                    }

                    // Rimuovi la partita
                    remove_match(found_match);
                    printf("%s Match id=%d rimosso per abbandono\n", MSG_INFO, _packet->match);
                } else {
                    printf("%s Player id=%d non è in Match id=%d\n",
                           MSG_WARNING, player_id, _packet->match);
                }
            } else {
                printf("%s Match id=%d non trovato\n", MSG_WARNING, _packet->match);
            }
        }
    }

    if(serialized != NULL) {
        free(serialized);
    }
}

// ===== GAME LOGIC HELPERS =====

int check_winner(Match *match) {
    char grid[3][3];
    memcpy(grid, match->grid, sizeof(grid));

    // Controlla righe
    for(int i = 0; i < 3; i++) {
        if(grid[i][0] != 0 && grid[i][0] == grid[i][1] && grid[i][1] == grid[i][2]) {
            return (grid[i][0] == 'X') ? 0 : 1;
        }
    }

    // Controlla colonne
    for(int j = 0; j < 3; j++) {
        if(grid[0][j] != 0 && grid[0][j] == grid[1][j] && grid[1][j] == grid[2][j]) {
            return (grid[0][j] == 'X') ? 0 : 1;
        }
    }

    // Controlla diagonale principale (\)
    if(grid[0][0] != 0 && grid[0][0] == grid[1][1] && grid[1][1] == grid[2][2]) {
        return (grid[0][0] == 'X') ? 0 : 1;
    }

    // Controlla diagonale secondaria (/)
    if(grid[0][2] != 0 && grid[0][2] == grid[1][1] && grid[1][1] == grid[2][0]) {
        return (grid[0][2] == 'X') ? 0 : 1;
    }

    return -1; // Nessun vincitore
}

int is_board_full(Match *match) {
    return match->free_slots == 0;
}

void end_match(Match *match, int winner_index) {
    // winner_index: 0=player1 vince, 1=player2 vince, -1=pareggio

    printf("%s Match id=%d terminato: %s\n", MSG_INFO, match->id,
           (winner_index == -1) ? "PAREGGIO" :
           (winner_index == 0) ? "Vince Player1" : "Vince Player2");

    // Reset busy flags
    match->participants[0]->busy = 0;
    match->participants[1]->busy = 0;

    // Invia notifiche ai giocatori
    int p0_fd = get_socket_by_player_id(match->participants[0]->id);
    int p1_fd = get_socket_by_player_id(match->participants[1]->id);

    Server_NoticeState *state0 = malloc(sizeof(Server_NoticeState));
    Server_NoticeState *state1 = malloc(sizeof(Server_NoticeState));

    if(winner_index == -1) {
        // Pareggio
        state0->state = STATE_DRAW;
        state1->state = STATE_DRAW;
    } else if(winner_index == 0) {
        // Player 0 vince
        state0->state = STATE_WIN;
        state1->state = STATE_LOSE;
    } else {
        // Player 1 vince
        state0->state = STATE_LOSE;
        state1->state = STATE_WIN;
    }

    state0->match = match->id;
    state1->match = match->id;

    Packet *packet0 = malloc(sizeof(Packet));
    packet0->id = SERVER_NOTICESTATE;
    packet0->content = state0;

    Packet *packet1 = malloc(sizeof(Packet));
    packet1->id = SERVER_NOTICESTATE;
    packet1->content = state1;

    if(p0_fd != -1) send_packet(p0_fd, packet0);
    if(p1_fd != -1) send_packet(p1_fd, packet1);

    free(packet0);
    free(packet1);
    free(state0);
    free(state1);

    // Imposta stato terminato
    match->state = STATE_TERMINATED;
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