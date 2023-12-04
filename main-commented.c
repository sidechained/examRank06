#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <stdlib.h>
#include <stdio.h>

#include <sys/time.h>
#include <sys/types.h>

#define MAX_CLIENTS 10

// handle incoming data from clients. buf fills in chunks, whenever the chunk contains a newline, extract the message from the buffer and return it
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

// join two strings together, when would this be used?
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

void put_str(char *str)
{
	int	i;

	i = 0;
	while (str[i])
		i++;
	write(1, str, i);
}

// creates a socket and binds to it
// - printf replaced with put_str
// - htonl and htons replaced by hardcoded hex values (to memorise)
int main() {
	int fds[MAX_CLIENTS];
	int num_clients = 0;

	int sock_fd, conn_fd, len;
	struct sockaddr_in servaddr, cli; 

	// socket create and verification 
	sock_fd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sock_fd == -1) { 
		put_str("socket creation failed...\n"); 
		exit(0); 
	} 
	else
		put_str("Socket successfully created..\n"); 
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = 0x0100007F; //127.0.0.1
	servaddr.sin_port = 0x911F; // 8081 
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sock_fd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) { 
		put_str("socket bind failed...\n"); 
		exit(0); 
	} 
	else
		put_str("Socket successfully binded..\n");
	if (listen(sock_fd, MAX_CLIENTS) != 0) {
		put_str("cannot listen\n"); 
		exit(0); 
	}
	len = sizeof(cli);
	conn_fd = accept(sock_fd, (struct sockaddr *)&cli, &len);
	if (conn_fd < 0) { 
        put_str("server acccept failed...\n"); 
        exit(0); 
    } 
    else
	{
		// accept client connections, read data from them, extract messages from this data and send to other clients
        put_str("server acccept the client...\n");
		fds[num_clients++] = conn_fd;
		fd_set rfds;
		struct timeval tv;
		FD_ZERO(&rfds);
		FD_SET(conn_fd, &rfds);
		// set timeout to 0 to return immediately (non-blocking)
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		int retval = select(conn_fd + 1, &rfds, NULL, NULL, &tv);
		// retval is the total number of ready file descriptors in all the sets
		if (retval == -1)
			perror("select()");
		else if (retval == 0)
			put_str("None of the file descriptors are ready.\n");
		else if (FD_ISSET(conn_fd, &rfds))
		{
			put_str("Data is available now.\n");
			// read data and send to other clients
			char buf[1024];
			char *msg = NULL;
			int len = recv(conn_fd, buf, sizeof(buf) - 1, 0);
			if (len > 0) {
				buf[len] = '\0';
				extract_message(&buf, &msg);
				int i = 0;
				while(i < num_clients)
				{
					if (fds[i] != conn_fd)
						send(fds[i], msg, strlen(msg), 0);
					i++;
				}
			}
		}
		else
			put_str("No data within five seconds.\n");
	}
}