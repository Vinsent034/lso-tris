# Corso Di Laurea In Informatica
# Anno Accademico 2024/2025

# Progettazione e sviluppo di un sistema concorrente e distribuito (Client-Server) per il gioco del Tris

**Autore:** Di Carluccio Vincenzo
**Matricola:** N86004800
**Email:** vi.dicarluccio@studenti.unina.it

---

## Indice

1. Introduzione
   - 1.1 Struttura del progetto
2. Progettazione
   - 2.1 Funzionalità principali
   - 2.2 Responsabilità Server-side
   - 2.3 Responsabilità Client-side
3. Implementazione
   - 3.1 Protocollo di Comunicazione
   - 3.2 Strutture Dati
   - 3.3 Gestione della Concorrenza
   - 3.4 Gestione degli Errori
   - 3.5 Ciclo di vita di una connessione
4. Note d'uso e Build
   - 4.1 Compilazione Nativa
   - 4.2 Esecuzione del Server
   - 4.3 Esecuzione del Client
   - 4.4 Workflow di gioco

---

## Capitolo 1 - Introduzione

L'elaborato descrive le scelte di progettazione e implementazione di un sistema Client-Server per la gestione di partite a Tris in modalità concorrente, sfruttando un protocollo di comunicazione binario personalizzato su socket TCP.

Lo sviluppo in ogni sua fase è stato supportato dall'utilizzo di Git per il version control. Come scelta preliminare si è deciso di utilizzare il linguaggio C per l'implementazione di entrambi i lati Client e Server, facendo uso delle API POSIX per la gestione dei thread e delle socket.

### 1.1 Struttura del progetto

La struttura del progetto segue un'organizzazione modulare che separa la logica comune, il server e il client:

```
lso-tris/
├── common/
│   ├── models.h / models.c       # Strutture dati condivise e liste
│   └── protocol.h / protocol.c   # Definizione e serializzazione pacchetti
├── server/
│   ├── server.c                  # Main server, inizializzazione socket e accept loop
│   ├── structures.h / structures.c  # Liste client, mutex globali, wrapper thread-safe
│   └── connection.h / connection.c  # Gestione pacchetti, logica di gioco, thread
└── client/
    ├── client.c                  # Main client, menu interattivo, loop principale
    └── connection.h / connection.c  # Gestione pacchetti lato client, stato di gioco
```

---

## Capitolo 2 - Progettazione

La progettazione del sistema si basa sull'identificazione dei macro-elementi principali: i giocatori (`Player`) e le partite (`Match`). Analizzandone i possibili stati e fasi del ciclo di vita è stato possibile definire una chiara separazione delle responsabilità tra Client e Server.

### 2.1 Funzionalità principali

Il sistema implementa le seguenti funzionalità:

**Creazione Partita**
Un giocatore può creare una nuova partita e attendere che un avversario si unisca. Alla partita viene assegnato un ID univoco e viene inviato un broadcast a tutti i client liberi per notificare la disponibilità.

**Join e Richieste**
Un giocatore può richiedere di unirsi a una partita disponibile. Le richieste vengono gestite in una coda FIFO associata alla partita. Il proprietario riceve la notifica della prima richiesta in coda e può accettarla o rifiutarla. In caso di rifiuto, la prossima richiesta in coda viene proposta automaticamente.

**Logica di Gioco**
Una volta avviata, la partita procede con un sistema di turni alternati. Il server riceve le mosse dal client di turno, le valida (coordinate valide, casella libera, turno corretto), aggiorna la griglia, notifica entrambi i giocatori e verifica la condizione di vittoria o pareggio. Il client aggiorna localmente la propria copia della griglia basandosi sulle notifiche ricevute dal server.

**Broadcasting**
Quando una nuova partita viene creata, il server invia un pacchetto `SERVER_BROADCASTMATCH` a tutti i client con il flag `busy == 0` (non impegnati in una partita attiva). La stessa notifica viene inviata, con convenzione `player_id = -1`, al termine di una partita per segnalarne la chiusura.

