CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
COMMON_SRC = common/models.c common/protocol.c
CLIENT_SRC = client/client.c client/connection.c
SERVER_SRC = server/server.c server/structures.c server/connection.c

all: client server

client: $(COMMON_SRC) $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o run_client $(COMMON_SRC) $(CLIENT_SRC)

server: $(COMMON_SRC) $(SERVER_SRC)
	$(CC) $(CFLAGS) -o run_server $(COMMON_SRC) $(SERVER_SRC)

clean:
	rm -f run_client run_server *.o

test_compile:
	@echo "Testing compilation..."
	@make clean
	@make all
	@echo "✓ Compilation successful!"

# Docker targets
docker-build:
	@echo "Building Docker images..."
	docker-compose build

docker-up:
	@echo "Starting Docker containers..."
	docker-compose up

docker-down:
	@echo "Stopping Docker containers..."
	docker-compose down

docker-clean:
	@echo "Removing Docker images and containers..."
	docker-compose down --rmi all --volumes

docker-rebuild:
	@echo "Rebuilding Docker images..."
	docker-compose down
	docker-compose build --no-cache
	docker-compose up

.PHONY: all client server clean test_compile docker-build docker-up docker-down docker-clean docker-rebuild