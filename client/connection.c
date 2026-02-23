#include "connection.h"
#include "../common/protocol.h"
#include "../common/models.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

// ========== STORICO PARTITE (LOG) ==========

#define MATCH_STATUS_ACTIVE  0
#define MATCH_STATUS_ENDED   1

typedef struct MatchLogEntry {
    int match_id;
    int owner_id;
    int status;
    struct MatchLogEntry *next;
} MatchLogEntry;

static MatchLogEntry *match_log = NULL;

static void log_match_created(int match_id, int owner_id) {
    MatchLogEntry *cur = match_log;
    while (cur != NULL) {
        if (cur->match_id == match_id) {
            cur->status = MATCH_STATUS_ACTIVE;
            cur->owner_id = owner_id;
            return;
        }
        cur = cur->next;
    }
    MatchLogEntry *node = malloc(sizeof(MatchLogEntry));
    node->match_id = match_id;
    node->owner_id = owner_id;
    node->status   = MATCH_STATUS_ACTIVE;
    node->next     = match_log;
    match_log      = node;
}

static void log_match_ended(int match_id) {
    MatchLogEntry *cur = match_log;
    while (cur != NULL) {
        if (cur->match_id == match_id) {
            cur->status = MATCH_STATUS_ENDED;
            return;
        }
        cur = cur->next;
    }
}

void print_match_log() {
    printf("\n=== STORICO PARTITE (sessione corrente) ===\n");
    if (match_log == NULL) {
        printf("  Nessuna partita vista finora.\n");
        printf("===========================================\n");
        return;
    }
    printf("  %-6s  %-10s  %s\n", "ID", "OWNER", "STATO");
    printf("  ------  ----------  -----------\n");
    int total = 0, active = 0;
    MatchLogEntry *cur = match_log;
    while (cur != NULL) {
        const char *status_str = (cur->status == MATCH_STATUS_ACTIVE)
                                 ? "DISPONIBILE" : "TERMINATA  ";
        printf("  #%-5d  player #%-3d  %s\n", cur->match_id, cur->owner_id, status_str);
        if (cur->status == MATCH_STATUS_ACTIVE) active++;
        total++;
        cur = cur->next;
    }
    printf("  ------  ----------  -----------\n");
    printf("  Totale: %d | Attive: %d\n", total, active);
    printf("===========================================\n");
}

// ========== LISTA PARTITE DISPONIBILI ==========

typedef struct AvailableMatch {
    int match_id;
    int owner_id;
    struct AvailableMatch *next;
} AvailableMatch;

static AvailableMatch *available_matches = NULL;

// Mutex per proteggere accesso concorrente alla lista delle partite disponibili
static pthread_mutex_t available_matches_mutex = PTHREAD_MUTEX_INITIALIZER;

static void add_available_match(int match_id, int owner_id) {
    pthread_mutex_lock(&available_matches_mutex);
    // Controlla duplicati
    AvailableMatch *cur = available_matches;
    while(cur != NULL) {
        if(cur->match_id == match_id) {
            pthread_mutex_unlock(&available_matches_mutex);
            return;
        }
        cur = cur->next;
    }
    AvailableMatch *node = malloc(sizeof(AvailableMatch));
    node->match_id = match_id;
    node->owner_id = owner_id;
    node->next = available_matches;
    available_matches = node;
    pthread_mutex_unlock(&available_matches_mutex);
}

static void remove_available_match(int match_id) {
    pthread_mutex_lock(&available_matches_mutex);
    AvailableMatch **prev = &available_matches;
    while(*prev != NULL) {
        if((*prev)->match_id == match_id) {
            AvailableMatch *to_free = *prev;
            *prev = (*prev)->next;
            free(to_free);
            pthread_mutex_unlock(&available_matches_mutex);
            return;
        }
        prev = &(*prev)->next;
    }
    pthread_mutex_unlock(&available_matches_mutex);
}