**Abbandono Partita (Quit)**
Un giocatore può abbandonare una partita in corso tramite l'opzione di menu o digitando `N` durante l'inserimento delle coordinate. Il server notifica l'avversario con `STATE_WIN` e `STATE_TERMINATED`, resetta i flag `busy` e rimuove la partita dal registro globale.

**Rivincita (Play Again)**
Al termine di una partita, entrambi i giocatori possono richiedere una rivincita. Il server tiene traccia delle preferenze tramite un contatore (`play_again_counter`) e un array (`play_again[2]`). Solo se entrambi accettano e nessuno dei due è nel frattempo entrato in un'altra partita, la griglia viene azzerata e la partita riprende. Se uno dei due rifiuta o si disconnette, la partita viene rimossa o, nel caso del solo proprietario rimasto, convertita in stato `STATE_CREATED` per attendere un nuovo avversario.

### 2.2 Responsabilità Server-side

Il server è responsabile di:

- Accettare connessioni e creare un thread dedicato per ogni client
- Assegnare un ID univoco a ogni giocatore durante l'handshake
- Gestire un registro globale di giocatori (`ClientNode *clients`) e partite (`MatchList *matches`)
- Implementare il protocollo binario di comunicazione (serializzazione e parsing pacchetti)
- Gestire la coda FIFO delle richieste di join per ogni partita
- **Validare le mosse** ricevute dai client (coordinate, turno, casella occupata)
- **Calcolare il risultato** di ogni partita (controllo righe, colonne, diagonali, pareggio)
- Instradare i messaggi tra i client della stessa partita
- Gestire le disconnessioni inaspettate e notificare l'avversario

### 2.3 Responsabilità Client-side

Il client è responsabile di:

- Connettersi al server e completare l'handshake iniziale
- Fornire un'interfaccia utente testuale con menu numerato
- Raccogliere e validare **sintatticamente** l'input dell'utente (es. che la coordinata sia un numero tra 0 e 2) prima di inviarlo al server
- Mantenere una copia locale della griglia (`client_grid[3][3]`), aggiornata tramite i pacchetti `SERVER_NOTICEMOVE` ricevuti dal server
- Ricevere le notifiche di stato dal server e aggiornare la visualizzazione
- Gestire il proprio stato di gioco tramite variabili globali (`my_turn_flag`, `current_match_id`, `match_ended`, ecc.)

---

## Capitolo 3 - Implementazione

### 3.1 Protocollo di Comunicazione

Il sistema utilizza un protocollo binario personalizzato. Ogni pacchetto ha il seguente formato:

```
| Type (1 byte) | Size (2 byte, little-endian) | Content (variabile) |
```

Il campo `Type` identifica il tipo di messaggio. Il campo `Size` indica la lunghezza in byte del payload `Content`. La codifica little-endian del campo `Size` è:

```c
serialized[1] = packet->size & 0xFF;
serialized[2] = (packet->size >> 8) & 0xFF;
```

Questa scelta riduce l'overhead rispetto a protocolli testuali e permette una deserializzazione diretta tramite offset fissi sui byte ricevuti.

**Pacchetti Client → Server:**

| Costante | Valore | Payload (byte) | Descrizione |
|---|---|---|---|
| `CLIENT_HANDSHAKE` | 0 | 0 | Primo contatto, richiesta ID univoco |
| `CLIENT_CREATEMATCH` | 1 | 0 | Creazione di una nuova partita |
| `CLIENT_JOINMATCH` | 3 | 1 (`match`) | Richiesta di join a una partita esistente |
| `CLIENT_MODIFYREQUEST` | 4 | 2 (`accepted`, `match`) | Accettazione o rifiuto di una richiesta di join |
| `CLIENT_MAKEMOVE` | 5 | 3 (`moveX`, `moveY`, `match`) | Invio di una mossa |
| `CLIENT_PLAYAGAIN` | 6 | 2 (`choice`, `match`) | Richiesta di rivincita a fine partita |
| `CLIENT_QUITMATCH` | 7 | 1 (`match`) | Abbandono volontario della partita |

**Pacchetti Server → Client:**

