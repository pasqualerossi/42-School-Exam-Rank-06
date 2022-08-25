#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_SIZE 4096

typedef struct s_client 
{
	int id;
	int fd;
	struct s_client * next;
} t_client;

int g_id = 0;
int g_sockfd;

t_client * g_clients = NULL;
fd_set curr_sock, write_sock, read_sock;

int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				return (-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

char *str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	strcat(newbuf, add);
	return (newbuf);
}

void rm_client(int fd)
{
	t_client * tmp = g_clients, * del;

	if (g_clients != NULL && g_clients->fd == fd) 
	{
		del = g_clients;
		g_clients = g_clients->next;
	}
	else 
	{
		while (tmp && tmp->next && tmp->next->fd != fd)
			tmp = tmp->next;
		del = tmp->next;
		tmp->next = tmp->next->next;
	}
	free(del);
}

int get_max_fd() 
{
	int max_sd = g_sockfd;
	t_client * tmp = g_clients;

	while (tmp) 
	{
		if (tmp->fd > max_sd)
			max_sd = tmp->fd;
		tmp = tmp->next;
	}
	return max_sd;
}

int get_fd_id(int fd) 
{
	t_client * tmp = g_clients;

	while (tmp) 
	{
		if (tmp->fd == fd)
			return tmp->id;
		tmp = tmp->next;
	}

	return -1;
}

void fatal() 
{
	write(2, "Fatal error\n", 13);
	exit(1);
}

void broadcastLeftMessage(int fd, int index) 
{
	t_client * tmp = g_clients;
	char str[50];

	bzero(&str, 50);
	sprintf(str, "server: client %d just left\n", index);
	while (tmp) 
	{
		if (tmp->fd != fd && FD_ISSET(tmp->fd, &write_sock)) 
		{
			if (send(tmp->fd, str, strlen(str), 0) < 0)
				fatal();
		}
		tmp = tmp->next;
	}
}

void broadcastJoinMessage(int fd, int index) 
{
	t_client * tmp = g_clients;
	char str[50];

	bzero(&str, 50);
	sprintf(str, "server: client %d just arrived\n", index);
	while (tmp) 
	{
		if (tmp->fd != fd && FD_ISSET(tmp->fd, &write_sock)) 
		{
			if (send(tmp->fd, str, strlen(str), 0) < 0)
				fatal();
		}
		tmp = tmp->next;
	}
}

void broadcastMessage(char *message, int fd, int index) 
{
	t_client * tmp = g_clients;
	char * newMsg = NULL;
	int len = 0;

	if (message)
		len = strlen(message);

	newMsg = calloc(len + 42, sizeof(char));
	if (newMsg == NULL)
		fatal();

	sprintf(newMsg, "client %d: %s", index, message);
	while (tmp) {
		if (tmp->fd != fd && FD_ISSET(tmp->fd, &write_sock)) 
		{
			if (send(tmp->fd, newMsg, strlen(newMsg), 0) < 0)
				fatal();
		}
		tmp = tmp->next;
	}
	free(newMsg);
}

int sendMessage(char * message, int fd) 
{
	char * newMsg;
	int id = get_fd_id(fd);
	int check = extract_message(&message, &newMsg);
	
	if (check == -1)
		fatal();
	if (check == 0)
		return (0);
	while (check) 
	{
		broadcastMessage(newMsg, fd, id);
		free(newMsg);
		check = extract_message(&message, &newMsg);
		if (check == -1)
			fatal();
	}
	free(newMsg);
	free(message);
	return (1);
}

void add_client() 
{
	int newConnection;
	socklen_t len;
	struct sockaddr_in cli;
	
	t_client * tmp = g_clients;
	len = sizeof(cli);
	newConnection = accept(g_sockfd, (struct sockaddr *)&cli, &len);
	
	if (newConnection < 0)
		fatal();
	0
	t_client * newClt = calloc(1, sizeof(t_client));
	if (newClt == NULL)
		fatal();
	newClt->id = g_id++;
	newClt->fd = newConnection;
	newClt->next = NULL;
	
	if (g_clients == NULL) 
	{
		g_clients = newClt;
	}
	else 
	{
		while (tmp->next)
			tmp = tmp->next;
		tmp->next = newClt;
	}
	broadcastJoinMessage(newClt->fd, newClt->id);
	FD_SET(newConnection, &curr_sock);
}

int main(int argc, char **argv) {

	if (argc != 2) 
	{
		write(2, "Wrong number of arguments\n", strlen("Wrong number of arguments\n"));
		return 1;
	}

	struct sockaddr_in servaddr; 

	g_sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (g_sockfd == -1)
		fatal();

	bzero(&servaddr, sizeof(servaddr)); 

	int port = atoi(argv[1]);

	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433);
	servaddr.sin_port = htons(port); 
  
	if ((bind(g_sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
		fatal(); 

	if (listen(g_sockfd, 10) != 0)
		fatal();

	printf("listening at port %d...\n", port);

	FD_ZERO(&curr_sock);
	FD_SET(g_sockfd, &curr_sock);
	char * message = NULL;
	
	while (1) 
	{
		read_sock = write_sock = curr_sock;
		int max_sd = get_max_fd();

		int select_ret = select(max_sd + 1, &read_sock, &write_sock, NULL, NULL);

		if (select_ret <= 0)
			continue ;

		for (int fd = 0 ; fd <= get_max_fd();fd++) 
		{
			if (FD_ISSET(fd, &read_sock)) 
			{
				if (fd == g_sockfd) 
				{
					add_client();
					break ;
				}
				int received = BUFFER_SIZE;
				char str[BUFFER_SIZE + 1];
				while (received == BUFFER_SIZE) 
				{
					bzero(&str, BUFFER_SIZE + 1);
					received = recv(fd, str, BUFFER_SIZE, 0);
					if (received <= 0)
						break ;
					char * ptr = message;
					message = str_join(message, str);
					if (ptr != NULL)
						free(ptr);
				}
				if (received <= 0) 
				{
					int id = get_fd_id(fd);
					broadcastLeftMessage(fd, id);
					rm_client(fd);
					FD_CLR(fd, &curr_sock);
					close(fd);
				}
				else 
				{
					if (sendMessage(message, fd) != 0)
						message = NULL;
				}
			}
		}
	}
}