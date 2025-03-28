/******************************************************************************
 * server.c
 *
 * Template for a networked ASCII "Battle Game" server in C.
 *
 * 1. Create a TCP socket, bind to <PORT>, listen for connections.
 * 2. Accept up to 4 client connections.
 * 3. Manage a global game state: ASCII grid + players + obstacles.
 * 4. On receiving commands (MOVE, ATTACK, QUIT, etc.), update the game state
 *    and broadcast it to all clients.
 *
 * Compile:
 *   gcc server.c -o server -pthread
 *
 * Usage:
 *   ./server <PORT>
 ******************************************************************************/

#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
// #include <arpa/inet.h> // Optional if you want to display IP addresses

#define MAX_CLIENTS 4
#define BUFFER_SIZE 1024
#define LISTENQ 4    // Upto 4 people can wait in the lobby for a next game session
#define MAXLINE 1000 // For hostname

/* Grid dimensions */
#define GRID_ROWS 5
#define GRID_COLS 5

/*---------------------------------------------------------------------------*
 * Data Structures
 *---------------------------------------------------------------------------*/

/* Player structure */
typedef struct
{
  int x, y;   // current position
  int hp;     // health points
  int active; // 1 if this player slot is used, 0 otherwise
} Player;

/* Game state: grid + players + count */
typedef struct
{
  char grid[GRID_ROWS]
           [GRID_COLS]; // '.' for empty, '#' for obstacle, or 'A'/'B'/'C'/'D'
  Player players[MAX_CLIENTS];
  int clientCount; // how many players are connected
} GameState;

/* Global game state */
GameState g_gameState;

/* Store each client's socket; index corresponds to a player ID (0..3) */
int g_clientSockets[MAX_CLIENTS];

void initSockets()
{
  for (int i = 0; i < MAX_CLIENTS; i++)
  {
    g_clientSockets[i] = -1;
  }
}

/* Mutex to protect shared game state (recommended for thread safety) */
pthread_mutex_t g_stateMutex = PTHREAD_MUTEX_INITIALIZER;

void initGameState()
{
  // Fill the grid with '.'
  for (int r = 0; r < GRID_ROWS; r++)
  {
    for (int c = 0; c < GRID_COLS; c++)
    {
      g_gameState.grid[r][c] = '.';
    }
  }

  // Example: place some obstacles
  // (Feel free to add more or randomize them)
  g_gameState.grid[2][2] = '#';
  g_gameState.grid[1][3] = '#';

  // Initialize players
  for (int i = 0; i < MAX_CLIENTS; i++)
  {
    g_gameState.players[i].x = -1;
    g_gameState.players[i].y = -1;
    g_gameState.players[i].hp = 100;
    g_gameState.players[i].active = 0;
    g_clientSockets[i] = -1;
  }

  g_gameState.clientCount = 0;
}

/*---------------------------------------------------------------------------*
 * Refresh the grid with current player positions.
 * We clear old player marks (leaving obstacles) and re-place them according
 * to the players' (x,y).
 *---------------------------------------------------------------------------*/
void refreshPlayerPositions()
{
  // Clear all non-obstacle cells
  for (int r = 0; r < GRID_ROWS; r++)
  {
    for (int c = 0; c < GRID_COLS; c++)
    {
      if (g_gameState.grid[r][c] != '#')
      {
        g_gameState.grid[r][c] = '.';
      }
    }
  }

  // Place each active player's symbol
  for (int i = 0; i < MAX_CLIENTS; i++)
  {
    if (g_gameState.players[i].active && g_gameState.players[i].hp > 0)
    {
      int px = g_gameState.players[i].x;
      int py = g_gameState.players[i].y;
      g_gameState.grid[px][py] = 'A' + i; // 'A', 'B', 'C', 'D'
    }
  }
}

/*---------------------------------------------------------------------------*
 * TODO: Build a string that represents the current game state (ASCII grid),
 *       which you can send to all clients.
 *---------------------------------------------------------------------------*/
void buildStateString(char *outBuffer)
{
  // e.g., prefix with "STATE\n", then rows of the grid, then player info
  outBuffer[0] = '\0'; // start empty

  strcat(outBuffer, "STATE\n");

  // Copy the grid
  for (int r = 0; r < GRID_ROWS; r++)
  {
    for (int c = 0; c < GRID_COLS; c++)
    {
      sprintf(outBuffer + strlen(outBuffer), "%c", g_gameState.grid[r][c]);
    }
    strcat(outBuffer, "\n");
  }
  // ...
  strcat(outBuffer, "ACTIVE PLAYER INFO (IF EXISTS)\n");
  // Optionally append player info
  for (int i = 0; i < MAX_CLIENTS; i++)
  {
    // Check for active players
    Player active_player;
    if (g_gameState.players[i].active == 1)
    {
      active_player = g_gameState.players[i];
      printf("Is player active: INT -> %d", g_gameState.players[i].active);
      active_player = g_gameState.players[i];
      sprintf(outBuffer + strlen(outBuffer), "Player %d\n", i);
      sprintf(outBuffer + strlen(outBuffer), "Player position: (%d, %d)\n", active_player.x, active_player.y);
      sprintf(outBuffer + strlen(outBuffer), "Player health points %d\n", active_player.hp);
    }
    else
    {
      continue;
    }
  }
}