| Costante | Valore | Payload (byte) | Descrizione |
|---|---|---|---|
| `SERVER_HANDSHAKE` | 20 | 1 (`player_id`) | Assegnazione ID univoco al client |
| `SERVER_SUCCESS` | 21 | 0 | Conferma operazione completata |
| `SERVER_ERROR` | 22 | 0 | Errore generico |
| `SERVER_MATCHREQUEST` | 23 | 2 (`other_player`, `match`) | Notifica richiesta di join al proprietario |
| `SERVER_NOTICESTATE` | 24 | 2 (`state`, `match`) | Aggiornamento stato partita |
| `SERVER_NOTICEMOVE` | 25 | 3 (`moveX`, `moveY`, `match`) | Sincronizzazione mossa tra i giocatori |
| `SERVER_BROADCASTMATCH` | 26 | 2 (`player_id`, `match`) | Annuncio nuova partita o partita terminata |
| `SERVER_UPDATEONREQUEST` | 30 | 2 (`accepted`, `match`) | Esito della richiesta di join al richiedente |
| `SERVER_INVALID_MOVE` | 31 | 0 | Casella già occupata |

**Convenzione `SERVER_BROADCASTMATCH`:** se `player_id = -1`, indica che la partita è terminata (e non più disponibile); altrimenti indica il creatore di una nuova partita disponibile.

### 3.2 Strutture Dati

Le strutture dati sono definite in `common/models.h` e `server/structures.h`.

**`Player`** (`common/models.h`)
Rappresenta un giocatore connesso. L'ID è un valore da 1 a 255, assegnato dal server durante l'handshake. Il flag `busy` vale 1 quando il giocatore è impegnato in una partita attiva.

```c
typedef struct {
    int id;    // ID univoco (1-255)
    int busy;  // 1 = in partita, 0 = libero
} Player;
```

**`RequestNode`** (`common/models.h`)
Nodo della coda FIFO delle richieste di join associate a una partita.

```c
typedef struct RequestNode {
    Player *requester;
    struct RequestNode *next;
} RequestNode;
```

**`Match`** (`common/models.h`)
Struttura centrale che rappresenta una sessione di gioco. Il campo `participants[0]` è sempre Player 1 (simbolo X), `participants[1]` è Player 2 (simbolo O). La griglia `grid[3][3]` contiene `0` per casella vuota, `'X'` o `'O'` per casella occupata. Il campo `free_slots` parte da 9 e viene decrementato ad ogni mossa valida. I campi `play_again[2]` e `play_again_counter` gestiscono la richiesta di rivincita.

```c
typedef struct {
    Player *participants[2];
    RequestNode *requests_head;
    RequestNode *requests_tail;
    char grid[3][3];
    int free_slots;
    int state;
    int play_again_counter;
    Player *play_again[2];
    int id;
} Match;
```

**`MatchList`** (`common/models.h`)
Lista linkata globale di tutte le partite attive, con inserimento in testa.

```c
typedef struct MatchList {
    Match *val;
    struct MatchList *next;
} MatchList;
```

**`Client`** (`server/structures.h`)
Rappresenta la connessione lato server, con il file descriptor della socket TCP, l'indirizzo IP e il puntatore al `Player` associato.

```c
typedef struct {
    int conn;
    struct sockaddr_in addr;
    Player *player;
} Client;
```

**`ClientNode`** (`server/structures.h`)
Lista linkata globale di tutti i client connessi al server.

```c
typedef struct ClientNode {
    Client *val;
    struct ClientNode *next;
} ClientNode;
```

**Stati della partita** (`common/models.h`):

| Costante | Valore | Significato |
|---|---|---|
| `STATE_TERMINATED` | 0 | Partita conclusa, in attesa di decisione rivincita |
| `STATE_INPROGRESS` | 1 | Partita avviata (fase transitoria) |
| `STATE_WAITING` | 2 | Proprietario in attesa di nuovo avversario (post-rivincita annullata) |
| `STATE_CREATED` | 3 | Partita creata, in attesa del secondo giocatore |
| `STATE_TURN_PLAYER1` | 4 | Turno di Player 1 (X) |
| `STATE_TURN_PLAYER2` | 5 | Turno di Player 2 (O) |
| `STATE_WIN` | 6 | Il giocatore ha vinto |
| `STATE_LOSE` | 7 | Il giocatore ha perso |
| `STATE_DRAW` | 8 | Pareggio |