void print_available_matches() {
    pthread_mutex_lock(&available_matches_mutex);
    printf("\n=== PARTITE DISPONIBILI ===\n");
    if(available_matches == NULL) {
        printf("Nessuna partita disponibile al momento.\n");
        pthread_mutex_unlock(&available_matches_mutex);
        return;
    }
    int count = 0;
    AvailableMatch *cur = available_matches;
    while(cur != NULL) {
        printf("  #%d  (proprietario: player #%d)\n", cur->match_id, cur->owner_id);
        count++;
        cur = cur->next;
    }
    printf("Totale: %d partita/e disponibile/i\n", count);
    pthread_mutex_unlock(&available_matches_mutex);
}

// ========== VARIABILI GLOBALI ==========
int player_id = -1;
char client_grid[3][3];
int current_match_id = -1;
int current_state = -1;
int match_ended = 0;
int am_i_player1 = -1;
int my_turn_flag = 0;
int clear_stdin_flag = 0;
int pending_request_player = -1;
int pending_request_match = -1;
int show_menu_flag = 0;
int in_waiting_room = 0;
int last_match_id = -1;      // ID dell'ultima partita persa (per rivincita)
int rematch_available = 0;   // 1 quando il vincitore ha riaperto la partita e possiamo chiedere rivincita

static int cached_sockfd = -1; // fd salvato per inviare pacchetti dagli handler

// ========== FUNZIONI DI UTILITÀ ==========

static void display_grid() {
    printf("     0   1   2\n");
    printf("   +---+---+---+\n");
    for(int i = 0; i < 3; i++) {
        printf(" %d |", i);
        for(int j = 0; j < 3; j++) {
            char c = client_grid[i][j];
            if(c == 0) c = ' ';
            printf(" %c |", c);
        }
        printf("\n   +---+---+---+\n");
    }
}

static void reset_match_state() {
    current_match_id = -1;
    match_ended = 0;
    my_turn_flag = 0;
    am_i_player1 = -1;
    in_waiting_room = 0;
    last_match_id = -1;
    rematch_available = 0;
    memset(client_grid, 0, sizeof(client_grid));
    // Libera la lista delle partite disponibili in modo thread-safe
    pthread_mutex_lock(&available_matches_mutex);
    AvailableMatch *cur = available_matches;
    while(cur != NULL) {
        AvailableMatch *to_free = cur;
        cur = cur->next;
        free(to_free);
    }
    available_matches = NULL;
    pthread_mutex_unlock(&available_matches_mutex);
}

// ========== HANDLER PER SINGOLI PACCHETTI ==========

static void handle_handshake(void *serialized) {
    if(serialized == NULL) return;

    Server_Handshake *response = (Server_Handshake *)serialized;
    player_id = response->player_id;

    if(DEBUG) {
        printf("%s Assegnato player_id=%d dal server\n", MSG_DEBUG, player_id);
    }
}

static void handle_success() {
    printf("\n%s Operazione completata con successo\n", MSG_INFO);
}

static void handle_error() {
    // Se siamo già in waiting room come proprietari, ignora l'errore:
    // è la risposta al CLIENT_PLAYAGAIN automatico inviato da handle_win_state
    // che il server ha già gestito direttamente riaprendo la partita.
    if(in_waiting_room == 1) return;

    printf("\n%s Errore dal server\n", MSG_ERROR);

    // Se eravamo in attesa di rivincita e il server rifiuta,
    // l'avversario è già occupato: resetta lo stato
    if(match_ended == 1 && current_match_id != -1) {
        printf("%s L'avversario non è più disponibile. Rivincita annullata.\n", MSG_INFO);
        printf("%s Ritorno al menu principale...\n\n", MSG_INFO);
        reset_match_state();

        // Stampa il menu direttamente perché il main thread
        // potrebbe essere bloccato su fgets
        printf("\n=== MENU ===\n");
        printf("\n1. Crea partita\n");
        printf("2. Join partita\n");
        printf("3. Lista partite disponibili\n");
        printf("4. Visualizza griglia\n");
        printf("9. Esci\n");
        printf("> Scegli opzione: ");
    }
}

