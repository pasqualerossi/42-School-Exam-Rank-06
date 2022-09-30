#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>

typedef int bool;

bool	check_arguments(int argc)
{
	return (argc > 1);
}

void	putstr_fd(char *str, int fd)
{
	write(fd, str, strlen(str));
}

void	arguments_error(void)
{
	putstr_fd("Wrong number of arguments\n", 2);
	exit(1);
}

void	fatal_error(void)
{
	putstr_fd("Fatal error\n", 2);
	exit(1);
}

int get_port(char *arg)
{
	int	port = atoi(arg);
	return (port);
}

int	main(int argc, char **argv)
{
	int						port;
	int						server_fd;
	int						client_fd;
	char					buf[1024];
	struct sockaddr_in		addr;
	socklen_t 				addr_len;
	fd_set					fds;

	if (!check_arguments(argc))
		arguments_error();
	if ((port = get_port(argv[1])) == -1)
		fatal_error();
	printf("port = %d\n", port);
	server_fd = socket(AF_INET, SOCK_STREAM, 0);

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr_len = sizeof(addr);
	if (bind(server_fd, (struct sockaddr *)&addr, addr_len)== -1)
		fatal_error();
	if (listen(server_fd, 0) == -1)
		fatal_error();
	while (1)
	{
		if ((client_fd = accept(server_fd, (struct sockaddr *)&addr, &addr_len)) == -1)
			fatal_error();
		FD_ZERO(&fds);
		FD_SET(client_fd, &fds);
		select(client_fd + 1, &fds, NULL, NULL, NULL);
		if (FD_ISSET(client_fd, &fds))
		{
			recv(client_fd, buf, 1024, 0);
			printf("Recv: %s\n", buf);
		}
	}
	return (0);
}