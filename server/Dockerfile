FROM gcc:latest
WORKDIR /tris
COPY ./server server
COPY ./common common
RUN gcc -Wall -Wextra -pthread -g \
    -o run_server \
    common/models.c common/protocol.c \
    server/server.c server/structures.c server/connection.c
CMD ["./run_server"]