static void handle_invalid_move() {
    printf("\n%s ================================\n", MSG_ERROR);
    printf("%s COORDINATA GIÀ OCCUPATA!\n", MSG_ERROR);
    printf("%s ================================\n", MSG_ERROR);
    printf("%s Scegli un'altra casella\n\n", MSG_INFO);

    // Mostra la griglia corrente
    display_grid();

    // Riabilita my_turn_flag per chiedere di nuovo le coordinate
    my_turn_flag = 1;
    clear_stdin_flag = 0;
}

static void handle_match_request(void *serialized) {
    if(serialized == NULL) return;

    Server_MatchRequest *req = (Server_MatchRequest *)serialized;
    pending_request_player = req->other_player;
    pending_request_match = req->match;

    printf("\n\n%s ===============================================\n", MSG_INFO);
    printf("%s RICHIESTA DI GIOCO!\n", MSG_INFO);
    printf("%s Il player #%d vuole unirsi alla tua partita #%d\n",
           MSG_INFO, req->other_player, req->match);
    printf("%s Usa l'opzione 5 del menu per accettare o rifiutare\n", MSG_INFO);
    printf("%s ===============================================\n\n", MSG_INFO);
}

static void handle_update_on_request(void *serialized) {
    if(serialized == NULL) return;

    Server_UpdateOnRequest *update = (Server_UpdateOnRequest *)serialized;
    if(update->accepted) {
        printf("\n%s La tua richiesta è stata ACCETTATA! Partita #%d inizia!\n",
               MSG_INFO, update->match);
    } else {
        printf("\n%s La tua richiesta è stata RIFIUTATA per la partita #%d\n",
               MSG_ERROR, update->match);
    }
}

static void handle_broadcast_match(void *serialized) {
    if(serialized == NULL) return;

    Server_BroadcastMatch *bc = (Server_BroadcastMatch *)serialized;

    if(bc->player_id == -1) {
        // Partita terminata: rimuovi dalla lista disponibili e aggiorna il log
        remove_available_match(bc->match);
        log_match_ended(bc->match);
        // Se era la partita per cui aspettavamo la rivincita, annullala
        if(bc->match == last_match_id && rematch_available == 1) {
            rematch_available = 0;
            last_match_id = -1;
            printf("%s Il vincitore ha rifiutato la rivincita. Opzione 6 rimossa.\n", MSG_INFO);
        }
        // Se siamo noi i partecipanti, già gestito da SERVER_NOTICESTATE
        if(current_match_id == bc->match) return;
        printf("%s Partita #%d non più disponibile\n", MSG_INFO, bc->match);
    } else {
        // Ignora il broadcast della nostra stessa partita riaperta
        if(bc->match == current_match_id && bc->player_id == player_id) return;
        add_available_match(bc->match, bc->player_id);
        log_match_created(bc->match, bc->player_id);
        // Se è la partita che abbiamo perso e il vincitore l'ha riaperta, abilita rivincita
        if(bc->match == last_match_id && last_match_id != -1) {
            rematch_available = 1;
            printf("\n%s Il vincitore ha riaperto la partita #%d! Usa l'opzione 6 per la rivincita.\n",
                   MSG_INFO, bc->match);
        } else {
            printf("\n%s Nuova partita disponibile: #%d (player #%d)\n",
                   MSG_INFO, bc->match, bc->player_id);
        }
    }
}

static void handle_turn_state(int state, int match_id) {
    // Se match_ended era 1 e ora riceviamo TURN, significa che è un riavvio
    if(match_ended == 1 && state == STATE_TURN_PLAYER1) {
        printf("\n%s NUOVA PARTITA! Match #%d riavviato\n", MSG_INFO, match_id);
        printf("=========================\n");
        memset(client_grid, 0, sizeof(client_grid));
        match_ended = 0;
    }
    // Se eravamo in waiting room, è arrivato un nuovo avversario
    if(in_waiting_room == 1) {
        printf("\n%s Nuovo avversario trovato! Match #%d inizia!\n", MSG_INFO, match_id);
        printf("=========================\n");
        in_waiting_room = 0;
    }

    printf("\n%s Partita #%d - %s\n", MSG_INFO, match_id,
           (state == STATE_TURN_PLAYER1) ? "Turno Player 1 (X)" : "Turno Player 2 (O)");

    // Visualizza griglia
    display_grid();

    // Indica se è il tuo turno
    if((state == STATE_TURN_PLAYER1 && am_i_player1 == 1) ||
       (state == STATE_TURN_PLAYER2 && am_i_player1 == 0)) {
        my_turn_flag = 1;
        clear_stdin_flag = 0;
        printf("\n%s ================================\n", MSG_INFO);
        printf("%s È IL TUO TURNO!\n", MSG_INFO);
        printf("%s ================================\n", MSG_INFO);
    } else {
        my_turn_flag = 0;
        printf("%s In attesa della mossa dell'avversario...\n", MSG_INFO);
    }
}

