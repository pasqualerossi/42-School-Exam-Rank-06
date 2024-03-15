#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>

typedef struct client {
    int fd;
    int id;
} client;

void exit_error(char *str) {
    write(2, str, strlen(str));
    exit(1);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        exit_error("Wrong number of arguments\n");
    }

    const int MAX_CLIENTS = 128;
    const int BUFFER_SIZE = 200000;
    client clients[MAX_CLIENTS];
    int next_id = 0;
    fd_set active_sockets, ready_sockets;
    char buffer[BUFFER_SIZE];
    char msg_buffer[BUFFER_SIZE];
    int server_socket;

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        exit_error("Fatal error\n");
    }

    struct sockaddr_in server_address = {0};
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    server_address.sin_port = htons(atoi(argv[1]));

    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        exit_error("Fatal error\n");
    }

    if (listen(server_socket , MAX_CLIENTS) < 0) {
        exit_error("Fatal error\n");
    }

    bzero(clients, sizeof(client) * MAX_CLIENTS);
    FD_ZERO(&active_sockets);
    FD_SET(server_socket, &active_sockets);
    int max_socket = server_socket;

    while (42) {
        ready_sockets = active_sockets;
        if (select(max_socket + 1, &ready_sockets, NULL, NULL, NULL) < 0) {
            exit_error("Fatal error\n");
        }
        for (int socket_id = 0; socket_id <= max_socket; socket_id++) {
            if (!FD_ISSET(socket_id, &ready_sockets)) {
                continue ;
            }
            bzero(buffer, BUFFER_SIZE);
            if (socket_id == server_socket) {
                int client_socket;

                if ((client_socket = accept(server_socket, NULL, NULL)) < 0) {
                    exit_error("Fatal error\n");
                }
                FD_SET(client_socket, &active_sockets);
                max_socket = (client_socket > max_socket) ? client_socket : max_socket;
                clients[client_socket].fd = client_socket;
                clients[client_socket].id = next_id++;
                sprintf(buffer, "server: client %d just arrived\n", clients[client_socket].id);
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i].fd != 0 && clients[i].fd != client_socket) {
                        send(clients[i].fd, buffer, strlen(buffer), 0);
                    }
                }
            } else {
                int bytes_read = recv(socket_id, buffer, sizeof(buffer) - 1, 0);

                if (bytes_read <= 0) {
                    bzero(msg_buffer, BUFFER_SIZE);
                    sprintf(msg_buffer, "server: client %d just left\n", clients[socket_id].id);
                    for (int i = 0; i < MAX_CLIENTS; i++) {
                        if (clients[i].fd != socket_id && clients[i].fd != 0) {
                            send(clients[i].fd, msg_buffer, strlen(msg_buffer), 0);
                        }
                    }
                    close(socket_id);
                    FD_CLR(socket_id, &active_sockets);
                } else {
                    bzero(msg_buffer, BUFFER_SIZE);
                    sprintf(msg_buffer, "client %d: %s\n", clients[socket_id].id, buffer);
                    for (int i = 0; i < MAX_CLIENTS; i++) {
                        if (clients[i].fd != socket_id) {
                            send(clients[i].fd, msg_buffer, strlen(msg_buffer), 0);
                        }
                    }
                }
            }
        }
    }
    return 0;
}
