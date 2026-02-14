#!/bin/bash
# Script rapido per avviare l'ambiente Docker LSO-TRIS

echo "==================================="
echo "  LSO-TRIS Docker Quick Start"
echo "==================================="
echo ""

# Verifica che Docker sia installato
if ! command -v docker &> /dev/null; then
    echo "❌ Docker non è installato. Installalo da https://docs.docker.com/get-docker/"
    exit 1
fi

if ! command -v docker-compose &> /dev/null; then
    echo "❌ Docker Compose non è installato. Installalo da https://docs.docker.com/compose/install/"
    exit 1
fi

echo "✓ Docker e Docker Compose trovati"
echo ""

# Build delle immagini
echo "📦 Building Docker images..."
docker-compose build

if [ $? -ne 0 ]; then
    echo "❌ Build fallito!"
    exit 1
fi

echo ""
echo "✓ Build completato con successo!"
echo ""

# Avvio dei container
echo "🚀 Avvio dei container..."
echo ""
echo "Server sarà disponibile su localhost:5555"
echo ""
echo "Per connetterti ai client, apri nuovi terminali ed esegui:"
echo "  docker attach lso-tris-client1"
echo "  docker attach lso-tris-client2"
echo ""
echo "Per staccarti senza fermare il container: Ctrl+P poi Ctrl+Q"
echo "Per fermare tutto: docker-compose down"
echo ""
echo "==================================="
echo ""

docker-compose up