static void handle_win_state(int match_id) {
    printf("\n%s Partita #%d - Risultato finale:\n", MSG_INFO, match_id);
    display_grid();
    printf("\n%s HAI VINTO! Sei il nuovo proprietario della partita. In attesa di un nuovo avversario...\n", MSG_INFO);
    printf("=========================\n");
    my_turn_flag = 0;

    // Invia automaticamente CLIENT_PLAYAGAIN con choice=1: diventa il proprietario
    if(cached_sockfd != -1) {
        Client_PlayAgain *play = malloc(sizeof(Client_PlayAgain));
        play->choice = 1;
        play->match = match_id;

        Packet *pkt = malloc(sizeof(Packet));
        pkt->id = CLIENT_PLAYAGAIN;
        pkt->content = play;

        send_packet(cached_sockfd, pkt);

        free(pkt);
        free(play);
    }

    // Entra in waiting room come proprietario
    match_ended = 0;
    am_i_player1 = 1;
    in_waiting_room = 1;
    memset(client_grid, 0, sizeof(client_grid));
    // current_match_id rimane invariato
    // Aggiorna il log: siamo il nuovo proprietario della partita riaperta
    log_match_created(match_id, player_id);
}

static void handle_lose_state(int match_id) {
    printf("\n%s Partita #%d - Risultato finale:\n", MSG_INFO, match_id);
    display_grid();
    printf("\n%s Hai perso.\n", MSG_ERROR);
    printf("=========================\n");
    my_turn_flag = 0;
    last_match_id = match_id; // Salva l'ID per la possibile rivincita
    // Il perdente riceverà STATE_TERMINATED a breve: reset_match_state lì
}

static void handle_draw_state(int match_id) {
    printf("\n%s Partita #%d - Risultato finale:\n", MSG_INFO, match_id);
    display_grid();
    printf("\n%s PAREGGIO!\n", MSG_INFO);
    printf("=========================\n");
    match_ended = 1;
    my_turn_flag = 0;
}

static void handle_terminated_state(int match_id) {
    // Se siamo già in waiting room come vincitori (handle_win_state ci ha già messi lì),
    // ignora il TERMINATED: è il segnale inviato al perdente/avversario che abbiamo
    // già gestito con STATE_WIN. Non dobbiamo resettare lo stato.
    if(in_waiting_room == 1) {
        return;
    }

    if(match_ended != 1) {
        printf("%s La partita è terminata\n", MSG_INFO);
    } else {
        printf("%s L'avversario ha abbandonato la partita.\n", MSG_INFO);
        printf("%s Ritorno al menu principale...\n\n", MSG_INFO);
    }

    // Se abbiamo perso (last_match_id != -1), preserva last_match_id per la rivincita
    int saved_last = last_match_id;
    reset_match_state();
    if(saved_last != -1) {
        last_match_id = saved_last;
        printf("%s In attesa che il vincitore riapra la partita... (opzione 6 disponibile a breve)\n", MSG_INFO);
    }
    (void)match_id;
}

static void handle_inprogress_state(int match_id) {
    printf("%s Partita #%d in corso...\n", MSG_INFO, match_id);
    current_match_id = match_id;
    match_ended = 0;
    memset(client_grid, 0, sizeof(client_grid));
    // Se siamo il creatore, aggiungiamo la partita alla nostra lista disponibili
    // (il broadcast del server ci esclude, quindi la aggiungiamo manualmente)
    if(am_i_player1 == 1) {
        add_available_match(match_id, player_id);
        log_match_created(match_id, player_id);
    }
}

