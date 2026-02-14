#!/bin/bash
# Script di entrypoint per il client Docker
# Utilizza le variabili d'ambiente SERVER_HOST e SERVER_PORT se presenti

SERVER_HOST=${SERVER_HOST:-"127.0.0.1"}
SERVER_PORT=${SERVER_PORT:-"5555"}

exec ./run_client "$SERVER_HOST" "$SERVER_PORT"
