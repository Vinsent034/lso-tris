#define _DEFAULT_SOURCE
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include "../common/models.h"
#include "../common/protocol.h"
#include "connection.h"

#define DEFAULT_IP      "127.0.0.1"
#define DEFAULT_PORT    5555
#define BUFFER_SIZE     1024

static void show_menu();

static volatile int g_sockfd = -1;

static void sigint_handler(int sig) {
    (void)sig;
    // Se siamo in partita, invia quit così l'avversario viene notificato della vittoria
    if(g_sockfd >= 0 && current_match_id != -1) {
        quit_match(g_sockfd, current_match_id);
    }
    printf("\n%s Disconnessione...\n", MSG_INFO);
    if(g_sockfd >= 0)
        close(g_sockfd);
    _exit(0);
}

// ========== PARSING ARGOMENTI ==========

static int parse_args(int argc, char **argv, char **ip, int *port) {
    *ip = DEFAULT_IP;
    *port = DEFAULT_PORT;

    if(argc == 3) {
        *ip = argv[1];
        if(sscanf(argv[2], "%d", port) <= 0 || *port < 0 || *port > 65535) {
            fprintf(stderr, "%s Porta non valida: %s\n", MSG_ERROR, argv[2]);
            return -1;
        }
    }
    return 0;
}

// ========== CONNESSIONE AL SERVER ==========

static int connect_to_server(const char *ip, int port) {
    int sockfd;
    struct sockaddr_in address;

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "%s Impossibile creare socket: %s\n", MSG_ERROR, strerror(errno));
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if(getaddrinfo(ip, NULL, &hints, &result) != 0) {
        fprintf(stderr, "%s Impossibile risolvere l'indirizzo: %s\n", MSG_ERROR, ip);
        close(sockfd);
        return -1;
    }

    memcpy(&address.sin_addr, &((struct sockaddr_in *)result->ai_addr)->sin_addr, sizeof(struct in_addr));
    freeaddrinfo(result);

    if(connect(sockfd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        fprintf(stderr, "%s Impossibile connettersi: %s\n", MSG_ERROR, strerror(errno));
        close(sockfd);
        return -1;
    }

    printf("%s Connesso al server!\n", MSG_INFO);
    return sockfd;
}

// ========== INVIO HANDSHAKE ==========

static void send_handshake(int sockfd) {
    Packet *handshake = malloc(sizeof(Packet));
    handshake->id = CLIENT_HANDSHAKE;
    handshake->content = NULL;
    send_packet(sockfd, handshake);
    free(handshake);
}

// ========== RICEZIONE PACCHETTI (THREAD) ==========

static int process_packets(int sockfd, char *buffer, ssize_t received) {

    size_t total = received;

    while(total > 0) {
        char *block = buffer + (received - total);

        if(total < 3) {
            fprintf(stderr, "[WARN] Pacchetto incompleto (header): solo %zu byte disponibili\n", total);
            break;
        }

        Packet *packet = malloc(sizeof(Packet));
        packet->id = block[0];
        packet->size = block[1] + (block[2] << 8);

        if(packet->size > 65536) {
            fprintf(stderr, "[ERROR] Dimensione pacchetto troppo grande: %d byte\n", packet->size);
            free(packet);
            break;
        }

        if(total < (size_t)(packet->size + 3)) {
            fprintf(stderr, "[WARN] Pacchetto incompleto (payload): attesi %d byte, disponibili %zu\n",
                    packet->size + 3, total);
            free(packet);
            break;
        }

        packet->content = malloc(sizeof(char) * packet->size);
        memcpy(packet->content, block + 3, packet->size);

        handle_packet(sockfd, packet);

        total -= packet->size + 3;
        free(packet->content);
        free(packet);
    }

    return 0;
}

static void *receiver_thread(void *arg) {
    int sockfd = *((int*)arg);
    char buffer[BUFFER_SIZE];

    while(1) {
        ssize_t received = recv(sockfd, buffer, BUFFER_SIZE, 0);
        if(received <= 0) {
            printf("%s Connessione chiusa dal server\n", MSG_INFO);
            exit(0);
        }
        process_packets(sockfd, buffer, received);
    }

    return NULL;
}

// ========== GESTIONE INPUT: TURNO DI GIOCO ==========

static int read_coordinate(const char *prompt) {
    char input[100];
    printf("%s", prompt);

    if(fgets(input, sizeof(input), stdin) == NULL)
        return -2;

    // Rimuovi newline
    input[strcspn(input, "\n")] = '\0';

    if(input[0] == 'N' || input[0] == 'n')
        return -1;

    // Cheat code per debug: "ù" = vittoria immediata
    if((unsigned char)input[0] == 0xC3 && (unsigned char)input[1] == 0xB9)
        return -3;

    if(input[0] == '\0')
        return -2;

    char *endptr;
    errno = 0;
    long val = strtol(input, &endptr, 10);
    if(errno != 0 || endptr == input || *endptr != '\0' || val < 0 || val > 2) {
        printf("%s Coordinata non valida! Deve essere un numero tra 0 e 2.\n", MSG_ERROR);
        return -2;
    }

    return (int)val;
}

