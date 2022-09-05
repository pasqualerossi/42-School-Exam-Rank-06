#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

int extract_message(char **buffer, char **message)
{
	char	*new_buffer;
	int		i = 0;
	
	*message = 0;
	if (*buffer == 0)
		return (0);
	while ((*buffer)[i])
	{
		if ((*buffer)[i] == '\n')
		{
			new_buffer = calloc(1, sizeof(*new_buffer) * (strlen(*buffer + i + 1) + 1));
			if (new_buffer == 0)
				return (-1);
			strcpy(new_buffer, *buffer + i + 1);
			*message = *buffer;
			(*message)[i + 1] = 0;
			*buffer = new_buffer;
			return (1);
		}
		i++;
	}
	return (0);
}

char *string_join(char *buffer, char *add)
{
	char	*new_buffer;
	int		length;

	if (buffer == 0)
		length = 0;
	else
		length = strlen(buffer);
	new_buffer = malloc(sizeof(*new_buffer) * (length + strlen(add) + 1));
	if (new_buffer == 0)
		return (0);
	new_buffer[0] = 0;
	if (buffer != 0)
		strcat(new_buffer, buffer);
	free(buffer);
	strcat(new_buffer, add);
	return (new_buffer);
}


int main() 
{
	int sockfd, connfd, len;
	struct sockaddr_in servaddr, cli; 

	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) 
	{ 
		printf("socket creation failed...\n"); 
		exit(0); 
	} 
	else
		printf("Socket successfully created..\n"); 
	bzero(&servaddr, sizeof(servaddr)); 

	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433);
	servaddr.sin_port = htons(8081); 
  
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) 
	{ 
		printf("socket bind failed...\n"); 
		exit(0); 
	} 
	else
		printf("Socket successfully binded..\n");
	
	if (listen(sockfd, 10) != 0) 
	{
		printf("cannot listen\n"); 
		exit(0); 
	}
	len = sizeof(cli);
	connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
	if (connfd < 0) 
	{ 
        printf("server acccept failed...\n"); 
        exit(0); 
    } 
    else
        printf("server acccept the client...\n");
}