### 3.3 Gestione della Concorrenza

**Architettura thread del server**

Per ogni nuova connessione accettata, il server crea due thread:

- **`server_thread`**: rimane in un loop `recv()` in attesa di pacchetti dal client e li processa chiamando `handle_packet()`.
- **`joiner_thread`**: creato in modalità detached, esegue `pthread_join(server_thread)` e attende la terminazione naturale del `server_thread`. Solo dopo provvede al cleanup: notifica l'eventuale avversario, rimuove le partite associate e libera la memoria. Questa separazione garantisce che il cleanup avvenga sempre dopo la completa terminazione del thread di comunicazione.

Il main thread del server rimane bloccato su `accept()` in un loop infinito, accettando nuove connessioni.

**Architettura thread del client**

Il client utilizza due thread:

- **Main thread**: gestisce il menu interattivo e l'input dell'utente tramite un loop principale. Nella fase di attesa del turno avversario, usa `select()` con timeout di 100ms su `stdin` per non bloccarsi su `fgets()` e poter rilevare tempestivamente il cambio di turno.
- **`receiver_thread`**: riceve in background i pacchetti dal server e aggiorna le variabili globali di stato (`my_turn_flag`, `current_match_id`, ecc.).

**Mutex e sincronizzazione** (`server/structures.c`)

Le strutture dati globali condivise tra i thread sono protette da mutex inizializzati staticamente:

```c
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t matches_mutex = PTHREAD_MUTEX_INITIALIZER;
```

| Mutex | Funzioni protette |
|---|---|
| `clients_mutex` | `broadcast_packet()`, `get_socket_by_player_id()`, `remove_client_from_list()` |
| `matches_mutex` | `safe_add_match()`, `safe_remove_match()`, `safe_get_match_by_id()` |

Per la generazione degli ID univoci dei giocatori è presente un terzo mutex locale al file `server/connection.c`:

```c
static pthread_mutex_t player_id_mutex = PTHREAD_MUTEX_INITIALIZER;
```

Durante `get_unique_player_id()` vengono acquisiti in sequenza sia `player_id_mutex` che `clients_mutex`, garantendo che due thread non possano ricevere lo stesso ID in caso di connessioni simultanee.

Il broadcast è selettivo: `broadcast_packet()` invia pacchetti solo ai client con `player->busy == 0`, escludendo i giocatori impegnati in partite attive.

### 3.4 Gestione degli Errori

Il sistema gestisce le seguenti categorie di errori:

- **Errori di connessione**: il client termina stampando un messaggio su `stderr` tramite `fprintf`. Il server stampa l'errore ma continua ad accettare nuove connessioni.
- **Disconnessioni inaspettate**: il `joiner_thread` rileva la terminazione del `server_thread`, identifica le partite a cui il player partecipava e notifica l'avversario con `STATE_WIN` + `STATE_TERMINATED` se la partita era in corso, oppure con `STATE_WAITING` se era in fase di rivincita. Nel secondo caso la partita viene riconvertita in `STATE_CREATED` per consentire un nuovo join.
- **Errori di input**: il client valida sintatticamente l'input (verifica che la coordinata sia un intero compreso tra 0 e 2) e richiede un nuovo inserimento senza inviare dati al server.
- **Errori di protocollo**: il server risponde con `SERVER_ERROR` o `SERVER_INVALID_MOVE` in caso di mossa su casella occupata, turno non corretto, match non trovato o permessi insufficienti.
- **Pacchetti incompleti**: sia client che server gestiscono la ricezione parziale verificando che il buffer contenga almeno 3 byte di header e che il payload completo sia disponibile prima di processare il pacchetto.

### 3.5 Ciclo di vita di una connessione