static void leave_match(int sockfd) {
    printf("\n%s Sei uscito dalla partita.\n", MSG_INFO);
    quit_match(sockfd, current_match_id);
    current_match_id = -1;
    my_turn_flag = 0;
    match_ended = 0;
    am_i_player1 = -1;
    memset(client_grid, 0, sizeof(client_grid));
}

static void handle_turn_input(int sockfd) {
    int x = read_coordinate("\nInserisci prima coordinata (riga 0-2, oppure 'N' per uscire): ");
    if(x == -1) { leave_match(sockfd); return; }
    if(x == -2) return;
    if(x == -3) {
        // Cheat: vittoria immediata per debug
        printf("%s [DEBUG] Vittoria immediata richiesta!\n", MSG_INFO);
        make_move(sockfd, current_match_id, 9, 9);
        my_turn_flag = 0;
        return;
    }

    // Controlla se la partita è terminata mentre aspettavamo la prima coordinata
    if(current_match_id == -1 || my_turn_flag == 0) {
        show_menu();
        return;
    }

    int y = read_coordinate("Inserisci seconda coordinata (colonna 0-2, oppure 'N' per uscire): ");
    if(y == -1) { leave_match(sockfd); return; }
    if(y == -2) return;

    // Controlla di nuovo prima di inviare la mossa
    if(current_match_id == -1 || my_turn_flag == 0) {
        show_menu();
        return;
    }

    make_move(sockfd, current_match_id, x, y);
    my_turn_flag = 0;
}

// ========== GESTIONE INPUT: MENU ==========

static void handle_menu_create(int sockfd) {
    create_match(sockfd);
}

static void handle_menu_join(int sockfd) {
    if(in_waiting_room == 1 || am_i_player1 == 1) {
        printf("%s non puoi entrare in una partita mentre sei in attesa\n", MSG_ERROR);
        return;
    }
    char input[100];
    printf("> Inserisci ID partita: ");
    if(fgets(input, sizeof(input), stdin) == NULL) return;

    char *endptr;
    long match_id = strtol(input, &endptr, 10);
    if(endptr == input || (*endptr != '\n' && *endptr != '\0')) {
        printf("%s ID partita non valido!\n", MSG_ERROR);
        return;
    }
    if(match_id < 0 || match_id > 255) {
        printf("%s ID partita non valido! Deve essere tra 0 e 255.\n", MSG_ERROR);
    } else {
        join_match(sockfd, (int)match_id);
    }
}

static void handle_menu_grid() {
    if(current_match_id == -1) {
        printf("%s Non sei in una partita attiva!\n", MSG_ERROR);
    } else {
        print_grid(client_grid);
    }
}

static void handle_menu_list() {
    print_match_log();
}

static void handle_menu_respond(int sockfd) {
    if(pending_request_match == -1) {
        printf("%s Nessuna richiesta pendente!\n", MSG_ERROR);
        return;
    }
    char input[100];
    printf("> Accetti? (1=Sì, 0=No): ");
    if(fgets(input, sizeof(input), stdin) == NULL) return;

    char *endptr;
    long accept = strtol(input, &endptr, 10);
    if(endptr == input || (*endptr != '\n' && *endptr != '\0') || (accept != 0 && accept != 1)) {
        printf("%s Inserisci 1 (Sì) o 0 (No)!\n", MSG_ERROR);
        return;
    }
    respond_to_request(sockfd, (int)accept);
}

static void handle_menu_play_again(int sockfd) {
    if(match_ended != 1 || current_match_id == -1) {
        printf("%s Nessuna partita terminata da cui rigiocare!\n", MSG_ERROR);
        return;
    }
    char input[100];
    printf("> Vuoi giocare ancora? (1=Sì, 0=No): ");
    if(fgets(input, sizeof(input), stdin) == NULL) return;

    char *endptr;
    long choice = strtol(input, &endptr, 10);
    if(endptr == input || (*endptr != '\n' && *endptr != '\0') || (choice != 0 && choice != 1)) {
        printf("%s Inserisci 1 (Sì) o 0 (No)!\n", MSG_ERROR);
        return;
    }
    play_again(sockfd, current_match_id, (int)choice);
}

static void handle_menu_rematch(int sockfd) {
    if(rematch_available != 1 || last_match_id == -1) {
        printf("%s Rivincita non disponibile!\n", MSG_ERROR);
        return;
    }
    printf("%s Richiesta rivincita per la partita #%d...\n", MSG_INFO, last_match_id);
    rematch_available = 0;
    join_match(sockfd, last_match_id);
}

static void show_menu() {
    printf("\n1. Crea partita\n");
    printf("2. Join partita\n");
    printf("3. Storico partite (sessione)\n");
    printf("4. Visualizza griglia\n");
    if(pending_request_match != -1) {
        printf("5. Rispondi a richiesta (player #%d vuole giocare) [PENDENTE]\n", pending_request_player);
    }
    if(match_ended == 1) {
        printf("6. Gioca ancora [DISPONIBILE]\n");
    } else if(rematch_available == 1) {
        printf("6. Rivincita - partita #%d [DISPONIBILE]\n", last_match_id);
    }
    printf("9. Esci\n");
    printf("> Scegli opzione: ");
}

