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
    
    // Test: invia un messaggio
    char *test_msg = "HELLO SERVER";
    send(sockfd, test_msg, strlen(test_msg), 0);
    printf("%s Messaggio inviato: %s\n", MSG_DEBUG, test_msg);

    sleep(2);
    
    close(sockfd);
    printf("%s Disconnesso\n", MSG_INFO);
    
    return 0;
}