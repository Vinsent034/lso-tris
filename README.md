# Progetto LSO: Tris

## Istruzioni per l'esecuzione

### Client
1. Avviare lo script di compilazione:
```bash
sh build.sh client
```

2. Eseguire il programma compilato:
```bash
./run_client
```

oppure specificando IP e porta del server:
```bash
./run_client 192.168.1.10 5555
```

### Server
1. Avviare lo script di compilazione:
```bash
sh build.sh server
```

2. Eseguire il programma compilato:
```bash
./run_server
```

oppure specificando una porta alternativa:
```bash
./run_server 8080
```

### Server (Docker)
1. Accedere alla directory del server:
```bash
cd server
```

2. Avviare il container con Docker:
```bash
docker compose up
```
