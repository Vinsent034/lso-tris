CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
COMMON_SRC = common/models.c common/protocol.c
CLIENT_SRC = client/client.c client/gui.c client/connection.c
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

.PHONY: all clean test_compile