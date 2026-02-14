#include "models.h"
#include <stdlib.h>
#include <stdio.h>

MatchList *matches = NULL;
short curr_matches_size = 0;

void add_match(Match *match) { // aggiunge una partita alla lista , inserimento in testa 
    MatchList *new_node = malloc(sizeof(MatchList));
    new_node->val = match;
    new_node->next = matches;
    matches = new_node;
    curr_matches_size++;
}

void remove_match(Match *match) { // elimina un partita dalla lista , rimozione di tipo n position
    MatchList *curr = matches;
    MatchList *prev = NULL;
    
    while(curr != NULL) {
        if(curr->val == match) {
            if(prev == NULL) {
                matches = curr->next;
            } else {
                prev->next = curr->next;
            }
            free(curr);
            curr_matches_size--;
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}

Match *get_match_by_id(MatchList *head, int id) {
    MatchList *curr = head;
    while(curr != NULL) {
        if(curr->val->id == id) {
            return curr->val;
        }
        curr = curr->next;
    }
    return NULL;
}

int find_free_id() { // trova una partita che non e occupato
    for(int id = 0; id < MAX_MATCHES; id++) {
        int found = 0;
        MatchList *curr = matches;
        while(curr != NULL) {
            if(curr->val->id == id) {
                found = 1;
                break;
            }
            curr = curr->next;
        }
        if(!found) return id;
    }
    return -1;
}

void add_requester(Match *match, RequestNode *node) { // funzione che serve per effettuare una richiesta alla partita
    node->next = NULL;
    if(match->requests_tail == NULL) {
        match->requests_head = node;
    } else {
        match->requests_tail->next = node;
    }
    match->requests_tail = node;
}

void delete_from_head(Match *match) {
    if(match->requests_head == NULL) return;
    RequestNode *temp = match->requests_head;
    match->requests_head = match->requests_head->next;
    if(match->requests_head == NULL) {
        match->requests_tail = NULL;
    }
    free(temp);
}