/*---------------------------------------------------------------------------*
 * Broadcast the current game state to all connected clients
 *---------------------------------------------------------------------------*/
void broadcastState()
{
  char buffer[BUFFER_SIZE];
  buildStateString(buffer);
  size_t len = strlen(buffer);

  // TODO: send buffer to each active client via send() or write()
  for (int i = 0; i < MAX_CLIENTS; i++)
  {
    // Checking for valid sockets
    if (g_clientSockets[i] != -1)
    {
      if (send(g_clientSockets[i], buffer, len, 0) < 0)
      {
        printf("Failed to send message to a socket %d", g_clientSockets[i]);
        continue;
      }
    }
    else
    {
      continue;
    }
  }
}

/*---------------------------------------------------------------------------*
 * TODO: Handle a client command: MOVE, ATTACK, QUIT, etc.
 *  - parse the string
 *  - update the player's position or HP
 *  - call refreshPlayerPositions() and broadcastState()
 *---------------------------------------------------------------------------*/
void handleCommand(int playerIndex, const char *cmd)
{
  // Lock state if needed
  pthread_mutex_lock(&g_stateMutex);

  if (strncmp(cmd, "MOVE", 4) == 0)
  {
    // Example commands: MOVE UP, MOVE DOWN, MOVE LEFT, MOVE RIGHT
    if (strstr(cmd, "UP"))
    {
      int nx = g_gameState.players[playerIndex].x > 0 ? g_gameState.players[playerIndex].x - 1 : g_gameState.players[playerIndex].x;
      int ny = g_gameState.players[playerIndex].y;
      if (nx >= 0 && g_gameState.grid[nx][ny] != '#')
      {
        g_gameState.players[playerIndex].x = nx;
      }
    }
    else if (strstr(cmd, "DOWN"))
    {
      int nx = g_gameState.players[playerIndex].x < GRID_ROWS - 1 ? g_gameState.players[playerIndex].x + 1 : g_gameState.players[playerIndex].x;
      int ny = g_gameState.players[playerIndex].y;
      if (nx >= 0 && g_gameState.grid[nx][ny] != '#')
      {
        g_gameState.players[playerIndex].x = nx;
      }
    }
    else if (strstr(cmd, "LEFT"))
    {
      int nx = g_gameState.players[playerIndex].x;
      int ny = g_gameState.players[playerIndex].y < GRID_COLS ? g_gameState.players[playerIndex].y - 1 : g_gameState.players[playerIndex].y;
      if (ny >= 0 && g_gameState.grid[nx][ny] != '#')
      {
        g_gameState.players[playerIndex].y = ny;
      }
    }
    else if (strstr(cmd, "RIGHT"))
    {
      int nx = g_gameState.players[playerIndex].x;
      int ny = g_gameState.players[playerIndex].y < GRID_COLS - 1 ? g_gameState.players[playerIndex].y + 1 : g_gameState.players[playerIndex].y;
      if (ny >= 0 && g_gameState.grid[nx][ny] != '#')
      {
        g_gameState.players[playerIndex].y = ny;
      }
    }
  }
  // else if (strncmp(cmd, "ATTACK", 6) == 0) { ... }
  // else if (strncmp(cmd, "QUIT", 4) == 0) { ... }

  // Refresh positions and broadcast
  refreshPlayerPositions();
  broadcastState();

  // Unlock
  pthread_mutex_unlock(&g_stateMutex);
}

/*---------------------------------------------------------------------------*
 * Thread function: handle communication with one client
 *---------------------------------------------------------------------------*/
