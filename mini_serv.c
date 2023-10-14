#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_CLIENTS 1000
#define BUFFER_SIZE 120000
int serverSocket, next_id = 0, clientSocket, socketId, bytesRead;
int clientSockets[MAX_CLIENTS] = {0}, fakeSockets[MAX_CLIENTS] = {0};
char buffer[BUFFER_SIZE] = {0}, buff[BUFFER_SIZE] = {0}, line[BUFFER_SIZE] = {0};
fd_set activeSockets;

int getFd(int fd)
{
	for (size_t i = 0; i < MAX_CLIENTS; i++) {
		if (clientSockets[i] == clientSocket)
			return fd = fakeSockets[i];
	}
	return 42;
}
void sendMsg(char *msg)
{
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (clientSockets[i] != clientSocket)
			msg ? send(clientSockets[i], buff, strlen(buff), 0) : send(clientSockets[i], buffer, strlen(buffer), 0);
	}
}
int main(int argc, char **argv)
{
	if (argc != 2)
		return write(2, "Wrong number of arguments\n", strlen("Wrong number of arguments\n")), 1;

	if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return write(2, "Fatal error\n", strlen("Fatal error\n")), 1;

	struct sockaddr_in serverAddress = {0};
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	serverAddress.sin_port = htons(atoi(argv[1]));

	if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
		return write(2, "Fatal error\n", strlen("Fatal error\n")), 1;

	if (listen(serverSocket, MAX_CLIENTS) < 0)
		return write(2, "Fatal error\n", strlen("Fatal error\n")), 1;

	while (1)
	{
		FD_ZERO(&activeSockets);
		FD_SET(serverSocket, &activeSockets);
		for (size_t i = 0; i < MAX_CLIENTS; i++) {
			if (clientSockets[i] > 0)
				FD_SET(clientSockets[i], &activeSockets);
		}

		if (select(MAX_CLIENTS + 1, &activeSockets, NULL, NULL, NULL) < 0)
			return write(2, "Fatal error\n", strlen("Fatal error\n")), 1;

		if (FD_ISSET(serverSocket, &activeSockets)) {
			if ((clientSocket = accept(serverSocket, NULL, NULL)) < 0)
				return write(2, "Fatal error\n", strlen("Fatal error\n")), 1;

			for (int i = 0; i < MAX_CLIENTS; i++) {
				if (clientSockets[i] == 0) {
					clientSockets[i] = clientSocket;
					fakeSockets[i] = next_id++;
					break;
				}
			}
			sprintf(buffer, "server: client %d just arrived\n", getFd(clientSocket));
			sendMsg(NULL);
		}
		for (int i = 0; i < MAX_CLIENTS; i++) {
			clientSocket = clientSockets[i];
			if (FD_ISSET(clientSocket, &activeSockets)) {
				if ((bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0)) <= 0) {
					sprintf(buffer, "server: client %d just left\n", getFd(clientSocket));
					sendMsg(NULL);
					close(clientSocket);
					clientSockets[i] = 0;
					fakeSockets[i] = 0;
				}
				else {
					for (int i = 0, j = 0; i < bytesRead; i++, j++) {
						line[j] = buffer[i];
						if (line[j] == '\n') {
							sprintf(buff, "client %d: %s", getFd(clientSocket), line);
							sendMsg(buff);
							bzero(line, strlen(line));
							j = -1;
						}
					}
				}
			}
		}
	}
	return 0;
}