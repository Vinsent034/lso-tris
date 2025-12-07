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
    
    printf("%s Connessione chiusa (fd=%d)\n", MSG_INFO, thread_args->client->conn);
    
    // TODO: rimuovere client dalla lista
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
                                send_packet(found_match->participants[0]->id, req_packet);
                                
                                printf("%s Notifica inviata a Player id=%d per richiesta\n", 
                                       MSG_INFO, found_match->participants[0]->id);
                                
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
                    send_packet(requester->id, update_packet);
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
                            send_packet(found_match->requests_head->requester->id, deny_packet);
                            
                            free(deny_packet);
                            free(deny);
                            delete_from_head(found_match);
                        }
                        
                        // TODO: Avviare la partita (Giorno 6)
                        printf("%s Match id=%d iniziato tra Player id=%d e id=%d\n", 
                               MSG_INFO, _packet->match, 
                               found_match->participants[0]->id, 
                               found_match->participants[1]->id);
                        
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
                            send_packet(found_match->participants[0]->id, next_packet);
                            
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




    if(serialized != NULL) {
        free(serialized);
    }
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