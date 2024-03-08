#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

typedef struct s_client {
    int id;
    char msg[1024];
} t_client;

t_client clients[1024];
fd_set read_set, write_set, current;
int maxfd, gid = 0;
char send_buffer[200000], recv_buffer[200040];

void err(char *msg){
    if (msg)
        write(2, msg, strlen(msg));
    else
        write(2, "Fatal error", 11);
    write(2, "\n", 1);
    exit(1);
}

void send_to_all(int except){ 
    for (int fd = 0; fd <= maxfd; fd++)
        if (FD_ISSET(fd, &write_set) && fd != except)
            if (send(fd, send_buffer, strlen(send_buffer), 0) == -1)
                err(NULL);
}

int main(int ac, char **av) {
    if (ac != 2)
        err("Wrong number of arguments");
    
    struct sockaddr_in servaddr;
    int servfd = socket(AF_INET, SOCK_STREAM, 0);
    if (servfd == -1) err(NULL);
    maxfd = servfd;
    FD_ZERO(&current);
    FD_SET(servfd, &current);

    bzero(clients, sizeof(clients));
    bzero(&servaddr, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(2130706433);
    servaddr.sin_port = htons(atoi(av[1]));

    if (bind(servfd, (const struct sockaddr*)&servaddr, sizeof(servaddr)) == -1 || listen(servfd, 100) == -1)
        err(NULL);

    while (42) {
        read_set = write_set = current;
        if (select(maxfd + 1, &read_set, &write_set, NULL, NULL) == -1) continue;

        for (int fd = 0; fd <= maxfd; fd++) {
            if (FD_ISSET(fd, &read_set)) {
                if (fd == servfd) {
                    int clientfd = accept(servfd, NULL, NULL);
                    if (clientfd == -1) continue;
                    if (clientfd > maxfd) maxfd = clientfd;
                    clients[clientfd].id = gid++;
                    FD_SET(clientfd, &current);
                    sprintf(send_buffer, "server: client %d just arrived\n", clients[clientfd].id);
                    send_to_all(clientfd);
                    break;
                } else {
                    int ret = recv(fd, recv_buffer, sizeof(recv_buffer), 0);
                    if (ret <= 0) {
                        sprintf(send_buffer, "server: client %d just left\n", clients[fd].id);
                        send_to_all(fd);
                        FD_CLR(fd, &current);
                        close(fd);
                    } else {
                        for (int i = 0, j = strlen(clients[fd].msg); i < ret; i++, j++) {
                            clients[fd].msg[j] = recv_buffer[i];
                            if (clients[fd].msg[j] == '\n') {
                                clients[fd].msg[j] = '\0';
                                sprintf(send_buffer, "client %d: %s\n", clients[fd].id, clients[fd].msg);
                                send_to_all(fd);
                                bzero(clients[fd].msg, strlen(clients[fd].msg));
                                j = -1;
                            }
                        }
                        bzero(clients[fd].msg, strlen(clients[fd].msg));
                        break;
                    }
                }
            }
        }
    }
}