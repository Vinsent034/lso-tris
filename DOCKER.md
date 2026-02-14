# Docker Setup - LSO-TRIS

Guida per eseguire il progetto LSO-TRIS con Docker.

## Prerequisiti

- Docker Engine (versione 20.10+)
- Docker Compose (versione 1.29+)

## Avvio Rapido

### Metodo 1: Script automatico (Consigliato)
```bash
./docker-start.sh
```

### Metodo 2: Comandi manuali

#### 1. Build delle immagini
```bash
docker-compose build
```

#### 2. Avvia server e client
```bash
docker-compose up
```

Questo comando avvia:
- 1 server sulla porta 5555
- 2 client (client1 e client2) già connessi

### 3. Interagire con i client

Apri due terminali separati per interagire con i client:

**Terminal 1 - Player 1:**
```bash
docker attach lso-tris-client1
```

**Terminal 2 - Player 2:**
```bash
docker attach lso-tris-client2
```

> **Nota:** Per staccarti dal container senza fermarlo, premi `Ctrl+P` seguito da `Ctrl+Q`

## Comandi Utili

### Avvia solo il server
```bash
docker-compose up server
```

### Avvia un singolo client
```bash
docker-compose up client1
```

### Ferma tutti i container
```bash
docker-compose down
```

### Visualizza i log
```bash
# Tutti i servizi
docker-compose logs -f

# Solo server
docker-compose logs -f server

# Solo client1
docker-compose logs -f client1
```

### Ricompila e riavvia
```bash
docker-compose down
docker-compose build --no-cache
docker-compose up
```

### Esegui un client aggiuntivo

#### Metodo rapido con script
```bash
./docker-client.sh 3  # Avvia client #3
./docker-client.sh 4  # Avvia client #4
```

#### Metodo manuale
```bash
docker-compose run --rm client1
```

## Build Singole (Opzionale)

### Build manuale del server
```bash
docker build -f Dockerfile.server -t lso-tris-server .
docker run -it --rm -p 5555:5555 lso-tris-server
```

### Build manuale del client
```bash
docker build -f Dockerfile.client -t lso-tris-client .
docker run -it --rm -e SERVER_HOST=localhost -e SERVER_PORT=5555 --network host lso-tris-client
```

## Architettura Docker

- **Multi-stage build**: Utilizziamo `gcc:13` per compilare e `debian:bookworm-slim` per runtime (immagini più leggere)
- **Network bridge**: I container comunicano tramite una rete Docker dedicata `tris-network`
- **Port mapping**: La porta 5555 del server è esposta sull'host
- **Entrypoint script**: I client usano variabili d'ambiente per connettersi al server

## Risoluzione Problemi

### Il client non si connette al server
Verifica che il server sia in ascolto:
```bash
docker-compose logs server | grep "Listening"
```

### Ricompilare dopo modifiche al codice
```bash
docker-compose down
docker-compose build
docker-compose up
```

### Pulire tutto (immagini, container, volumi)
```bash
docker-compose down --rmi all --volumes
```

## Workflow di Gioco con Docker

1. Avvia l'ambiente: `docker-compose up -d`
2. Attach al client1: `docker attach lso-tris-client1`
3. **Player 1** crea una partita (opzione `1`)
4. In un altro terminal, attach al client2: `docker attach lso-tris-client2`
5. **Player 2** si unisce alla partita (opzione `2`)
6. **Player 1** accetta la richiesta (opzione `5` → `1`)
7. Giocate a turno con l'opzione `3`

## Script di Utilità

Il progetto include 3 script per facilitare l'uso di Docker:

1. **`docker-start.sh`**: Avvio rapido (build + run)
2. **`docker-client.sh`**: Avvia client aggiuntivi facilmente
3. **`docker-entrypoint.sh`**: Entrypoint interno per i container client

## Note

- I client sono configurati per connettersi automaticamente a `server:5555` tramite variabili d'ambiente
- Lo script `docker-entrypoint.sh` utilizza le variabili `SERVER_HOST` e `SERVER_PORT`
- Se vuoi modificare la porta del server, aggiorna `docker-compose.yml` e ricompila
- Per testare con più di 2 client, usa `./docker-client.sh 3`
