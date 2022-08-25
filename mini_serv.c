#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>

fd_set wfds, rfds, fds;

int sock_fd_size = 0;
int client_ids[10000];
int client_nb = 0;

char recv_buf[1024];
char *msg[10000];
char wbuf[50];

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
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}

void fatal_error(void)
{
    write(2, "Fatal error\n", 12);
    exit(1);
}

void notify_clients(int wfd, char* buf)
{
	for (int fd = 0; fd <= sock_fd_size; fd++)
	{
		if (fd != wfd && FD_ISSET(fd, &wfds))
			send(fd, buf, strlen(buf), 0);
	}
}

void	serve(int fd)
{
	char	*s;
	while (extract_message(&(msg[fd]), &s))
	{
		sprintf(wbuf, "client %d: ", client_ids[fd]);
		notify_clients(fd, wbuf);
		notify_clients(fd, s);
		free(s);
		s = NULL;
	}
}

int main(int ac, char *av[])
{
    struct sockaddr_in servaddr;
    int sockfd;

    if (ac != 2)
    {
        write (2, "Wrong number of arguments\n", 26);
        exit(1);
    }
    FD_ZERO(&fds);
    bzero(&servaddr, sizeof(servaddr)); 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433);
	servaddr.sin_port = htons(atoi(av[1]));
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        fatal_error();
	sock_fd_size  = sockfd;
    FD_SET(sockfd, &fds);
    if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
        fatal_error();
	if (listen(sockfd, SOMAXCONN))
		fatal_error();
	while (1)
	{
		rfds = wfds = fds;
		if (select(sock_fd_size + 1, &rfds, &wfds, NULL, NULL) < 0)
			fatal_error();
		for (int fd = 0; fd <= sock_fd_size; fd++)
		{
			if (FD_ISSET(fd, &rfds))
			{
				if (fd == sockfd)
				{
					socklen_t len = sizeof(servaddr);
					int client = accept(fd, (struct sockaddr *)&servaddr, &len);
					if (client >= 0)
					{
                        if (client > sock_fd_size)
                            sock_fd_size = client;
                        client_ids[client] = client_nb++;
                        msg[client] = NULL;
                        FD_SET(client, &fds);
                        sprintf(wbuf, "server: client %d just arrived\n", client_ids[client]);
                        notify_clients(client, wbuf);
                        break;
					}
				}
				else
				{
					int ret = recv(fd, recv_buf, 1024, 0);
					if (ret <= 0)
					{
						sprintf(wbuf, "server: client %d just left\n", client_ids[fd]);
						notify_clients(fd, wbuf);
						free(msg[fd]);
						msg[fd] = NULL;
						FD_CLR(fd, &fds);
						close(fd);
						break;
					}
					recv_buf[ret] = '\0';
					msg[fd] = str_join(msg[fd], recv_buf);
					serve(fd);
				}
			}
		}
	}
    return 0;
}
