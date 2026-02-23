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
#include <signal.h>

#define PORT 5555
#define MAX_CLIENTS 255

void init_socket(int port) {
    // varibili per tenere conto delle informazioni della socket 
    int sockfd = 0;  // contiene le informazione del file descriptor 
    int opt = 1; // serve per abilitare la socket
    struct sockaddr_in address; // risorsa presa da <netinet/in.h> , mi permette di  avere gia una struttura preimpostata 
    int conn = 0;
    socklen_t addrlen = sizeof(address);  // <sys/socket.h>, serve a prendere la dimensione dell'indirizzo 
    pthread_t threads[MAX_CLIENTS]; // <pthread.h> serve a tenere conto dei thread usati
    int thread_count = 0; 

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { // crea una socket , indicadno la famiglia dell'indirizzo e indicando che è orientata allo stream
        fprintf(stderr, "%s Impossibile inizializzare socket: %s\n", MSG_ERROR, strerror(errno));
        exit(1);
    }

    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {// funzione importtata che mi peremette di riutilizare la porta eventualmente riavvio il server, usato  SO_REUSEADDR ,adtto per i test attuali per riavvi contunui e veloci 
        fprintf(stderr, "%s Impossibile configurare socket: %s\n", MSG_ERROR, strerror(errno));
        exit(1);
    }
    


    // imposto la famiglia dell'indirizzo di rete da cui il server ascolta 
    address.sin_family = AF_INET; // impongo la categirua di appartenenza
    address.sin_addr.s_addr = INADDR_ANY; // imposto l'indirizzo ip da dove deve ascoltare 
    address.sin_port = htons(port); // impsotp il numero della porta da dove devo ascoltare

    if(bind(sockfd, (struct sockaddr*)&address, sizeof(address)) < 0) { // tramite la bind posso indico che la socket deve ruspondere a un determianta porta, se ka porta specificata non e collegata con la socket allora la porta è occupata
        fprintf(stderr, "%s Impossibile eseguire bind: %s\n", MSG_ERROR, strerror(errno));
        exit(1);
    }

    if(listen(sockfd, 3) < 0) { // con la listen faccio in modod che la socket diventi di tipo passivo ovvero si metta ina scolto per ricever eeventiali richieste , se non le rieve da errore
        exit(1);
    }

    printf("%s Server in ascolto sulla porta %d...\n", MSG_INFO, port);

    while(1) { // di preassi aspetta un client , se arriva crea un thread lo assegna al client e infine ritorna punto e capo per altri client
        if((conn = accept(sockfd, (struct sockaddr*)&address, &addrlen)) < 0) { // quando crea un client , crea un file descriptor , se viene creata con sucesso perfetto vado avanti altriemtni no
            fprintf(stderr, "%s Impossibile accettare connessione: %s\n", MSG_ERROR, strerror(errno));
        } else {
            if(curr_clients_size < MAX_CLIENTS) { // prima di accettare un client verifico se ho spazzio
                printf("%s Connessione accettata (fd=%d)\n", MSG_INFO, conn);

                //alloco le risorse oer creare un client (compreso i settaggi per il player)
                Client *new_client = malloc(sizeof(Client));
                new_client->conn = conn;
                new_client->addr = address;

                Player *player = malloc(sizeof(Player));
                player->id = -1;
                player->busy = 0;
                new_client->player = player;

                curr_clients_size++;



                // cre un nodo Client per salvaer il cliente nella lista
                ClientNode *node = malloc(sizeof(ClientNode));
                node->val = new_client;
                node->next = clients;
                clients = node;




                // verifico se posso creare un thread , in caso positivo non assgna nulla perchè non c nee sono e dellaca il client creato prima 
                if(pthread_create(&threads[thread_count], NULL, server_thread, (void *)new_client) < 0) {
                    fprintf(stderr, "%s Impossibile creare thread: %s\n", MSG_ERROR, strerror(errno));
                    close(conn);
                    free(new_client->player);
                    free(new_client);
                    curr_clients_size--;
                    continue;
                }

                // assegno il thread al cliente 
                JoinerThreadArgs *args = malloc(sizeof(JoinerThreadArgs));
                args->client = new_client; // per tenre traccia del cleint asseganto 
                args->thread = threads[thread_count]; 


                pthread_t joiner_tid; // questo secondo thread passa ulteriori informazioni sul thread principale e client
                if(pthread_create(&joiner_tid, NULL, joiner_thread, (void *)args) < 0) { // crea un thread
                    fprintf(stderr, "%s Impossibile creare joiner thread: %s\n", MSG_ERROR, strerror(errno));
                }

                pthread_detach(joiner_tid); // si libera del thread con la usa morte
                thread_count++; // aggiunge un thrad disponibilie
            } else {
                close(conn);
                printf("%s Limite massimo client raggiunto (%d)\n", MSG_WARNING, MAX_CLIENTS);
            }
        }
    }

    close(sockfd); // qunado termina il server chiudo tutto
    
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    signal(SIGPIPE, SIG_IGN); // Ignora SIGPIPE: send() su fd chiuso ritorna errore invece di crashare

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