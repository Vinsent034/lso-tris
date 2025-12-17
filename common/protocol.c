#include "protocol.h"
#include "models.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <stdlib.h>


// qyesta funzione prende le struct di Pkect li converte in byte array e li invia 
// La conversione avvinee in base al tipoi di id corrispondente col pachetto definito in protocol.h 



void send_packet(int sockfd, Packet *packet) { // funzione invio del pachetto , prnde in input il tipo di file con cui andremo a laovrere e il pachetto ceh deve essere dato
    char serialized[65536 + 3]; //buffer di serilizzazione (la dimensione masisma di un paketto piu i tre byte per dimensione  ) = 2 byte 65536
    serialized[0] = packet->id; // il primo byte è sempre l'id del pachetto
    
    //  SERVER PACKETS   (ogni pachetto in base alla define corripondente avrà la sua serializazione )
    if(packet->id == SERVER_HANDSHAKE) { // caso pachetto handhake
        packet->size = 1; // la domnsione del pachetto è  di 1 byte
        serialized[3] = ((Server_Handshake *)packet->content)->player_id; //assegno il valroe del' ide del player alla posizione 3 del buffer
    }
    
    if(packet->id == SERVER_SUCCESS || packet->id == SERVER_ERROR || packet->id == SERVER_INVALID_MOVE) { //gestione dei tre casi // ma perchè in questo moddo
        packet->size = 0; // la dimensione sara 0
    }
    
    if(packet->id == SERVER_MATCHREQUEST) { // richiesta di unirsi alla prtita
        packet->size = 2;
        serialized[3] = ((Server_MatchRequest *)packet->content)->other_player; // id del giocatore ceh richiede
        serialized[4] = ((Server_MatchRequest *)packet->content)->match; // id della partita
    }
    
    if(packet->id == SERVER_NOTICESTATE) { // notifica dello stato della parttita
        packet->size = 2;
        serialized[3] = ((Server_NoticeState *)packet->content)->state; //asegno lo stto della peritta
        serialized[4] = ((Server_NoticeState *)packet->content)->match; //ide della pertita
    }
    
    if(packet->id == SERVER_NOTICEMOVE) { // notifica della mossa dell'avversario
        packet->size = 3; // dimensione di 3 byte
        serialized[3] = ((Server_NoticeMove *)packet->content)->moveX; //salvo la cordinat x
        serialized[4] = ((Server_NoticeMove *)packet->content)->moveY; // salvo la cordinata y
        serialized[5] = ((Server_NoticeMove *)packet->content)->match; // id 
    }
    
    if(packet->id == SERVER_BROADCASTMATCH) { //brodcast per la nuova partita
        packet->size = 2; // dimensione di 2 byte
        serialized[3] = ((Server_BroadcastMatch *)packet->content)->player_id; // id del giocatore che ha creato la partita
        serialized[4] = ((Server_BroadcastMatch *)packet->content)->match; // ide del amtch
    }
    
    if(packet->id == SERVER_UPDATEONREQUEST) { // aggiotnamento per la richiesta di partecipare
        packet->size = 2; //ridimensionamento
        serialized[3] = ((Server_UpdateOnRequest *)packet->content)->accepted; // partita acettata
        serialized[4] = ((Server_UpdateOnRequest *)packet->content)->match; 
    }
    















    //CLIENT PACKETS  (stessa cosa ma oer i client)
    if(packet->id == CLIENT_HANDSHAKE || packet->id == CLIENT_CREATEMATCH) { // pachetto handshake o creazione della partita
        packet->size = 0;
    }
    
    if(packet->id == CLIENT_JOINMATCH) { // pachetto per unirsi alla partita
        packet->size = 1;
        serialized[3] = ((Client_JoinMatch *)packet->content)->match; 
    }
    
    if(packet->id == CLIENT_MODIFYREQUEST) { // pachetto per modificare la richiesta di partecipazione
        packet->size = 2; 
        serialized[3] = ((Client_ModifyRequest *)packet->content)->accepted; // accettazione
        serialized[4] = ((Client_ModifyRequest *)packet->content)->match;
    }
    
    if(packet->id == CLIENT_MAKEMOVE) { // pachetto per fare una mossa
        packet->size = 3; 
        serialized[3] = ((Client_MakeMove *)packet->content)->moveX;
        serialized[4] = ((Client_MakeMove *)packet->content)->moveY;
        serialized[5] = ((Client_MakeMove *)packet->content)->match;
    }
    
    if(packet->id == CLIENT_PLAYAGAIN) { // pachetto per richiedere di giocare ancora
        packet->size = 2;
        serialized[3] = ((Client_PlayAgain *)packet->content)->choice;
        serialized[4] = ((Client_PlayAgain *)packet->content)->match;
    }

    if(packet->id == CLIENT_QUITMATCH) { // pachetto per uscire dalla partita
        packet->size = 1;
        serialized[3] = ((Client_QuitMatch *)packet->content)->match;
    }

    //scrivo la dimensione del pachetto nei due byte successivi all'id
    serialized[1] = packet->size & 0xFF; 
    serialized[2] = (packet->size >> 8) & 0xFF;
    
    // +3 in quanto un byte e per la diemnsione e 2 sono per l'id 
    if(send(sockfd, serialized, packet->size + 3, 0) < 1) { // invio del pachetto
        fprintf(stderr, "%s Errore invio pacchetto (fd=%d, id=%d): %s\n", 
                MSG_ERROR, sockfd, packet->id, strerror(errno));
    }
}




















