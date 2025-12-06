#include "../common/models.h"
#include "../common/protocol.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>

#define DEFAULT_IP      "127.0.0.1"
#define DEFAULT_PORT    5555
#define BUFFER_SIZE     1024

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    
    char *ip = DEFAULT_IP;
    int port = DEFAULT_PORT;
    int sockfd = 0;
    struct sockaddr_in address;

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
    
    printf("%s Handshake inviato\n", MSG_DEBUG);
    
    // Ricevi risposta
    char buffer[BUFFER_SIZE];
    ssize_t received = recv(sockfd, buffer, BUFFER_SIZE, 0);
    if(received > 0) {
        Packet *packet = malloc(sizeof(Packet));
        packet->id = buffer[0];
        packet->size = buffer[1] + (buffer[2] << 8);
        packet->content = malloc(sizeof(char) * packet->size);
        memcpy(packet->content, buffer + 3, packet->size);
        
        if(packet->id == SERVER_HANDSHAKE) {
            Server_Handshake *response = (Server_Handshake *)serialize_packet(packet);
            if(response != NULL) {
                printf("%s Ricevuto player_id=%d dal server!\n", MSG_INFO, response->player_id);
                free(response);
            }
        }
        
        free(packet->content);
        free(packet);
    }
    
    sleep(1);
    
    close(sockfd);
    printf("%s Disconnesso\n", MSG_INFO);
    
    return 0;
}