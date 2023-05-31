#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_CLIENTS 128
#define BUFFER_SIZE 200000

int main(int argc, char **argv) 
{
    if (argc != 2) 
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    int clientSockets[MAX_CLIENTS];
    int next_id = 0;
    fd_set activeSockets, readySockets;
    char buffer[BUFFER_SIZE];
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket < 0) 
    {
        perror("Error creating server socket");
        exit(1);
    }

    struct sockaddr_in serverAddress = {0};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    serverAddress.sin_port = htons(atoi(argv[1]));

    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) 
    {
        perror("Error binding server socket");
        exit(1);
    }

    if (listen(serverSocket, MAX_CLIENTS) < 0) 
    {
        perror("Error listening on server socket");
        exit(1);
    }

    FD_ZERO(&activeSockets);
    FD_SET(serverSocket, &activeSockets);
    int maxSocket = serverSocket;

    while (1) 
    {
        readySockets = activeSockets;
        if (select(maxSocket + 1, &readySockets, NULL, NULL, NULL) < 0) 
        {
            perror("Error in select");
            exit(1);
        }

        for (int socketId = 0; socketId <= maxSocket; socketId++) 
        {
            if (FD_ISSET(socketId, &readySockets)) 
            {
                if (socketId == serverSocket) 
                {
                    int clientSocket = accept(serverSocket, NULL, NULL);
                    if (clientSocket < 0) 
                    {
                        perror("Error accepting client connection");
                        exit(1);
                    }

                    FD_SET(clientSocket, &activeSockets);
                    maxSocket = (clientSocket > maxSocket) ? clientSocket : maxSocket;
                    sprintf(buffer, "server: client %d just arrived\n", next_id);
                    send(clientSocket, buffer, strlen(buffer), 0);
                    clientSockets[next_id++] = clientSocket;

                } 
                else 
                {
                    int bytesRead = recv(socketId, buffer, sizeof(buffer) - 1, 0);

                    if (bytesRead <= 0) 
                    {
                        sprintf(buffer, "server: client %d just left\n", socketId);

                        for (int i = 0; i < next_id; i++)
                        {
                            if (clientSockets[i] != socketId) 
                            {
                                send(clientSockets[i], buffer, strlen(buffer), 0);
                            }
                        }
                        close(socketId);
                        FD_CLR(socketId, &activeSockets);
                    } 
                    else 
                    {
                        buffer[bytesRead] = '\0';
                        sprintf(buffer, "client %d: %s\n", socketId, buffer);
                        for (int i = 0; i < next_id; i++) 
                        {
                            if (clientSockets[i] != socketId) 
                            {
                                send(clientSockets[i], buffer, strlen(buffer), 0);
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}