static void handle_waiting_state(int match_id) {
    printf("\n%s L'avversario ha abbandonato la rivincita.\n", MSG_INFO);
    printf("%s Sei ancora nella partita #%d come Player 1 (X).\n", MSG_INFO, match_id);
    printf("%s In attesa di un nuovo avversario...\n\n", MSG_INFO);
    match_ended = 0;
    my_turn_flag = 0;
    am_i_player1 = 1;
    in_waiting_room = 1;
    memset(client_grid, 0, sizeof(client_grid));
    // current_match_id rimane invariato: siamo ancora in quella stanza
    // Aggiorna il log: la partita è rinata e noi ne siamo il nuovo proprietario
    log_match_created(match_id, player_id);
}

static void handle_notice_state(void *serialized) {
    if(serialized == NULL) return;

    Server_NoticeState *state = (Server_NoticeState *)serialized;
    current_state = state->state;
    current_match_id = state->match;

    switch(state->state) {
        case STATE_TURN_PLAYER1:
        case STATE_TURN_PLAYER2:
            handle_turn_state(state->state, state->match);
            break;

        case STATE_WIN:
            handle_win_state(state->match);
            break;

        case STATE_LOSE:
            handle_lose_state(state->match);
            break;

        case STATE_DRAW:
            handle_draw_state(state->match);
            break;

        case STATE_TERMINATED:
            handle_terminated_state(state->match);
            break;

        case STATE_INPROGRESS:
            handle_inprogress_state(state->match);
            break;

        case STATE_CREATED:
            // Il richiedente accettato era già in partita: siamo ancora in attesa
            printf("\n%s Il giocatore accettato era già occupato in un'altra partita.\n", MSG_INFO);
            printf("%s In attesa di un nuovo avversario per la partita #%d...\n\n", MSG_INFO, state->match);
            in_waiting_room = 1;
            my_turn_flag = 0;
            break;

        case STATE_WAITING:
            handle_waiting_state(state->match);
            break;

        default:
            printf("%s Stato sconosciuto: %d\n", MSG_WARNING, state->state);
            break;
    }
}

static void handle_notice_move(void *serialized) {
    if(serialized == NULL) return;

    Server_NoticeMove *move = (Server_NoticeMove *)serialized;
    printf("%s Mossa: (%d,%d)\n", MSG_INFO, move->moveX, move->moveY);

    // Aggiorna griglia locale
    // Il turno corrente indica chi HA APPENA GIOCATO (il precedente)
    // Se current_state è TURN_PLAYER1, vuol dire che player2 ha appena giocato
    if(current_state == STATE_TURN_PLAYER1) {
        client_grid[move->moveX][move->moveY] = 'O'; // Player 2 ha giocato
    } else if(current_state == STATE_TURN_PLAYER2) {
        client_grid[move->moveX][move->moveY] = 'X'; // Player 1 ha giocato
    }
}

// ========== HANDLER PRINCIPALE ==========

void handle_packet(int sockfd, Packet *packet) {
    cached_sockfd = sockfd;

    void *serialized = serialize_packet(packet);

    // Dispatch del pacchetto al rispettivo handler
    switch(packet->id) {
        case SERVER_HANDSHAKE:
            handle_handshake(serialized);
            break;

        case SERVER_SUCCESS:
            handle_success();
            break;

        case SERVER_ERROR:
            handle_error();
            break;

        case SERVER_INVALID_MOVE:
            handle_invalid_move();
            break;

        case SERVER_MATCHREQUEST:
            handle_match_request(serialized);
            break;

        case SERVER_UPDATEONREQUEST:
            handle_update_on_request(serialized);
            break;

        case SERVER_BROADCASTMATCH:
            handle_broadcast_match(serialized);
            break;

        case SERVER_NOTICESTATE:
            handle_notice_state(serialized);
            break;

        case SERVER_NOTICEMOVE:
            handle_notice_move(serialized);
            break;

        default:
            printf("%s Pacchetto sconosciuto: id=%d\n", MSG_WARNING, packet->id);
            break;
    }

    // Cleanup
    if(serialized != NULL) {
        free(serialized);
    }
}

