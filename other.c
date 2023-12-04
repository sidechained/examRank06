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

    int clientFds[MAX_CLIENTS];
    int nextFd = 0; // Identifier for the next client connection

    fd_set activeFds, readyFds; // Sets of active and ready file descriptors
    char buffer[BUFFER_SIZE];

    int serverFd = socket(AF_INET, SOCK_STREAM, 0);  // IPv4 addressing and TCP protocol
    if (serverFd < 0)
    {                    
        perror("Error creating server socket");
        exit(1);
    }

    // Set up the server address
    struct sockaddr_in serverAddress = {0};    // Structure to hold the server address
    serverAddress.sin_family = AF_INET; // IPv4
    serverAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK);  // localhost
    serverAddress.sin_port = htons(atoi(argv[1]));  // port

    if (bind(serverFd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) 
    {
        perror("Error binding server socket");
        exit(1);
    }

    // mark socket as passive socket (to be used to accept incoming connection requests using accept())
    if (listen(serverFd, MAX_CLIENTS) < 0) 
    {
        perror("Error listening on server socket");
        exit(1);
    }

    // Initialise the active sockets set
    FD_ZERO(&activeFds); // Clear the set
    FD_SET(serverFd, &activeFds); // Add the server socket to the set
    int maxFd = serverFd; // Variable to track the maximum socket descriptor

    while (1) 
    {
        // Wait for activity on the sockets
        readyFds = activeFds; // Keep an unmodified copy of the full set of active fds, to be added to on connection

        // activeFds contains the fds that are accepted and connected (i.e. the server fd and the client fds)
        // readyFds contains the fds that are ready to be read from
    
        if (select(maxFd + 1, &readyFds, NULL, NULL, NULL) < 0)
        // modifies readyFds to indicate which sockets are ready for reading
        {
            perror("Error in select");
            exit(1);
        }

        // Check each socket for activity
        for (int connFd = 0; connFd <= maxFd; connFd++) 
        {
            if (FD_ISSET(connFd, &readyFds)) 
            {
                // New client connection
                if (connFd == serverFd)
                // activity is on the server file descriptor, i.e. a new client connecting
                {
                    int clientFd = accept(serverFd, NULL, NULL);
                    if (clientFd < 0) 
                    {
                        perror("Error accepting client connection");
                        exit(1);
                    }
                    FD_SET(clientFd, &activeFds); // Add the client fd to the set of active fds
                    maxFd = (clientFd > maxFd) ? clientFd : maxFd;
                    sprintf(buffer, "server: client %d just arrived\n", nextFd);
                    send(clientFd, buffer, strlen(buffer), 0);
                    clientFds[nextFd++] = clientFd;  // Add the client socket to the array, storing for future reference
                } 
                else
                // activity is on a client file descriptor, i.e. a client sending a message
                {
                    int bytesRead = recv(connFd, buffer, sizeof(buffer) - 1, 0);
                    if (bytesRead <= 0)
                    // Client disconnected
                    {                        
                        sprintf(buffer, "server: client %d just left\n", connFd);
                        // Notify remaining clients about the disconnected client
                        for (int i = 0; i < nextFd; i++) 
                        {
                            if (clientFds[i] != connFd) 
                            {
                                send(clientFds[i], buffer, strlen(buffer), 0); // Send the disconnection message to other clients
                            }
                        }
                        // Close the socket and remove it from the active set
                        close(connFd);                          // Close the client socket
                        FD_CLR(connFd, &activeFds);         // Remove the client socket from the set of active sockets
                    } 
                    else
                    // Client sent a message
                    {
                        // Broadcast the received message to all other clients
                        buffer[bytesRead] = '\0';                  // Null-terminate the received message
                        sprintf(buffer, "client %d: %s\n", connFd, buffer);  // Add client identifier to the message

                        for (int i = 0; i < nextFd; i++) 
                        {
                            if (clientFds[i] != connFd) 
                            {
                                send(clientFds[i], buffer, strlen(buffer), 0);  // Send the message to other clients
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}