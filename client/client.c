#include "../common/models.h"
#include "../common/protocol.h"
#include "connection.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define DEFAULT_IP      "127.0.0.1"
#define DEFAULT_PORT    5555
#define BUFFER_SIZE     1024

void *client_thread(void *arg) {
    int sockfd = *((int*)arg);
    char buffer[BUFFER_SIZE];
    
    while(1) {
        ssize_t received = recv(sockfd, buffer, BUFFER_SIZE, 0);
        if(received <= 0) {
            printf("%s Connessione chiusa dal server\n", MSG_INFO);
            exit(0);
        }
        
        size_t total = received;
        while(total > 0) {
            char *block = buffer + (received - total);
            Packet *packet = malloc(sizeof(Packet));
            packet->id = block[0];
            packet->size = block[1] + (block[2] << 8);
            packet->content = malloc(sizeof(char) * packet->size);
            memcpy(packet->content, block + 3, packet->size);
            
            handle_packet(sockfd, packet);
            
            total -= packet->size + 3;
            free(packet->content);
            free(packet);
        }
    }
    
    return NULL;
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    
    char *ip = DEFAULT_IP;
    int port = DEFAULT_PORT;
    int sockfd = 0;
    struct sockaddr_in address;
    pthread_t tid;

    if(argc == 3) {
        ip = argv[1];
        if(sscanf(argv[2], "%d", &port) <= 0 || port < 0 || port > 65535) {
            fprintf(stderr, "%s Porta non valida: %s\n", MSG_ERROR, argv[2]);
            return 1;
        }
    }

    printf("%s Connessione a %s:%d\n", MSG_INFO, ip, port);

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "%s Impossibile creare socket: %s\n", MSG_ERROR, strerror(errno));
        return 1;
    }

    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    if(inet_pton(AF_INET, ip, &address.sin_addr) <= 0) {
        fprintf(stderr, "%s Indirizzo non valido: %s\n", MSG_ERROR, ip);
        return 1;
    }

    if(connect(sockfd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        fprintf(stderr, "%s Impossibile connettersi: %s\n", MSG_ERROR, strerror(errno));
        return 1;
    }

    printf("%s Connesso al server!\n", MSG_INFO);
    
    // Invia HANDSHAKE
    Packet *handshake = malloc(sizeof(Packet));
    handshake->id = CLIENT_HANDSHAKE;
    handshake->content = NULL;
    send_packet(sockfd, handshake);
    free(handshake);
    
    // Avvia thread per ricevere messaggi
    if(pthread_create(&tid, NULL, client_thread, &sockfd) < 0) {
        fprintf(stderr, "%s Impossibile creare thread: %s\n", MSG_ERROR, strerror(errno));
        return 1;
    }
    
    // Attendi player_id
    while(player_id == -1) {
        usleep(100000);
    }
    
    printf("\n=== MENU ===\n");
    
    int scelta = 0;
    char input[100];

    while(1) {
        // Se è il mio turno, chiedi automaticamente le coordinate
        if(my_turn_flag == 1) {
            // Pulisci stdin se necessario (dopo errore di casella occupata)
            if(clear_stdin_flag == 1) {
                int c;
                while ((c = getchar()) != '\n' && c != EOF);
                clear_stdin_flag = 0;
            }

            printf("\nInserisci prima coordinata (riga 0-2, oppure 'N' per uscire): ");
            if(scanf("%s", input) > 0) {
                // Controlla se vuole uscire
                if(input[0] == 'N' || input[0] == 'n') {
                    printf("\n%s Sei uscito dalla partita.\n", MSG_INFO);
                    quit_match(sockfd, current_match_id);
                    current_match_id = -1;
                    my_turn_flag = 0;
                    match_ended = 0;
                    memset(client_grid, 0, sizeof(client_grid));
                    continue;
                }

                int x = atoi(input);
                if(x < 0 || x > 2) {
                    printf("%s Coordinata non valida! Deve essere tra 0 e 2.\n", MSG_ERROR);
                    continue;
                }

                printf("Inserisci seconda coordinata (colonna 0-2, oppure 'N' per uscire): ");
                if(scanf("%s", input) > 0) {
                    // Controlla se vuole uscire
                    if(input[0] == 'N' || input[0] == 'n') {
                        printf("\n%s Sei uscito dalla partita.\n", MSG_INFO);
                        quit_match(sockfd, current_match_id);
                        current_match_id = -1;
                        my_turn_flag = 0;
                        match_ended = 0;
                        memset(client_grid, 0, sizeof(client_grid));
                        continue;
                    }

                    int y = atoi(input);
                    if(y < 0 || y > 2) {
                        printf("%s Coordinata non valida! Deve essere tra 0 e 2.\n", MSG_ERROR);
                        continue;
                    }

                    // Invia la mossa
                    make_move(sockfd, current_match_id, x, y);
                    my_turn_flag = 0; // Reset flag dopo aver giocato
                }
            }
            continue;
        }

        // Se siamo in una partita attiva e non è il nostro turno, aspetta senza mostrare menu
        if(current_match_id != -1 && match_ended == 0 && my_turn_flag == 0) {
            usleep(100000); // Aspetta 100ms
            continue; // Controlla di nuovo se è diventato il nostro turno
        }

        // Controlla se nel frattempo è diventato il nostro turno
        if(my_turn_flag == 1) {
            continue; // Salta il menu e vai direttamente al prompt coordinate
        }

        // Menu normale quando non è il tuo turno (e non sei in partita)
        printf("\n1. Crea partita\n");
        printf("2. Join partita\n");
        printf("4. Visualizza griglia\n");
        if(pending_request_match != -1) {
            printf("5. Rispondi a richiesta (player #%d vuole giocare) [PENDENTE]\n", pending_request_player);
        }
        if(match_ended == 1) {
            printf("6. Gioca ancora [DISPONIBILE]\n");
        }
        printf("9. Esci\n");
        printf("> Scegli opzione: ");

        if(scanf("%d", &scelta) < 0) {
            continue;
        }

        // Dopo aver letto l'input, ricontrolla se è diventato il nostro turno
        if(my_turn_flag == 1) {
            continue; // Ignora l'input e vai al prompt coordinate
        }

        if(scelta == 1) {
            create_match(sockfd);
        } else if(scelta == 2) {
            int match_id;
            printf("> Inserisci ID partita: ");
            if(scanf("%d", &match_id) > 0) {
                if(match_id < 0 || match_id > 255) {
                    printf("%s ID partita non valido! Deve essere tra 0 e 255.\n", MSG_ERROR);
                } else {
                    join_match(sockfd, match_id);
                }
            }
        } else if(scelta == 4) {
            if(current_match_id == -1) {
                printf("%s Non sei in una partita attiva!\n", MSG_ERROR);
            } else {
                print_grid(client_grid);
            }
        } else if(scelta == 5) {
            if(pending_request_match != -1) {
                int accept;
                printf("> Accetti? (1=Sì, 0=No): ");
                if(scanf("%d", &accept) > 0) {
                    respond_to_request(sockfd, accept);
                }
            } else {
                printf("%s Nessuna richiesta pendente!\n", MSG_ERROR);
            }
        } else if(scelta == 6) {
            if(match_ended == 1 && current_match_id != -1) {
                int choice;
                printf("> Vuoi giocare ancora? (1=Sì, 0=No): ");
                if(scanf("%d", &choice) > 0) {
                    play_again(sockfd, current_match_id, choice);
                }
            } else {
                printf("%s Nessuna partita terminata da cui rigiocare!\n", MSG_ERROR);
            }
        } else if(scelta == 9) {
            printf("%s Disconnessione...\n", MSG_INFO);
            close(sockfd);
            exit(0);
        }
    }
    
    pthread_join(tid, NULL);
    close(sockfd);
    
    return 0;
}