# Networked ASCII "Battle Game"

## Overview

This assignment requires you to implement a **client-server** ASCII-based game using **TCP sockets**.  
You will:
1. **Design an application-level protocol** (how clients and server exchange messages).
2. **Implement the socket code** (on both the server and client side).
3. **Manage concurrency** on the server (e.g., using threads or I/O multiplexing).
4. **Implement game logic** for a simple “battle” scenario with movement, attacks, and obstacles.

You may complete this assignment in **C** or **Python** (both templates are provided).

---

## Requirements

1. **Server**
    - Listens on a TCP socket.
    - Accepts up to 4 clients.
    - Maintains a **GameState** (a 2D grid).
    - Supports commands like **MOVE**, **ATTACK**, **QUIT**, etc.
    - After each valid command, it **broadcasts** the updated game state to all players.

2. **Client**
    - Connects to the server via TCP.
    - Sends user-typed commands.
    - Continuously **receives** and displays updates of the ASCII grid (plus any extra info).

3. **Protocol**
    - Must be documented (in code comments or a separate file/section).
    - Could be plain text like `"MOVE UP"`, `"ATTACK"`, `"QUIT"`, etc.
    - Server can respond with messages like `"STATE\n"` (followed by the ASCII grid) or `"ERROR\n"` if needed.

4. **Game Logic**
    - 2D grid (e.g., 5×5).
    - **Obstacles** (`#`) block movement.
    - **Players** are labeled `'A', 'B', 'C', 'D'`.
    - **Attacks**: Deduct HP from adjacent players or define your own rules.
    - **Quit**: On `QUIT`, the client disconnects, and the server updates the state accordingly.

5. **Extra Features** (Optional)
    - Turn-based mechanics.
    - More advanced attacking.
    - Security checks (e.g., invalid commands).

---

## Instructions (C Version)

1. Open `server.c` and `client.c`.
2. Complete all **`TODO`** sections.
3. **Compile**:
   ```bash
   # Server
   gcc server.c -o server -pthread

   # Client
   gcc client.c -o client -pthread
   ```
   
    Or use `clang`:

    ```bash
    clang server.c -o server -pthread
    clang client.c -o client -pthread
    ```

4. **Run**:
    ```bash
    # Terminal 1: start the server on port 12345
    ./server 12345

    # Terminal 2..N: start clients
    ./client 127.0.0.1 12345
    ```
   
---

## Instructions (Python Version)

1. Open `server.py` and `client.py`.
2. Complete all **TODO** sections.
3. **Run**:
```shell
# Server
python server.py 12345

# Client
python client.py 127.0.0.1 12345
```
Where `12345` is your chosen port, and `127.0.0.1` is the server IP (local host).

**Have fun building your networked ASCII Battle Game!**
