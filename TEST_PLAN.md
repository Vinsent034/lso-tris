# PIANO DI TEST - LSO Tris

## Setup

```bash
# Terminale SERVER (T0)
make clean && make all
./server 5555

# Terminale CLIENT 1 (T1)
./client 127.0.0.1 5555

# Terminale CLIENT 2 (T2)
./client 127.0.0.1 5555

# Terminale CLIENT 3 (T3) - per test con 3+ giocatori
./client 127.0.0.1 5555
```

---

## SEZIONE A - CONNESSIONE E HANDSHAKE

### A1. Connessione singola fatta
```
T0: ./server 5555
T1: ./client 127.0.0.1 5555
```
- [ ] Verificare che il client riceve un Player ID
- [ ] Verificare che il server stampa il messaggio di connessione

### A2. Connessioni multiple simultanee fatto
```
T1: ./client 127.0.0.1 5555
T2: ./client 127.0.0.1 5555
T3: ./client 127.0.0.1 5555
```
- [ ] Verificare che ogni client riceve un ID diverso
- [ ] Verificare che il server gestisce tutti e 3

### A3. Connessione a server non attivo fatto 
```
T1: ./client 127.0.0.1 5555   (senza server avviato)
```
- [ ] Verificare che il client mostra errore e non crasha

### A4. Connessione con porta sbagliata  (capire meglio come funziona)
```
T0: ./server 5555
T1: ./client 127.0.0.1 9999
```
- [ ] Verificare errore di connessione

---

## SEZIONE B - CREAZIONE PARTITA

### B1. Creare una partita (fatto)
```
T1: Opzione 1 (Crea partita)
```
- [ ] Verificare risposta SUCCESS dal server
- [ ] Verificare che T2 e T3 ricevono il broadcast della nuova partita

### B2. Creare partita quando sei già in una partita (in attesa) (fallisce da aggiustare )
```
T1: Opzione 1 (Crea partita)
T1: Opzione 1 (Crea un'altra partita)
```
- [ ] Verificare comportamento (dovrebbe dare errore o creare seconda partita?)

### B3. Più giocatori creano partite diverse (funziona)
```
T1: Opzione 1 (Crea partita)
T2: Opzione 1 (Crea partita)
T3: Opzione 1 (Crea partita)
```
- [ ] Verificare che ogni partita ha ID diverso
- [ ] Verificare che tutti ricevono i broadcast delle partite degli altri

---

## SEZIONE C - JOIN E RICHIESTE

### C1. Join semplice (funziona)
```
T1: Opzione 1 (Crea partita) → nota match_id
T2: Opzione 2 (Join partita) → inserisci match_id
```
- [ ] T1 riceve notifica "Player X vuole unirsi"
- [ ] T2 riceve SUCCESS

### C2. Accettare una richiesta di join (funziona)
```
(dopo C1)
T1: Opzione 5 → inserisci match_id → 1 (accetta)
```
- [ ] Entrambi ricevono NOTICESTATE (turno player 1)
- [ ] La partita inizia, T1 è 'X', T2 è 'O'

### C3. Rifiutare una richiesta di join  (funziona)
```
T1: Opzione 1 (Crea)
T2: Opzione 2 (Join)
T1: Opzione 5 → match_id → 0 (rifiuta)
```
- [ ] T2 riceve notifica di rifiuto
- [ ] T1 resta in attesa di altri giocatori

### C4. Più giocatori richiedono la stessa partita (CODA) (funziona)
```
T1: Opzione 1 (Crea partita) → nota match_id
T2: Opzione 2 (Join con match_id)
T3: Opzione 2 (Join con match_id)
T1: Opzione 5 → match_id → 0 (rifiuta T2)
```
- [ ] T1 riceve prima la richiesta di T2
- [ ] Dopo il rifiuto di T2, T1 riceve la richiesta di T3
- [ ] T1 può accettare T3

### C5. Join alla propria partita (funziona)
```
T1: Opzione 1 (Crea) → nota match_id
T1: Opzione 2 (Join con proprio match_id)
```
- [ ] Verificare che viene rifiutato (non puoi giocare contro te stesso)

### C6. Join con match_id inesistente (fatto)
```
T1: Opzione 2 → inserisci match_id 200 (non esiste)
```
- [ ] Verificare errore dal server

### C7. Join a partita già in corso (fatto)
```
T1: Crea partita, T2 fa Join, T1 accetta → partita inizia
T3: Opzione 2 (Join alla stessa partita)
```
- [ ] Verificare che T3 viene rifiutato (partita piena/in corso)

---

## SEZIONE D - GAMEPLAY (MOSSE)