**Avvio:**
Il client si connette al server tramite socket TCP e invia immediatamente `CLIENT_HANDSHAKE`. Il server crea `server_thread` e `joiner_thread`, assegna un `player_id` univoco tramite `get_unique_player_id()` e risponde con `SERVER_HANDSHAKE`. Da questo momento il client è operativo.

**Fase di gioco:**
Il `server_thread` rimane nel loop `recv()` elaborando i pacchetti ricevuti. Le operazioni principali (`CLIENT_CREATEMATCH`, `CLIENT_JOINMATCH`, `CLIENT_MODIFYREQUEST`, `CLIENT_MAKEMOVE`, `CLIENT_PLAYAGAIN`, `CLIENT_QUITMATCH`) sono gestite all'interno di `handle_packet()` in `server/connection.c`. La logica di validazione delle mosse e il calcolo del risultato avvengono interamente sul server tramite `check_winner()` e `is_board_full()`.

**Terminazione:**
Quando `recv()` restituisce 0 o un valore negativo, il `server_thread` esce dal loop e termina. Il `joiner_thread`, che era in attesa su `pthread_join(server_thread)`, si sblocca ed esegue il cleanup: rimozione dalla lista client, liberazione della memoria, aggiornamento dello stato delle partite associate e notifica dell'avversario.

---

## Capitolo 4 - Note d'uso e Build

### 4.1 Compilazione Nativa

Per compilare il progetto su Linux/Unix è sufficiente il comando:

```bash
make all
```

Il `Makefile` esegue le seguenti compilazioni:

```bash
# Client
gcc -Wall -Wextra -pthread -g -o run_client \
    common/models.c common/protocol.c \
    client/client.c client/connection.c

# Server
gcc -Wall -Wextra -pthread -g -o run_server \
    common/models.c common/protocol.c \
    server/server.c server/structures.c server/connection.c
```

I file eseguibili generati sono `run_client` e `run_server`.

### 4.2 Esecuzione del Server

```bash
./run_server
```

Il server si avvia sulla porta `5555` (default) e rimane in ascolto per le connessioni dei client. È possibile specificare una porta alternativa come argomento:

```bash
./run_server 6000
```

### 4.3 Esecuzione del Client

```bash
./run_client
```

Il client si connette all'indirizzo `127.0.0.1:5555` di default. È possibile specificare IP e porta:

```bash
./run_client 192.168.1.10 5555
```

È possibile avviare più istanze del client in terminali separati per simulare partite tra giocatori.

### 4.4 Workflow di gioco

1. Giocatore 1 seleziona l'opzione `1` per creare una partita
2. Tutti i client liberi ricevono una notifica broadcast con l'ID della nuova partita
3. Giocatore 2 seleziona l'opzione `2` e inserisce l'ID della partita
4. Giocatore 1 riceve la notifica di richiesta di join e seleziona l'opzione `5` per rispondere
5. Giocatore 1 accetta (`1`) o rifiuta (`0`) la richiesta
6. Se accettata, la partita inizia: Giocatore 1 ha il simbolo X e gioca per primo
7. Il gioco procede con turni alternati: a ogni turno il server notifica entrambi i giocatori con `SERVER_NOTICESTATE` e la mossa effettuata con `SERVER_NOTICEMOVE`
8. La partita termina quando il server rileva una vittoria (tre simboli allineati in riga, colonna o diagonale) o un pareggio (griglia piena senza vincitore)
9. Al termine, ciascun giocatore può selezionare l'opzione `6` per richiedere una rivincita

---

## Conclusione

Il progetto dimostra come implementare un sistema distribuito concorrente in C sfruttando socket TCP, threading POSIX e un protocollo binario personalizzato. La separazione tra logica comune (`common/`), logica server (`server/`) e logica client (`client/`) facilita la manutenzione del codice.

La scelta di centralizzare nel server tutta la validazione delle mosse e il calcolo del risultato garantisce la correttezza del gioco indipendentemente dal comportamento del client. L'utilizzo di un thread per client, di mutex distinti per le strutture dati condivise e di wrapper thread-safe per le operazioni sulle liste assicura la consistenza del sistema anche in presenza di connessioni e disconnessioni simultanee.
