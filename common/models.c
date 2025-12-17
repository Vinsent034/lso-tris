#include "models.h"
#include <stdlib.h>
#include <stdio.h>

MatchList *matches = NULL; // lista dei match di defoutl
short curr_matches_size = 0; //numero di partite attiva 

void add_match(Match *match) {
    MatchList *new_node = malloc(sizeof(MatchList)); // cro un nodo di match , allocando dimensione con la malloc
    new_node->val = match; // assegno al nuovo noo il valore della peritta presa in input
    new_node->next = matches; // poi assegno al nidi sucessivo il prissimo math(nel primo caso vuoto poiceh di default)
    matches = new_node; // infine aggiono la testa della lista come nuovo nodo
    curr_matches_size++; // incremento la dimensione delle parite attive
}


// funizione simile alla precedente ma nella rmozione
void remove_match(Match *match) { // prende in input la partita terminata da rimuovere 
    MatchList *curr = matches; // mette un puntarore al math corrente 
    MatchList *prev = NULL; // e uno di sicurezza al precdente (per casi in cui la perdevo)
    
    while(curr != NULL) { // se il math c'è
        if(curr->val == match) { // scorre la lista finche non trova la partita corrispondete 
            if(prev == NULL) { // verifica se la partita da rimuovere è la prima della lista
                matches = curr->next; // se è la prima aggiorna la testa della lista
            } else {
                prev->next = curr->next;// altrimenti cilleghiamo il precedente col successivo
            }
            free(curr); // dealloc con la free free 
            curr_matches_size--; // decremento il contatore delle partite attive
            return; // termino
        }
        prev = curr; // aggiorno il nodo prcedente 
        curr = curr->next;  // e scrro al prissimo math
    }
}

Match *get_match_by_id(MatchList *head, int id) { //prende la lista dei match diponibili e un id della paritata  per trovarla
    MatchList *curr = head; // asegnamo il puntatore alla lista
    while(curr != NULL) { //  scoore la lsita finch non è vipta 
        if(curr->val->id == id) { // verifica se l'id della partita corripinde con quella del nodo
            return curr->val; 
        }
        curr = curr->next; 
    }
    return NULL; // in caso di fallimennto termino
}

int find_free_id() { //funzione per trovare un id libero 
    for(int id = 0; id < MAX_MATCHES; id++) { // verifico in tutti gliid 
        int found = 0; // flago per vedere se un id e occupato
        MatchList *curr = matches; // creo un puntatore per salvare la lista delle partte 
        while(curr != NULL) { 
            if(curr->val->id == id) { // se trovo una parita con l'id che sto cercando mi fermo
                found = 1;
                break;
            }
            curr = curr->next;
        }
        if(!found) return id; // se ho trovato un id libero lo ritorno
    }
    return -1; // in caso non ci sono id liberi -1
}

void add_requester(Match *match, RequestNode *node) { // funzione per aggiungere una richiesta di partecipazione alla partita
    node->next = NULL; // inizio col'impostare che l'elemnto sucessivo è null
    if(match->requests_tail == NULL) { // se la coda è vuota
        match->requests_head = node; // allora la testa e coda puntano allo stesso nodo
    } else {
        match->requests_tail->next = node; //altrimenti la cosa punta all'elemnto successivo
    }
    match->requests_tail = node; // infine aggiorno la cosa 
}

void delete_from_head(Match *match) { // funzione per rimuovere la richiesta nella lista
    if(match->requests_head == NULL) return; //se la testa della richiesta è vuota termino
    RequestNode *temp = match->requests_head; //altrimenti salvo la testa in un nodo temporaneo
    match->requests_head = match->requests_head->next; //scorro al nodo sucessivo
    if(match->requests_head == NULL) { // se il macth e nullo allora impongo anche la coda nulla
        match->requests_tail = NULL;
    }
    free(temp); // dealloco 
}