### D1. Partita completa - Vittoria
```
T1 crea, T2 join, T1 accetta.
Giocano una partita dove X vince (riga orizzontale):
  T1 (X): Opzione 3 → 0,0
  T2 (O): Opzione 3 → 1,0
  T1 (X): Opzione 3 → 0,1
  T2 (O): Opzione 3 → 1,1
  T1 (X): Opzione 3 → 0,2    ← X vince (riga 0)
```
- [ ] T1 vede "HAI VINTO!"
- [ ] T2 vede "Hai perso"
- [ ] Entrambi tornano al menu con opzione 6 disponibile

### D2. Partita completa - Pareggio
```
T1 crea, T2 join, T1 accetta.
Giocano fino a riempire la griglia senza vincere:
  T1 (X): 0,0    T2 (O): 0,1
  T1 (X): 0,2    T2 (O): 1,0
  T1 (X): 1,1    T2 (O): 2,0
  T1 (X): 2,1    T2 (O): 1,2
  T1 (X): 2,0    ← Se è pareggio
```
- [ ] Entrambi vedono "PAREGGIO!"

### D3. Mossa su cella occupata (fatto)
```
(durante una partita in corso)
T1 (X): Opzione 3 → 0,0     (piazza X)
T2 (O): aspetta turno
T1 (X): Opzione 3 → 0,0     (stessa cella!)
```
- [ ] Server risponde con INVALID_MOVE
- [ ] T1 può ritentare con coordinate diverse

### D4. Mossa fuori turno (fatto)
```
(durante partita, è il turno di T1)
T2: Opzione 3 → prova a fare una mossa
```
- [ ] Verificare che il server rifiuta la mossa
- [ ] Il turno resta a T1

### D5. Coordinate fuori range (fatta)
```
(durante il turno di T1)
T1: Opzione 3 → 5,5
```
- [ ] Verificare che il client o server rifiuta coordinate > 2

### D6. Visualizza griglia durante partita (fatta)
```
(dopo alcune mosse)
T1: Opzione 4
T2: Opzione 4
```
- [ ] Entrambi vedono la griglia aggiornata con X e O corretti
- [ ] Le posizioni corrispondono

### D7. Vittoria per colonna (fatta)
```
T1 (X): 0,0    T2 (O): 0,1
T1 (X): 1,0    T2 (O): 1,1
T1 (X): 2,0    ← X vince colonna 0
```
- [ ] Vittoria riconosciuta correttamente

### D8. Vittoria per diagonale (fatta)
```
T1 (X): 0,0    T2 (O): 0,1
T1 (X): 1,1    T2 (O): 0,2
T1 (X): 2,2    ← X vince diagonale
```
- [ ] Vittoria riconosciuta correttamente

### D9. Vittoria per anti-diagonale (fatta)
```
T1 (X): 0,2    T2 (O): 0,0
T1 (X): 1,1    T2 (O): 1,0
T1 (X): 2,0    ← X vince anti-diagonale
```
- [ ] Vittoria riconosciuta correttamente

### D10. Player 2 (O) vince (fatta)
```
T1 (X): 0,0    T2 (O): 1,0
T1 (X): 0,1    T2 (O): 1,1
T1 (X): 2,2    T2 (O): 1,2    ← O vince riga 1
```
- [ ] T2 vede "HAI VINTO!"
- [ ] T1 vede "Hai perso"

---

## SEZIONE E - ABBANDONO PARTITA (QUIT) (fatta)

### E1. Abbandono durante il proprio turno
```
(è il turno di T1)
T1: inserisci 'N' come coordinata
```
- [ ] T2 riceve notifica di vittoria
- [ ] T1 torna al menu

### E2. Abbandono durante il turno avversario (fatta)
```
(è il turno di T2, T1 sta aspettando)
T1: Opzione 7 (quit match) se disponibile, oppure CTRL+C
```
- [ ] T2 vince automaticamente
- [ ] Verificare cleanup corretto

---

## SEZIONE F - DISCONNESSIONE (fatta)

### F1. Client si disconnette durante partita (CTRL+C) (fatta)
```
T1 crea, T2 join, T1 accetta, inizia partita
T1: CTRL+C (chiudi terminale)
```
- [ ] T2 riceve notifica di vittoria automatica
- [ ] Il server non crasha
- [ ] Il server libera le risorse

### F2. Client si disconnette in attesa di join (fatta)
```
T1: Crea partita
T2: Join alla partita
T1: CTRL+C prima di accettare/rifiutare
```
- [ ] Il server non crasha
- [ ] T2 riceve notifica appropriata
- [ ] La partita viene rimossa

