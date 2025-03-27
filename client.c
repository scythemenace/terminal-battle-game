/******************************************************************************
 * client.c
 *
 * Template for a networked ASCII "Battle Game" client in C.
 *
 * 1. Connect to the server via TCP.
 * 2. Continuously read user input (e.g. MOVE, ATTACK, QUIT).
 * 3. Send commands to the server.
 * 4. Spawn a thread to receive and display the updated game state from the server.
 *
 * Compile:
 *   gcc client.c -o client -pthread
 *
 * Usage:
 *   ./client <SERVER_IP> <PORT>
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

/* Global server socket used by both main thread and receiver thread. */
int g_serverSocket = -1;

/*---------------------------------------------------------------------------*
 * Thread to continuously receive updates (ASCII grid) from the server
 *---------------------------------------------------------------------------*/
void *receiverThread(void *arg)
{
    (void)arg; // unused

    char buffer[BUFFER_SIZE];

    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        // TODO: recv from g_serverSocket
        ssize_t bytesRead = recv(g_serverSocket, buffer, BUFFER_SIZE, 0);
        if (bytesRead <= 0)
        {
            printf("Disconnected from server.\n");
            break;
        }

        // Print the game state or server message
        printf("\n%s\n", buffer);
        fflush(stdout);
    }

    close(g_serverSocket);
    exit(0);
    return NULL;
}

/*---------------------------------------------------------------------------*
 * main: connect to server, spawn receiver thread, send commands in a loop
 *---------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <SERVER_IP> <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *serverIP = argv[1];
    int port = atoi(argv[2]); // No need in getaddrinfo because expects a char

    struct addrinfo hints, *listp, *p;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM; // TCP Connection
    hints.ai_flags = AI_NUMERICSERV; // using numeric port tag
    hints.ai_flags |= AI_ADDRCONFIG;
    getaddrinfo(serverIP, argv[2], &hints, &listp);

    for (p = listp; p != NULL; p = p->ai_next)
    {
        // 1. Create socket
        if ((g_serverSocket = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
        {
            perror("global socket creation failed");
            continue;
        }

        // 2. connect
        if (connect(g_serverSocket, p->ai_addr, p->ai_addrlen) < 0)
        {
            perror("failed to connect");
            close(g_serverSocket);
            continue;
        }

        break; // Connected successfully
    }

    printf("Connected to server %s:%d\n", serverIP, port);

    // 3. Create a receiver thread
    pthread_t recvThread;
    pthread_create(&recvThread, NULL, receiverThread, NULL);
    pthread_detach(recvThread);

    // 4. Main loop: read user commands, send to server
    while (1)
    {
        char command[BUFFER_SIZE];
        memset(command, 0, sizeof(command));

        printf("Enter command (MOVE/ATTACK/QUIT): ");
        fflush(stdout);

        if (fgets(command, sizeof(command), stdin) == NULL)
        {
            // Possibly user pressed Ctrl+D
            printf("Exiting client.\n");
            break;
        }

        // Remove trailing newline
        size_t len = strlen(command);
        if (len > 0 && command[len - 1] == '\n')
        {
            command[len - 1] = '\0';
        }

        // TODO:
        send(g_serverSocket, command, strlen(command), 0);

        // If QUIT => break
        if (strncmp(command, "QUIT", 4) == 0)
        {
            break;
        }
    }

    // Cleanup
    close(g_serverSocket);
    return 0;
}