void *clientHandler(void *arg)
{
  int playerIndex = *(int *)arg;
  free(arg);

  int clientSocket = g_clientSockets[playerIndex];

  // Example: set player's initial position
  pthread_mutex_lock(&g_stateMutex);
  g_gameState.players[playerIndex].x = playerIndex; // naive approach
  g_gameState.players[playerIndex].y = 0;
  g_gameState.players[playerIndex].active = 1;
  refreshPlayerPositions();
  broadcastState();
  pthread_mutex_unlock(&g_stateMutex);

  char buffer[BUFFER_SIZE];

  // Main recv loop
  while (1)
  {
    memset(buffer, 0, sizeof(buffer));
    // TODO: recv from clientSocket
    if (recv(clientSocket, buffer, BUFFER_SIZE, 0) < 0)
    {
      close(clientSocket);
      continue;
    }

    // Strip trailing newline (if any)
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n')
    {
      buffer[len - 1] = '\0';
    }

    // Handle the command
    handleCommand(playerIndex, buffer);

    // If the player used QUIT in handleCommand, we can also break here:
    pthread_mutex_lock(&g_stateMutex);
    if (g_gameState.players[playerIndex].active == 0)
    {
      pthread_mutex_unlock(&g_stateMutex);
      break;
    }
    pthread_mutex_unlock(&g_stateMutex);
  }

  // Cleanup on disconnect
  printf("Player %c disconnected.\n", 'A' + playerIndex);
  close(clientSocket);

  // Mark inactive
  pthread_mutex_lock(&g_stateMutex);
  g_clientSockets[playerIndex] = -1;
  g_gameState.players[playerIndex].active = 0;
  refreshPlayerPositions();
  broadcastState();
  pthread_mutex_unlock(&g_stateMutex);

  return NULL;
}

/*---------------------------------------------------------------------------*
 * main: set up server socket, accept clients, spawn threads
 *---------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    fprintf(stderr, "Usage: %s <PORT>\n", argv[0]);
    exit(EXIT_FAILURE);
  }
  int port = atoi(argv[1]); // No need in getaddrinfo because expects a char
  g_gameState.clientCount = 0;

  // 1. Initialize game state
  initGameState();
  initSockets();

  // Setting up addrInfo

  struct addrinfo *p, *listp, hints; // Exists in netdb.h which I had imported on top of the boilerplate
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;                   // IPv4
  hints.ai_socktype = SOCK_STREAM;             // TCP Connections
  hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG; // Since host is NULL and we need a wildcard address or on any IP address
  hints.ai_flags |= AI_NUMERICSERV;            // Using port number

  int rc, optval = 1;

  if ((rc = getaddrinfo(NULL, argv[1], &hints, &listp)) != 0)
  {
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(rc));
    return 1;
  }

  int serverSock; // Global server socket

  for (p = listp; p != NULL; p = p->ai_next)
  {
    // 2. Create server socket
    serverSock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (serverSock < 0)
    {
      perror("socket creation failed");
      continue;
    }

    // Eliminates "Address already in use" error from bind
    setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR,
               (const void *)&optval, sizeof(int));

    // 3. Bind, listen
    if (bind(serverSock, p->ai_addr, p->ai_addrlen) < 0)
    {
      perror("bind failed");
      close(serverSock);
      continue;
    }

    break;
  }

  if (p == NULL)
  {
    fprintf(stderr, "Failed to bind to any address\n");
    freeaddrinfo(listp);
    return 1;
  }

  // Free result anyways
  freeaddrinfo(listp);

  // listen(...)
  if (listen(serverSock, LISTENQ) < 0)
  {
    perror("listen failed");
    close(serverSock);
    return 1;
  }

  printf("Server listening on port %d...\n", port);

  // 4. Accept loop
  while (1)
  {
    struct sockaddr_storage clientAddr;
    socklen_t clientlen = sizeof(clientAddr);
    char client_hostname[MAXLINE], client_port[MAXLINE];

    int newSock = accept(serverSock, (struct sockaddr *)&clientAddr, &clientlen);
    if (newSock < 0)
    {
      perror("accept failed");
      continue;
    }

    // Reject new clients if it exceeds the max capacity at a time
    if (g_gameState.clientCount >= MAX_CLIENTS)
    {
      // Server is full, reject the client
      printf("Server full! Rejecting new client.\n");
      const char *msg = "Server full. Please try again later.\n";
      send(newSock, msg, strlen(msg), 0);
      close(newSock);
      continue;
    }

    getnameinfo((struct sockaddr *)&clientAddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0); // Get hostname from address
    printf("New client connected! Connected to (%s, %s). Active clients: %d/%d\n", client_hostname, client_port, g_gameState.clientCount, MAX_CLIENTS);

    // If we have capacity, find a free index in g_clientSockets
    int freeIndex;
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
      if (g_clientSockets[i] == -1)
      {
        freeIndex = i;
        break;
      }
    }

    g_clientSockets[freeIndex] = newSock; // Adding the activeClient to the array
    g_gameState.clientCount++;

    // create a thread:
    pthread_t tid;
    int *arg = malloc(sizeof(int));
    *arg = freeIndex;
    pthread_create(&tid, NULL, clientHandler, arg);
    pthread_detach(tid);
  }

  close(serverSock);
  return 0;
}