### F3. Client si disconnette subito dopo connessione (fatta)
```
T1: ./client 127.0.0.1 5555
T1: CTRL+C immediatamente
```
- [ ] Il server gestisce la disconnessione senza crash
- [ ] Nessun leak di risorse visibile nei log

### F4. Tutti i client si disconnettono (fatta)
```
T1, T2, T3: connessi
T1: CTRL+C
T2: CTRL+C
T3: CTRL+C
```
- [ ] Il server resta attivo e funzionante
- [ ] Nuovi client possono connettersi dopo

### F5. Disconnessione e riconnessione rapida (non funziona)
```
T1: CTRL+C
T1: ./client 127.0.0.1 5555  (riconnetti subito)
```
- [ ] Il client riceve un nuovo Player ID
- [ ] Può creare/unirsi a partite normalmente

---

## SEZIONE G - REMATCH (GIOCA ANCORA) 

### G1. Entrambi vogliono rigiocare (fatta)
```
(dopo fine partita)
T1: Opzione 6 → 1 (sì)
T2: Opzione 6 → 1 (sì)
```
- [ ] La partita riparte
- [ ] La griglia è resettata
- [ ] Il turno ricomincia da Player 1

### G2. Un giocatore rifiuta il rematch (fatta)
```
(dopo fine partita)
T1: Opzione 6 → 1 (sì)
T2: Opzione 6 → 0 (no)
```
- [ ] T1 riceve notifica che la partita è terminata
- [ ] Entrambi tornano liberi

### G3. Rematch dopo pareggio
```
(dopo pareggio)
T1: Opzione 6 → 1
T2: Opzione 6 → 1
```
- [ ] La partita riparte normalmente

### G4. Disconnessione durante attesa rematch
```
(dopo fine partita)
T1: Opzione 6 → 1 (vuole rigiocare)
T2: CTRL+C (si disconnette)
```
- [ ] T1 riceve notifica appropriata
- [ ] T1 torna libero

### G5. Rematch multiplo (giocare 3+ volte di fila)
```
(giocare una partita completa, rematch, giocare un'altra, rematch, giocare un'altra)
```
- [ ] Ogni rematch funziona correttamente
- [ ] La griglia si resetta ogni volta
- [ ] I turni funzionano correttamente

---

## SEZIONE H - SCENARI CON 3+ CLIENT

### H1. Due partite contemporanee (fatta)
```
T1: Crea partita
T2: Join alla partita di T1, T1 accetta → giocano
T3: ./client → Opzione 1 (Crea un'altra partita)
T4: ./client → Join alla partita di T3, T3 accetta → giocano
```
- [ ] Entrambe le partite funzionano indipendentemente
- [ ] Le mosse di una partita non influenzano l'altra

### H2. Spettatore vede i broadcast (non richiesta)
```
T1: Crea partita
T2: Crea partita
T3: (appena connesso, osserva)
```
- [ ] T3 riceve i broadcast di ENTRAMBE le partite create

### H3. Terzo giocatore tenta join a partita in corso (fatta)
```
T1 e T2 stanno giocando
T3: Opzione 2 → match_id della partita in corso
```
- [ ] T3 riceve errore (partita piena)

### H4. Coda di richieste con 3 giocatori (fatta)
```
T1: Crea partita (match_id = X)
T2: Join match X
T3: Join match X
T1: Rifiuta T2 (Opzione 5 → 0)
```
- [ ] Dopo rifiuto di T2, T1 riceve richiesta di T3
- [ ] T1 può accettare T3

### H5. Un giocatore finisce e ne inizia un'altra (fatto)
```
T1 e T2 giocano e finiscono (T1 vince)
T1: Opzione 6 → 0 (non rigioca)
T1: Opzione 1 (Crea nuova partita)
T3: Join alla nuova partita di T1
```
- [ ] T1 può creare una nuova partita dopo averne finita una
- [ ] T3 può unirsi normalmente

### H6. Stress test - 4 client contemporanei, 2 partite
```
T1+T2: Partita 1
T3+T4: Partita 2
Entrambe le partite giocate contemporaneamente
```
- [ ] Nessun crash
- [ ] Mosse corrette in entrambe le partite
- [ ] Vittorie/sconfitte riconosciute correttamente

---

## SEZIONE I - INPUT INVALIDI / EDGE CASES

### I1. Input non numerico nel menu (fatto)
```
T1: Scegli opzione → "abc"
T1: Scegli opzione → ""  (invio vuoto)
T1: Scegli opzione → "!@#$"
```
- [ ] Il client non crasha
- [ ] Ritorna al menu

