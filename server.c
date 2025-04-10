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

typedef struct
{
  int x, y;        // shuriken pos
  int dx, dy;      // Direction
  int active;      // Unactive it it hits a wall
  int justSpawned; // 1 if just spawned, 0 otherwise
} Shuriken;

/* Player structure */
typedef struct
{
  int x, y;          // current position
  int hp;            // health points
  int active;        // 1 if this player slot is used, 0 otherwise
  Shuriken shuriken; // Each player has one shuriken
} Player;

/* Game state: grid + players + count */
typedef struct
{
  char grid[GRID_ROWS]
           [GRID_COLS]; // '.' for empty, '#' for obstacle, or 'A'/'B'/'C'/'D'
  Player players[MAX_CLIENTS];
  int clientCount; // how many players are connected
  int currentTurn; // Index of the player whose turn it is
  int gameStarted; // 0 if no players have connected yet, 1 after first player connects
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

// Shifted reset logic from init to a function
void resetPlayerState(int playerIndex)
{
  g_gameState.players[playerIndex].x = -1;
  g_gameState.players[playerIndex].y = -1;
  g_gameState.players[playerIndex].hp = 100;
  g_gameState.players[playerIndex].active = 0;
  g_gameState.players[playerIndex].shuriken.x = -1;
  g_gameState.players[playerIndex].shuriken.y = -1;
  g_gameState.players[playerIndex].shuriken.dx = 0;
  g_gameState.players[playerIndex].shuriken.dy = 0;
  g_gameState.players[playerIndex].shuriken.active = 0;
  g_gameState.players[playerIndex].shuriken.justSpawned = 0;
}

void initGameState()
{
  for (int r = 0; r < GRID_ROWS; r++)
  {
    for (int c = 0; c < GRID_COLS; c++)
    {
      g_gameState.grid[r][c] = '.';
    }
  }

  g_gameState.grid[2][2] = '#';
  g_gameState.grid[1][3] = '#';

  for (int i = 0; i < MAX_CLIENTS; i++)
  {
    resetPlayerState(i);
    g_clientSockets[i] = -1;
  }

  g_gameState.clientCount = 0;
  g_gameState.currentTurn = 0;
  g_gameState.gameStarted = 0;
}

// Function to send a message to a player via their socket
void sendMessageToPlayer(int playerIndex, const char *message)
{
  if (g_clientSockets[playerIndex] != -1)
  {
    send(g_clientSockets[playerIndex], message, strlen(message), 0);
  }
}

// Transfered logic for shuriken collision handling to helper function
int checkShurikenCollision(int shurikenOwnerIndex, int shurikenX, int shurikenY)
{
  int hitPlayerIndex = -1;
  for (int j = 0; j < MAX_CLIENTS; j++)
  {
    if (g_gameState.players[j].active && g_gameState.players[j].hp > 0)
    {
      if (g_gameState.players[j].x == shurikenX && g_gameState.players[j].y == shurikenY)
      {
        hitPlayerIndex = j;
        break;
      }
    }
  }

  if (hitPlayerIndex != -1) // A player was hit
  {
    g_gameState.players[hitPlayerIndex].hp -= 50;
    printf("Player %c hit by shuriken! HP reduced to %d\n", 'A' + hitPlayerIndex, g_gameState.players[hitPlayerIndex].hp);
    fflush(stdout);

    // Deactivate shuriken after hitting a player
    g_gameState.players[shurikenOwnerIndex].shuriken.active = 0;

    // Check if player's HP is 0 or less
    if (g_gameState.players[hitPlayerIndex].hp <= 0)
    {
      printf("Player %c has been defeated!\n", 'A' + hitPlayerIndex);
      fflush(stdout);

      // Send "You have died!" message to the player
      const char *deathMessage = "You have died!\n";
      sendMessageToPlayer(hitPlayerIndex, deathMessage);

      g_gameState.players[hitPlayerIndex].active = 0;
      if (g_clientSockets[hitPlayerIndex] != -1)
      {
        close(g_clientSockets[hitPlayerIndex]);
        g_clientSockets[hitPlayerIndex] = -1;
        g_gameState.clientCount--;
      }
    }
    return 1; // Collision occurred
  }
  return 0; // No collision
}

// Function to rotate turns to make sure the game works on a turn by turn basis
void rotateTurn()
{
  int originalTurn = g_gameState.currentTurn;

  // Remainder obviously can't be higher than the divisor, so conveniently I can get the next turn
  int nextTurn = (g_gameState.currentTurn + 1) % MAX_CLIENTS;

  // Find the next active player
  while (nextTurn != originalTurn)
  {
    if (g_gameState.players[nextTurn].active && g_gameState.players[nextTurn].hp > 0)
    {
      break;
    }
    nextTurn = (nextTurn + 1) % MAX_CLIENTS;
  }

  // If we looped back to the original turn and no other players are active, keep the turn
  if (nextTurn == originalTurn && (!g_gameState.players[nextTurn].active || g_gameState.players[nextTurn].hp <= 0))
  {
    // No active players left, reset turn to 0 (or handle game over)
    g_gameState.currentTurn = 0;
    return;
  }

  g_gameState.currentTurn = nextTurn;

  // Notify the player whose turn it is
  char turnMessage[BUFFER_SIZE];
  snprintf(turnMessage, BUFFER_SIZE, "\nIt's your turn, Player %c\n", 'A' + g_gameState.currentTurn);
  sendMessageToPlayer(g_gameState.currentTurn, turnMessage);

  // Notify all other players whose turn it is
  char otherMessage[BUFFER_SIZE];
  snprintf(otherMessage, BUFFER_SIZE, "\nIt's Player %c's turn\n", 'A' + g_gameState.currentTurn);
  for (int i = 0; i < MAX_CLIENTS; i++)
  {
    if (i != g_gameState.currentTurn && g_clientSockets[i] != -1)
    {
      sendMessageToPlayer(i, otherMessage);
    }
  }
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

  // Place each active shuriken
  for (int i = 0; i < MAX_CLIENTS; i++)
  {
    if (g_gameState.players[i].shuriken.active)
    {
      int x = g_gameState.players[i].shuriken.x;
      int y = g_gameState.players[i].shuriken.y;
      g_gameState.grid[x][y] = '*';
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
 * Build a string that represents the current game state (ASCII grid),
 *       which you can send to all clients.
 *---------------------------------------------------------------------------*/
void buildStateString(char *outBuffer)
{
  // e.g., prefix with "STATE\n", then rows of the grid, then player info
  outBuffer[0] = '\0'; // start empty

  strcat(outBuffer, "\nSTATE:\n\n");

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
  strcat(outBuffer, "\nACTIVE PLAYER INFO (IF EXISTS)\n");
  // Optionally append player info
  for (int i = 0; i < MAX_CLIENTS; i++)
  {
    // Check for active players
    Player active_player;
    if (g_gameState.players[i].active == 1)
    {

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

  // send buffer to each active client via send() or write()
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
 * Handle a client command: MOVE, ATTACK, QUIT, etc.
 *  - parse the string
 *  - update the player's position or HP
 *  - call refreshPlayerPositions() and broadcastState()
 *---------------------------------------------------------------------------*/
void handleCommand(int playerIndex, const char *cmd)
{
  pthread_mutex_lock(&g_stateMutex);

  // Check if it's the player's turn
  if (playerIndex != g_gameState.currentTurn)
  {
    const char *notYourTurnMsg = "Sorry, it's not your turn\n";
    sendMessageToPlayer(playerIndex, notYourTurnMsg);
    pthread_mutex_unlock(&g_stateMutex);
    return;
  }

  // Move all active shurikens and check for collisions before the player's action
  for (int i = 0; i < MAX_CLIENTS; i++)
  {
    if (g_gameState.players[i].shuriken.active)
    {
      if (g_gameState.players[i].shuriken.justSpawned)
      {
        g_gameState.players[i].shuriken.justSpawned = 0;
        continue;
      }

      int oldX = g_gameState.players[i].shuriken.x;
      int oldY = g_gameState.players[i].shuriken.y;
      int nx = oldX + g_gameState.players[i].shuriken.dx;
      int ny = oldY + g_gameState.players[i].shuriken.dy;

      if (nx < 0 || nx >= GRID_ROWS || ny < 0 || ny >= GRID_COLS || g_gameState.grid[nx][ny] == '#')
      {
        g_gameState.players[i].shuriken.active = 0;
        continue;
      }

      g_gameState.players[i].shuriken.x = nx;
      g_gameState.players[i].shuriken.y = ny;

      if (checkShurikenCollision(i, nx, ny))
      {
        continue;
      }

      g_gameState.grid[nx][ny] = '*';
    }
  }

  // Process the player's command
  if (strncmp(cmd, "MOVE", 4) == 0)
  {
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
  else if (strncmp(cmd, "ATTACK", 6) == 0)
  {
    if (g_gameState.players[playerIndex].shuriken.active)
    {
      pthread_mutex_unlock(&g_stateMutex);
      return;
    }

    int px = g_gameState.players[playerIndex].x;
    int py = g_gameState.players[playerIndex].y;
    int dx = 0, dy = 0;

    if (strstr(cmd, "UP"))
    {
      dx = -1;
    }
    else if (strstr(cmd, "DOWN"))
    {
      dx = 1;
    }
    else if (strstr(cmd, "LEFT"))
    {
      dy = -1;
    }
    else if (strstr(cmd, "RIGHT"))
    {
      dy = 1;
    }

    int tx = px + dx;
    int ty = py + dy;
    if (tx >= 0 && tx < GRID_ROWS && ty >= 0 && ty < GRID_COLS && g_gameState.grid[tx][ty] != '#')
    {
      g_gameState.players[playerIndex].shuriken.x = tx;
      g_gameState.players[playerIndex].shuriken.y = ty;
      g_gameState.players[playerIndex].shuriken.dx = dx;
      g_gameState.players[playerIndex].shuriken.dy = dy;
      g_gameState.players[playerIndex].shuriken.active = 1;
      g_gameState.players[playerIndex].shuriken.justSpawned = 1;
      g_gameState.grid[tx][ty] = '*';

      checkShurikenCollision(playerIndex, tx, ty);
    }
  }
  else if (strncmp(cmd, "QUIT", 4) == 0)
  {
    // Notify the player they are quitting
    const char *quitMessage = "\nhYou have quit the game.\n";
    sendMessageToPlayer(playerIndex, quitMessage);

    // Notify other players that this player has quit
    char otherMessage[BUFFER_SIZE];
    snprintf(otherMessage, BUFFER_SIZE, "\nPlayer %c has quit the game.\n", 'A' + playerIndex);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
      if (i != playerIndex && g_clientSockets[i] != -1)
      {
        sendMessageToPlayer(i, otherMessage);
      }
    }

    // Reset the player's state
    resetPlayerState(playerIndex);

    // Close their socket
    if (g_clientSockets[playerIndex] != -1)
    {
      close(g_clientSockets[playerIndex]);
      g_clientSockets[playerIndex] = -1;
      g_gameState.clientCount--;
    }

    // Refresh positions and broadcast the updated state
    refreshPlayerPositions();
    broadcastState();

    // Rotate turn if the quitting player was the current turn
    if (playerIndex == g_gameState.currentTurn)
    {
      rotateTurn();
    }

    pthread_mutex_unlock(&g_stateMutex);
    return;
  }

  // Refresh positions and broadcast
  refreshPlayerPositions();
  broadcastState();

  // Rotate the turn to the next player
  rotateTurn();

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

  pthread_mutex_lock(&g_stateMutex);
  g_gameState.players[playerIndex].x = playerIndex;
  g_gameState.players[playerIndex].y = 0;
  g_gameState.players[playerIndex].active = 1;

  if (!g_gameState.gameStarted)
  {
    g_gameState.gameStarted = 1;
    const char *yourTurnMsg = "\nIt's your turn, Player A\n";
    sendMessageToPlayer(playerIndex, yourTurnMsg);
  }

  refreshPlayerPositions();
  broadcastState();
  pthread_mutex_unlock(&g_stateMutex);

  char buffer[BUFFER_SIZE];

  while (1)
  {
    memset(buffer, 0, sizeof(buffer));
    int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
    if (bytesReceived <= 0) // Client disconnected
    {
      pthread_mutex_lock(&g_stateMutex);

      // Notify other players that this player has disconnected
      char disconnectMessage[BUFFER_SIZE];
      snprintf(disconnectMessage, BUFFER_SIZE, "\nPlayer %c has disconnected.\n", 'A' + playerIndex);
      for (int i = 0; i < MAX_CLIENTS; i++)
      {
        if (i != playerIndex && g_clientSockets[i] != -1)
        {
          sendMessageToPlayer(i, disconnectMessage);
        }
      }

      // Reset the player's state
      resetPlayerState(playerIndex);

      // Close the socket
      close(clientSocket);
      g_clientSockets[playerIndex] = -1;
      g_gameState.clientCount--;

      // Refresh and broadcast the updated state
      refreshPlayerPositions();
      broadcastState();

      // Rotate turn if the disconnected player was the current turn
      if (playerIndex == g_gameState.currentTurn)
      {
        rotateTurn();
      }

      pthread_mutex_unlock(&g_stateMutex);
      break;
    }

    // Strip trailing newline (if any)
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n')
    {
      buffer[len - 1] = '\0';
    }

    // Handle the command
    handleCommand(playerIndex, buffer);

    // Check if the player quit via command
    pthread_mutex_lock(&g_stateMutex);
    if (g_gameState.players[playerIndex].active == 0)
    {
      pthread_mutex_unlock(&g_stateMutex);
      break;
    }
    pthread_mutex_unlock(&g_stateMutex);
  }

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

    // If we have capacity, find a free index in g_clientSockets
    int freeIndex;
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
      if (g_clientSockets[i] == -1) // Change to ==
      {
        freeIndex = i;
        break;
      }
    }

    g_clientSockets[freeIndex] = newSock; // Adding the activeClient to the array
    g_gameState.clientCount++;

    printf("New client connected! Connected to (%s, %s). Active clients: %d/%d\n", client_hostname, client_port, g_gameState.clientCount, MAX_CLIENTS);

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