// Funzione per deserializare il pahcetto ricevuto
// nota lo faccio void peche puo ritonare doversi tipi in base al packed id 


void *serialize_packet(Packet *packet) { // funzione per serializare il pachetto
    //CLIENT PACKETS (ogni pachetto ha una dimensione precisa quindi verifichiamo che la dimnsione corrsponda altrimenti lo scartiamo)
    
    if(packet->id == CLIENT_JOINMATCH) { // pachetto per unirsi alla partita
        if(packet->size < 1) return NULL;
        Client_JoinMatch *new = malloc(sizeof(Client_JoinMatch)); // alloco la memoria per la nuova struttura
        new->match = ((char *)packet->content)[0]; // assegno il valore dell'id della partita
        return new;
    }
    
    if(packet->id == CLIENT_MODIFYREQUEST) {
        if(packet->size < 2) return NULL;
        Client_ModifyRequest *new = malloc(sizeof(Client_ModifyRequest));
        new->accepted = ((char *)packet->content)[0]; //
        new->match = ((char *)packet->content)[1];
        return new;
    }
    
    if(packet->id == CLIENT_MAKEMOVE) {
        if(packet->size < 3) return NULL;
        Client_MakeMove *new = malloc(sizeof(Client_MakeMove));
        new->moveX = ((char *)packet->content)[0];
        new->moveY = ((char *)packet->content)[1];
        new->match = ((char *)packet->content)[2];
        return new;
    }
    
    if(packet->id == CLIENT_PLAYAGAIN) {
        if(packet->size < 2) return NULL;
        Client_PlayAgain *new = malloc(sizeof(Client_PlayAgain));
        new->choice = ((char *)packet->content)[0];
        new->match = ((char *)packet->content)[1];
        return new;
    }

    if(packet->id == CLIENT_QUITMATCH) {
        if(packet->size < 1) return NULL;
        Client_QuitMatch *new = malloc(sizeof(Client_QuitMatch));
        new->match = ((char *)packet->content)[0];
        return new;
    }


























    // ERVER PACKETS 
    if(packet->id == SERVER_HANDSHAKE) {
        if(packet->size < 1) return NULL;
        Server_Handshake *new = malloc(sizeof(Server_Handshake));
        new->player_id = ((char *)packet->content)[0];
        return new;
    }
    
    if(packet->id == SERVER_MATCHREQUEST) {
        if(packet->size < 2) return NULL;
        Server_MatchRequest *new = malloc(sizeof(Server_MatchRequest));
        new->other_player = ((char *)packet->content)[0];
        new->match = ((char *)packet->content)[1];
        return new;
    }
    
    if(packet->id == SERVER_UPDATEONREQUEST) {
        if(packet->size < 2) return NULL;
        Server_UpdateOnRequest *new = malloc(sizeof(Server_UpdateOnRequest));
        new->accepted = ((char *)packet->content)[0];
        new->match = ((char *)packet->content)[1];
        return new;
    }
    
    if(packet->id == SERVER_NOTICESTATE) {
        if(packet->size < 2) return NULL;
        Server_NoticeState *new = malloc(sizeof(Server_NoticeState));
        new->state = ((char *)packet->content)[0];
        new->match = ((char *)packet->content)[1];
        return new;
    }
    
    if(packet->id == SERVER_NOTICEMOVE) {
        if(packet->size < 3) return NULL;
        Server_NoticeMove *new = malloc(sizeof(Server_NoticeMove));
        new->moveX = ((char *)packet->content)[0];
        new->moveY = ((char *)packet->content)[1];
        new->match = ((char *)packet->content)[2];
        return new;
    }
    
    if(packet->id == SERVER_BROADCASTMATCH) {
        if(packet->size < 2) return NULL;
        Server_BroadcastMatch *new = malloc(sizeof(Server_BroadcastMatch));
        new->player_id = ((char *)packet->content)[0];
        new->match = ((char *)packet->content)[1];
        return new;
    }
    
    return NULL; // ritonera null solo se la dimensione del pachetto non è corretta 
}















