#!/bin/bash
# Script per avviare facilmente un client Docker aggiuntivo

PLAYER_NUM=${1:-3}
CONTAINER_NAME="lso-tris-client${PLAYER_NUM}"

echo "🎮 Avvio client ${PLAYER_NUM}..."
echo "Container: ${CONTAINER_NAME}"
echo ""

# Verifica che il server sia in esecuzione
if ! docker ps | grep -q "lso-tris-server"; then
    echo "❌ Il server non è in esecuzione!"
    echo "Avvialo prima con: ./docker-start.sh o docker-compose up server -d"
    exit 1
fi

echo "✓ Server trovato"
echo ""

# Avvia un nuovo client
docker-compose run --rm --name "${CONTAINER_NAME}" client1

echo ""
echo "Client ${PLAYER_NUM} terminato."
