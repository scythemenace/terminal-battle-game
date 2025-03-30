[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/GT67vYVA)

# Networked ASCII Battle Game

## Overview

The Networked ASCII Battle Game is a turn-based multiplayer game implemented in C, where up to 4 players can compete on a 5x5 ASCII grid. Players can move, attack with shurikens, or quit the game, with the server managing the game state and broadcasting updates to all connected clients. The game uses TCP for reliable communication and supports a simple text-based protocol for client-server interaction.

## Game Logic Description

- **Grid**: The game is played on a 5x5 grid, where:
  - `.` represents an empty cell.
  - `#` represents an obstacle (fixed at positions (2,2) and (1,3)).
  - `A`, `B`, `C`, `D` represent players (up to 4 players).
  - `*` represents a shuriken.
- **Players**: Each player starts with 100 HP and is assigned a position (e.g., Player A at (0,0), Player B at (1,0), etc.).
- **Turn-Based Gameplay**: The game operates on a turn-based system, starting with Player A. Players take turns to:
  - Move one cell in a specified direction.
  - Attack by launching a shuriken.
  - Quit the game.
- **Shuriken**: When a player attacks, a shuriken is launched in the specified direction. It moves one cell per turn until it hits a player (dealing 50 damage), an obstacle, or goes out of bounds.
- **Winning Condition**: The last player with HP greater than 0 wins. A player dies if their HP drops to 0 or below.

### Extra Features

We enhanced the game with the following features:

- **Shuriken**: Players can attack with a shuriken, dealing 50 damage on hit.
- **Turn-by-Turn Gameplay**: Ensures fair play by rotating turns among active players.
- **Proper QUIT Mechanics**: Notifies all players, resets the quitter’s state, and closes their socket cleanly.
- **Border Adherence**: Prevents players from moving out of the 5x5 grid, ensuring valid moves within boundaries.

## Compilation Instructions

The game consists of two programs: `server.c` (the server) and `client.c` (the client). Both require a C compiler (e.g., `gcc`) and the POSIX threads library (`-pthread`).

### Prerequisites

- A C compiler (e.g., `gcc`).
- POSIX threads support (available on Unix-like systems, including Linux and macOS).

### Compile the Server

```bash
gcc server.c -o server -pthread
```

### Compile the Client

```bash
gcc client.c -o client -pthread
```

## Running the Game

1. **Start the Server**:

   - Run the server on a specified port (e.g., 12345):
     ```bash
     ./server 12345
     ```
   - The server will listen for incoming connections and can handle up to 4 clients.

2. **Connect Clients**:

   - In separate terminal windows, run the client, connecting to the server’s IP and port (e.g., `127.0.0.1` for localhost):
     ```bash
     ./client 127.0.0.1 12345
     ```
   - Repeat this for up to 4 clients (each client will be assigned a player ID: A, B, C, or D).

3. **Play the Game**:
   - The game starts once the first client connects (Player A gets the first turn).
   - Follow the prompts to enter commands (see below).

## Protocol Overview

The game uses a text-based protocol over TCP for client-server communication.

### Commands

- **MOVE <DIRECTION>**: Moves the player in the specified direction (UP, DOWN, LEFT, RIGHT).
  - Example: `MOVE UP`
- **ATTACK <DIRECTION>**: Launches a shuriken in the specified direction (UP, DOWN, LEFT, RIGHT).
  - Example: `ATTACK DOWN`
- **QUIT**: Removes the player from the game.
  - Example: `QUIT`

### Server Messages

- **Game State**: `"STATE:\n\n<grid>\n\nACTIVE PLAYER INFO (IF EXISTS)\n<player info>"` (shows the grid and player details).
- **Turn Notifications**:
  - Current player: `"It's your turn, Player X\n"`.
  - Other players: `"It's Player X's turn\n"`.
- **Error**: `"Sorry, it's not your turn\n"` (if a player acts out of turn).
- **Death**: `"You have died!\n"` (when a player’s HP drops to 0).
- **Quit**:
  - Quitting player: `"You have quit the game.\n"`.
  - Other players: `"Player X has quit the game.\n"`.
- **Disconnect**: `"Player X has disconnected.\n"` (if a player disconnects unexpectedly).

### Message Flow

- **Client Connects**: The client connects, the server assigns a player ID, and broadcasts the game state.
- **Turn-Based Loop**:
  - The server notifies the current player of their turn.
  - The client sends a command → the server processes it → broadcasts the updated state → rotates the turn.
- **Shuriken Movement**: At the start of each turn, shurikens move and collision checks are performed.
- **Client Disconnects**: If a client sends QUIT or disconnects unexpectedly, the server resets their state and broadcasts the updated state.

## How to Play

1. **Start the Game**:

   - Launch the server and connect clients as described above.
   - The first client (Player A) starts the game and gets the first turn.

2. **Understand the Game State**:

   - The server sends the game state after every action, showing the 5x5 grid and player info (position, HP).
   - Example:

     ```
     STATE:

     A....
     B...#
     ..#..
     .....
     .....

     ACTIVE PLAYER INFO (IF EXISTS)
     Player 0
     Player position: (0, 0)
     Player health points 100
     Player 1
     Player position: (1, 0)
     Player health points 100
     ```

3. **Take Your Turn**:

   - When it’s your turn, the server will send `"It's your turn, Player X\n"`.
   - Enter a command at the prompt (e.g., `MOVE UP`, `ATTACK DOWN`, `QUIT`).
   - If it’s not your turn, you’ll see `"It's Player X's turn\n"`.

4. **Attack with Shurikens**:

   - Use `ATTACK <DIRECTION>` to launch a shuriken. It will move one cell per turn in the specified direction until it hits a player (dealing 50 damage), an obstacle, or goes out of bounds.
   - If a player’s HP drops to 0, they are removed from the game.

5. **Quit the Game**:

   - Enter `QUIT` to leave the game. Other players will be notified, and your state will be reset.

6. **Win the Game**:
   - The last player with HP greater than 0 wins the game.

## Troubleshooting

- **Server Full**: If you see `"Server full. Please try again later.\n"`, the server already has 4 clients. Wait for a player to quit or disconnect.
- **Connection Issues**: Ensure the server is running and the IP/port are correct when connecting the client.
- **Out-of-Turn Commands**: If you see `"Sorry, it's not your turn\n"`, wait for your turn to act.