### I2. Opzione menu non valida (fatto)
```
T1: Scegli opzione → 99
T1: Scegli opzione → -1
T1: Scegli opzione → 0
```
- [ ] Il client gestisce l'input senza crash

### I3. Coordinate non numeriche durante mossa (fatto)
```
(durante il turno)
T1: Coordinata → "abc"
T1: Coordinata → ""
```
- [ ] Il client non crasha
- [ ] Chiede nuovamente la coordinata o torna al menu

### I4. Match ID non valido nel join (fatto)
```
T1: Opzione 2 → inserisci "abc" come match_id
T1: Opzione 2 → inserisci -1
T1: Opzione 2 → inserisci 300
```
- [ ] Gestione corretta senza crash

### I5. Opzione 3 (mossa) senza essere in una partita
```
T1: (appena connesso, non in partita)
T1: Opzione 3
```
- [ ] Messaggio di errore o ignorato, senza crash

### I6. Opzione 5 (rispondi richiesta) senza richieste pendenti (fatto)
```
T1: Opzione 5 (nessuna richiesta pendente)
```
- [ ] Messaggio appropriato, senza crash

### I7. Opzione 6 (gioca ancora) senza partita terminata
```
T1: Opzione 6 (nessuna partita finita)
```
- [ ] Messaggio appropriato, senza crash

---

## SEZIONE J - ROBUSTEZZA DEL SERVER

### J1. Server rimane stabile dopo molte connessioni/disconnessioni
```
Connetti e disconnetti 5+ client di fila
Poi connetti un nuovo client
```
- [ ] Il server risponde normalmente
- [ ] Il nuovo client funziona correttamente

### J2. Server dopo crash di un client durante partita
```
T1 e T2 giocano
Forza chiusura T1: kill -9 $(pgrep -f "./client")   (da un altro terminale)
```
- [ ] Il server non crasha
- [ ] T2 viene notificato
- [ ] T2 può tornare a giocare

### J3. Chiusura pulita del server
```
T1 e T2 connessi
T0: CTRL+C sul server
```
- [ ] I client vengono notificati o gestiscono la disconnessione
- [ ] Nessun segfault

### J4. Riavvio del server
```
T0: CTRL+C → ./server 5555
T1: ./client 127.0.0.1 5555
```
- [ ] Il client si connette al server riavviato
- [ ] Tutto funziona da zero

---

## SEZIONE K - TEST SPECIFICI PER L'ESAME

### K1. Flusso completo dimostrativo (ESEGUIRE PER PRIMO ALL'ESAME)
```
1. T0: Avvia server
2. T1: Connetti client 1
3. T2: Connetti client 2
4. T1: Crea partita (opzione 1)
5. T2: Vede broadcast, fa join (opzione 2)
6. T1: Accetta richiesta (opzione 5 → 1)
7. Giocano una partita completa fino a vittoria
8. T1: Opzione 4 per vedere griglia finale
9. Entrambi: Opzione 6 → 1 per rematch
10. Giocano un'altra partita
11. Fine: Opzione 6 → 0 (no rematch)
```

### K2. Test con 3 giocatori
```
1. T1: Crea partita
2. T2: Join
3. T3: Join (stessa partita) → messo in coda
4. T1: Accetta T2 → partita inizia
5. T3 riceve rifiuto automatico (o attende)
6. T1 e T2 giocano, T1 vince
7. T1 e T2 rifiutano rematch
8. T3: Crea nuova partita
9. T1: Join alla partita di T3
10. T3: Accetta
11. T3 e T1 giocano
```

### K3. Test resilienza
```
1. T1 e T2 giocano
2. T2: CTRL+C a metà partita
3. T1: Verifica notifica vittoria
4. T1: Crea nuova partita
5. T3: Connetti e join
6. T1 accetta, giocano normalmente
```

---

## CHECKLIST RAPIDA PRE-ESAME

- [ ] `make clean && make all` compila senza errori
- [ ] Server avvia senza problemi
- [ ] 2 client si connettono
- [ ] Creare partita → broadcast funziona
- [ ] Join → richiesta arriva
- [ ] Accetta → partita inizia
- [ ] Mosse funzionano (turni alternati)
- [ ] Vittoria riconosciuta (riga/colonna/diagonale)
- [ ] Pareggio riconosciuto
- [ ] Rematch funziona
- [ ] Disconnessione gestita (CTRL+C)
- [ ] Quit partita funziona ('N')
- [ ] 3+ client contemporanei OK
- [ ] 2 partite contemporanee OK
- [ ] Server stabile dopo disconnessioni multiple