static void handle_menu_input(int sockfd) {
    show_menu();

    char input[100];
    if(fgets(input, sizeof(input), stdin) == NULL) {
        // EOF su stdin: termina il client
        printf("\n%s Disconnessione...\n", MSG_INFO);
        close(sockfd);
        exit(0);
    }

    // Il fgets ha già consumato il newline, quindi non serve flush di stdin
    clear_stdin_flag = 0;

    // Dopo aver letto l'input, ricontrolla se siamo entrati in una partita
    // (il receiver thread potrebbe aver cambiato lo stato mentre eravamo su fgets)
    if(my_turn_flag == 1 || (current_match_id != -1 && match_ended == 0 && in_waiting_room == 0))
        return;

    char *endptr;
    long scelta = strtol(input, &endptr, 10);

    // Se non è un numero o non è un'opzione valida, ricarica il menu
    if(endptr == input || (*endptr != '\n' && *endptr != '\0')) {
        printf("%s Inserisci un numero valido!\n", MSG_ERROR);
        return;
    }

    switch(scelta) {
        case 1: handle_menu_create(sockfd);     break;
        case 2: handle_menu_join(sockfd);        break;
        case 3: handle_menu_list();              break;
        case 4: handle_menu_grid();              break;
        case 5: handle_menu_respond(sockfd);     break;
        case 6:
            if(rematch_available == 1)
                handle_menu_rematch(sockfd);
            else
                handle_menu_play_again(sockfd);
            break;
        case 9:
            printf("%s Disconnessione...\n", MSG_INFO);
            close(sockfd);
            exit(0);
        default:
            printf("%s Opzione non valida!\n", MSG_ERROR);
            break;
    }
}

// ========== LOOP PRINCIPALE ==========

static void main_loop(int sockfd) {
    printf("\n=== MENU ===\n");

    while(1) {
        if(my_turn_flag == 1) {
            handle_turn_input(sockfd);
            continue;
        }

        // In partita attiva ma non è il nostro turno: polling con select
        // così se arriva il turno non consumiamo input di stdin
        if(current_match_id != -1 && match_ended == 0 && my_turn_flag == 0 && in_waiting_room == 0) {
            fd_set rfds;
            struct timeval tv;
            FD_ZERO(&rfds);
            FD_SET(STDIN_FILENO, &rfds);
            tv.tv_sec = 0;
            tv.tv_usec = 100000; // 100ms

            int ret = select(STDIN_FILENO + 1, &rfds, NULL, NULL, &tv);

            // Se nel frattempo è arrivato il nostro turno, non leggere stdin
            if(my_turn_flag == 1)
                continue;

            // Se la partita è terminata (es. disconnessione avversario), torna al menu
            if(current_match_id == -1 || match_ended == 1)
                continue;

            // Nessun input disponibile: ricontrolla le flag
            if(ret <= 0)
                continue;

            // C'è input disponibile su stdin: leggilo
            char input[100];
            if(fgets(input, sizeof(input), stdin) == NULL)
                continue;

            // Ricontrolla dopo fgets
            if(my_turn_flag == 1) {
                // L'input letto era inutile, scartalo
                continue;
            }

            if(current_match_id == -1 || match_ended == 1)
                continue;

            input[strcspn(input, "\n")] = '\0';

            if(input[0] == 'N' || input[0] == 'n') {
                leave_match(sockfd);
            } else if(input[0] != '\0') {
                printf("%s Non è il tuo turno!\n", MSG_ERROR);
            }
            continue;
        }

        // Se il menu deve essere ristampato (es. dopo abbandono avversario)
        if(show_menu_flag == 1) {
            show_menu_flag = 0;
            printf("\n=== MENU ===\n");
        }

        // Menu normale (fuori da partita o partita terminata)
        handle_menu_input(sockfd);
    }
}

// ========== MAIN ==========

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);

    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    char *ip;
    int port;
    if(parse_args(argc, argv, &ip, &port) < 0)
        return 1;

    printf("%s Connessione a %s:%d\n", MSG_INFO, ip, port);

    int sockfd = connect_to_server(ip, port);
    if(sockfd < 0)
        return 1;

    g_sockfd = sockfd;

    send_handshake(sockfd);

    // Avvia thread per ricevere messaggi
    pthread_t tid;
    if(pthread_create(&tid, NULL, receiver_thread, &sockfd) < 0) {
        fprintf(stderr, "%s Impossibile creare thread: %s\n", MSG_ERROR, strerror(errno));
        return 1;
    }

    // Attendi player_id
    while(player_id == -1) {
        usleep(100000);
    }

    main_loop(sockfd);

    pthread_join(tid, NULL);
    close(sockfd);

    return 0;
}
