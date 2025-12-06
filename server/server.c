#include "structures.h"
#include "connection.h"
#include "../common/models.h"
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define PORT 5555
#define MAX_CLIENTS 255

void init_socket(int port) {
    int sockfd = 0;
    int opt = 1;
    struct sockaddr_in address;
    int conn = 0;
    socklen_t addrlen = sizeof(address);
    pthread_t threads[MAX_CLIENTS];
    int thread_count = 0;

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "%s Impossibile inizializzare socket: %s\n", MSG_ERROR, strerror(errno));
        exit(1);
    }

    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        fprintf(stderr, "%s Impossibile configurare socket: %s\n", MSG_ERROR, strerror(errno));
        exit(1);
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if(bind(sockfd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        fprintf(stderr, "%s Impossibile eseguire bind: %s\n", MSG_ERROR, strerror(errno));
        exit(1);
    }

    if(listen(sockfd, 3) < 0) {
        fprintf(stderr, "%s Impossibile eseguire listen: %s\n", MSG_ERROR, strerror(errno));
        exit(1);
    }

    printf("%s Server in ascolto sulla porta %d...\n", MSG_INFO, port);

    while(1) {
        if((conn = accept(sockfd, (struct sockaddr*)&address, &addrlen)) < 0) {
            fprintf(stderr, "%s Impossibile accettare connessione: %s\n", MSG_ERROR, strerror(errno));
        } else {
            if(curr_clients_size < MAX_CLIENTS) {
                printf("%s Connessione accettata (fd=%d)\n", MSG_INFO, conn);

                Client *new_client = malloc(sizeof(Client));
                new_client->conn = conn;
                new_client->addr = address;

                Player *player = malloc(sizeof(Player));
                player->id = -1;
                player->busy = 0;
                new_client->player = player;

                curr_clients_size++;



                // Aggiungi client alla lista
                ClientNode *node = malloc(sizeof(ClientNode));
                node->val = new_client;
                node->next = clients;
                clients = node;





                if(pthread_create(&threads[thread_count], NULL, server_thread, (void *)new_client) < 0) {
                    fprintf(stderr, "%s Impossibile creare thread: %s\n", MSG_ERROR, strerror(errno));
                    close(conn);
                    free(new_client->player);
                    free(new_client);
                    curr_clients_size--;
                    continue;
                }

                JoinerThreadArgs *args = malloc(sizeof(JoinerThreadArgs));
                args->client = new_client;
                args->thread = threads[thread_count];

                pthread_t joiner_tid;
                if(pthread_create(&joiner_tid, NULL, joiner_thread, (void *)args) < 0) {
                    fprintf(stderr, "%s Impossibile creare joiner thread: %s\n", MSG_ERROR, strerror(errno));
                }

                pthread_detach(joiner_tid);
                thread_count++;
            } else {
                close(conn);
                printf("%s Limite massimo client raggiunto (%d)\n", MSG_WARNING, MAX_CLIENTS);
            }
        }
    }

    close(sockfd);
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    
    int port = PORT;
    printf("=== Tris Server ===\n\n");

    if(argc == 2) {
        if(sscanf(argv[1], "%d", &port) <= 0 || port < 0 || port > 65535) {
            fprintf(stderr, "%s Porta non valida: %s\n", MSG_ERROR, argv[1]);
            return 1;
        }
    }

    printf("%s Porta: %d\n", MSG_INFO, port);
    init_socket(port);
    
    return 0;
}