// cc mini_serv.c -o mini_serv && ./mini_serv
// nc 127.0.0.1 8081
// PROBLEM: disconnected clients don't get removed from clients array
// does it leak?

// Q: do i need to validate if port is positive?
// break: breaks out of the loops
// continue: skips the rest of the loop and goes to the next iteration

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

void print(char *str_to_print)
{
	for (int i = 0; i < strlen(str_to_print); i++)
		write(1, &str_to_print[i], 1);
}

void print_exit(char *str_to_print)
{
	print(str_to_print);
	exit(1);
}

int main(int argc, char **argv) {
	int server_fd, conn_fd;
	struct sockaddr_in servaddr, cli;
	if (argc != 2)
		print_exit("Wrong number of arguments\n");
	server_fd = socket(AF_INET, SOCK_STREAM, 0); 
	if (server_fd == -1)
		print_exit("socket creation failed...\n"); 
	print("socket successfully created..\n");
	bzero(&servaddr, sizeof(servaddr)); 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(argv[1])); 
	if ((bind(server_fd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
		print_exit("socket bind failed...\n"); 
	print("socket successfully binded..\n");
	if (listen(server_fd, 10) != 0)
		print_exit("cannot listen\n");
	print("listen success..\n");

	fd_set active_fds, ready_fds; // Sets of active and ready file descriptors
	FD_ZERO(&active_fds); // Clear the set
	FD_SET(server_fd, &active_fds); // Add the server socket to the set	

	int client_fds[MAX_CLIENTS];
	int next_fd = 0;
	char buffer[BUFFER_SIZE];
	int max_fd = server_fd; // Variable to track the maximum socket descriptor
	while(1)
	{
		ready_fds = active_fds;
		if (select(max_fd + 1, &ready_fds, NULL, NULL, NULL) < 0)
			print_exit("Error in select\n");
		for (int conn_fd = 0; conn_fd <= max_fd; conn_fd++) {
			if (!FD_ISSET(conn_fd, &ready_fds))
				continue;
			if (conn_fd == server_fd) {
				printf("new client connecting\n");
				// NEW CLIENT CONNECTING:
				socklen_t len;
				len = sizeof(cli);
				int client_fd = accept(server_fd, (struct sockaddr *)&cli, &len);
				if (client_fd < 0)
					print_exit("server acccept failed...\n");
				print("server acccepted the client...\n");
				FD_SET(client_fd, &active_fds); // Add the client fd to the set of active fds
				if (client_fd > max_fd)
					max_fd = client_fd;
				sprintf(buffer, "server: client %d just arrived\n", next_fd);
				send(client_fd, buffer, strlen(buffer), 0);
				client_fds[next_fd++] = client_fd;  // Add the client socket to the array, storing for future reference				
				continue;
			}
			// DATA IS READY:
			int bytesRead = recv(conn_fd, buffer, sizeof(buffer) - 1, 0);
			if (bytesRead <= 0) // CLIENT DISCONNECTED when no more bytes read
			{                        
				sprintf(buffer, "server: client %d just left\n", conn_fd);
				// Notify remaining clients about the disconnected client
				for (int i = 0; i < next_fd; i++) 
				{
					if (client_fds[i] != conn_fd) 
						send(client_fds[i], buffer, strlen(buffer), 0); // Send the disconnection message to other clients
				}
				close(conn_fd); // close client fd
				FD_CLR(conn_fd, &active_fds); // Remove the client fd from the set of active fd
				continue;
			} 			
			// CLIENT SENT A MESSAGE
			// Broadcast the received message to all other clients
			buffer[bytesRead] = '\0';
			char newBuffer[BUFFER_SIZE];
			strcpy(newBuffer, buffer);
			sprintf(newBuffer, "client %d: %s", conn_fd, buffer); // \n or not?
			printf("%s", newBuffer);
			for (int i = 0; i < next_fd; i++) 
			{
				if (client_fds[i] != conn_fd) 
					send(client_fds[i], newBuffer, strlen(newBuffer), 0);
			}
		}
	}
}