// ========== FUNZIONI PUBBLICHE ==========

void print_grid() {
    display_grid();
}

void create_match(int sockfd) {
    Packet *packet = malloc(sizeof(Packet));
    packet->id = CLIENT_CREATEMATCH;
    packet->content = NULL;

    send_packet(sockfd, packet);
    free(packet);

    // Chi crea la partita è sempre Player1 (X)
    am_i_player1 = 1;

    printf("%s Richiesta creazione partita inviata\n", MSG_DEBUG);
}

void join_match(int sockfd, int match_id) {
    Client_JoinMatch *join = malloc(sizeof(Client_JoinMatch));
    join->match = match_id;

    Packet *packet = malloc(sizeof(Packet));
    packet->id = CLIENT_JOINMATCH;
    packet->content = join;

    send_packet(sockfd, packet);

    free(packet);
    free(join);

    // Chi fa join è sempre Player2 (O)
    am_i_player1 = 0;

    printf("%s Richiesta join partita #%d inviata\n", MSG_DEBUG, match_id);
}

void make_move(int sockfd, int match_id, int x, int y) {
    Client_MakeMove *move = malloc(sizeof(Client_MakeMove));
    move->moveX = x;
    move->moveY = y;
    move->match = match_id;

    Packet *packet = malloc(sizeof(Packet));
    packet->id = CLIENT_MAKEMOVE;
    packet->content = move;

    send_packet(sockfd, packet);

    free(packet);
    free(move);

    printf("%s Mossa inviata: (%d,%d) partita #%d\n", MSG_DEBUG, x, y, match_id);
}

void respond_to_request(int sockfd, int accepted) {
    if(pending_request_match == -1) {
        printf("%s Nessuna richiesta pendente!\n", MSG_ERROR);
        return;
    }

    Client_ModifyRequest *modify = malloc(sizeof(Client_ModifyRequest));
    modify->accepted = accepted;
    modify->match = pending_request_match;

    Packet *packet = malloc(sizeof(Packet));
    packet->id = CLIENT_MODIFYREQUEST;
    packet->content = modify;

    send_packet(sockfd, packet);

    free(packet);
    free(modify);

    printf("%s Risposta inviata: %s\n", MSG_INFO, accepted ? "ACCETTATO" : "RIFIUTATO");

    // Se accettato, il proprietario della partita è sempre Player1
    if(accepted) {
        am_i_player1 = 1;
    }

    // Reset pending request
    pending_request_player = -1;
    pending_request_match = -1;
}

void play_again(int sockfd, int match_id, int choice) {
    Client_PlayAgain *play = malloc(sizeof(Client_PlayAgain));
    play->choice = choice;
    play->match = match_id;

    Packet *packet = malloc(sizeof(Packet));
    packet->id = CLIENT_PLAYAGAIN;
    packet->content = play;

    send_packet(sockfd, packet);

    free(packet);
    free(play);

    if(choice == 1) {
        // Caso pareggio: aspetta che anche l'altro dica sì
        printf("%s Richiesta 'Gioca Ancora' inviata. In attesa dell'altro giocatore...\n", MSG_INFO);
    } else {
        printf("%s Hai rifiutato di giocare ancora. Partita terminata.\n", MSG_INFO);
        reset_match_state();
    }
}

void quit_match(int sockfd, int match_id) {
    Client_QuitMatch *quit = malloc(sizeof(Client_QuitMatch));
    quit->match = match_id;

    Packet *packet = malloc(sizeof(Packet));
    packet->id = CLIENT_QUITMATCH;
    packet->content = quit;

    send_packet(sockfd, packet);

    free(packet);
    free(quit);

    printf("%s Richiesta uscita dalla partita #%d inviata\n", MSG_INFO, match_id);
}
