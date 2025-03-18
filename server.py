#!/usr/bin/env python3
"""
server.py
Skeleton for a networked ASCII "Battle Game" server in Python.

1. Create a TCP socket and bind to <PORT>.
2. Listen and accept up to 4 client connections.
3. Maintain a global game state (grid + players + obstacles).
4. On receiving commands (MOVE, ATTACK, QUIT, etc.), update the game state
   and broadcast the updated state to all connected clients.

Usage:
   python server.py <PORT>
"""

import sys
import socket
import threading

MAX_CLIENTS = 4
BUFFER_SIZE = 1024
GRID_ROWS = 5
GRID_COLS = 5

###############################################################################
# Data Structures
###############################################################################

# Each player can be stored as a dict, for instance:
# {
#    'x': int,
#    'y': int,
#    'hp': int,
#    'active': bool
# }

# The global game state might be stored in a dictionary:
# {
#   'grid': [ [char, ...], ... ],        # 2D list of chars
#   'players': [ playerDict, playerDict, ...],
#   'clientCount': int
# }

g_gameState = {}
g_clientSockets = [None] * MAX_CLIENTS  # track client connections
g_stateLock = threading.Lock()          # lock for the game state

###############################################################################
# Initialize the game state
###############################################################################
def initGameState():
    global g_gameState
    # Create a 2D grid filled with '.'
    grid = []
    for r in range(GRID_ROWS):
        row = ['.'] * GRID_COLS
        grid.append(row)

    # Example: place a couple of obstacles '#'
    # (Feel free to add more or randomize them.)
    grid[2][2] = '#'
    grid[1][3] = '#'

    # Initialize players
    players = []
    for i in range(MAX_CLIENTS):
        p = {
            'x': -1,
            'y': -1,
            'hp': 100,
            'active': False
        }
        players.append(p)

    g_gameState = {
        'grid': grid,
        'players': players,
        'clientCount': 0
    }

###############################################################################
# Refresh the grid with current player positions.
# We clear old player marks (leaving obstacles) and re-place them according
# to the players' (x,y).
###############################################################################
def refreshPlayerPositions():
    """Clear old positions (leaving obstacles) and place each active player."""
    grid = g_gameState['grid']
    players = g_gameState['players']

    # Clear non-obstacle cells
    for r in range(GRID_ROWS):
        for c in range(GRID_COLS):
            if grid[r][c] != '#':
                grid[r][c] = '.'

    # Place each active player
    for i, player in enumerate(players):
        if player['active'] and player['hp'] > 0:
            px = player['x']
            py = player['y']
            grid[px][py] = chr(ord('A') + i)  # 'A', 'B', 'C', 'D'

###############################################################################
# TODO: Build a string that represents the current game state (ASCII grid), 
# which you can send to all clients.
###############################################################################
def buildStateString():
    # e.g., prefix with "STATE\n", then rows of the grid, then player info
    buffer = []
    buffer.append("STATE\n")

    # Copy the grid
    # ...
    # Optionally append player info

    return ''.join(buffer)


###############################################################################
# Broadcast the current game state to all connected clients
###############################################################################
def broadcastState():
    stateStr = buildStateString().encode('utf-8')
    
    # TODO: send buffer to each active client

###############################################################################
# TODO: Handle a client command: MOVE, ATTACK, QUIT, etc.
#  - parse the string
#  - update the player's position or HP
#  - call refreshPlayerPositions() and broadcastState()
###############################################################################
def handleCommand(playerIndex, cmd):
    with g_stateLock:
        players = g_gameState['players']

        # Example: parse "MOVE UP", "MOVE DOWN", etc.
        if cmd.startswith("MOVE"):
            if "UP" in cmd:
                nx = players[playerIndex]['x'] - 1
                ny = players[playerIndex]['y']
                if 0 <= nx < GRID_ROWS and g_gameState['grid'][nx][ny] != '#':
                    players[playerIndex]['x'] = nx
            elif "DOWN" in cmd:
                nx = players[playerIndex]['x'] + 1
                ny = players[playerIndex]['y']
                if nx < GRID_ROWS and g_gameState['grid'][nx][ny] != '#':
                    players[playerIndex]['x'] = nx
            # elif "LEFT" in cmd:
            # ...
            #elif "RIGHT" in cmd:
            # ...

        # elif cmd.startswith("ATTACK"):
        # ...
        # elif cmd.startswith("QUIT"):
        # ...

        # Refresh and broadcast
        refreshPlayerPositions()
        broadcastState()

###############################################################################
# Thread function: handle communication with one client
###############################################################################
def clientHandler(playerIndex):
    sock = g_clientSockets[playerIndex]

    # Initialize player
    with g_stateLock:
        g_gameState['players'][playerIndex]['x'] = playerIndex  # naive example
        g_gameState['players'][playerIndex]['y'] = 0
        g_gameState['players'][playerIndex]['hp'] = 100
        g_gameState['players'][playerIndex]['active'] = True
        refreshPlayerPositions()
        broadcastState()

    while True:
        try:
            # TODO: receive from client socket
            break
        except:
            break  # On error, break out

    # Cleanup on disconnect
    print(f"Player {chr(ord('A') + playerIndex)} disconnected.")
    sock.close()

    with g_stateLock:
        g_clientSockets[playerIndex] = None
        g_gameState['players'][playerIndex]['active'] = False
        refreshPlayerPositions()
        broadcastState()

###############################################################################
# main: set up server socket, accept clients, spawn threads
###############################################################################
def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <PORT>")
        sys.exit(1)

    port = int(sys.argv[1])

    initGameState()

    # TODO: create server socket, bind, listen
    # Example:
    # serverSock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # serverSock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    # serverSock.bind(('', port))
    # serverSock.listen(5)
    
    print(f"Server listening on port {port}...")

    while True:
        # TODO: accept new connection
        # clientSock, addr = serverSock.accept()
        # print(f"Accepted new client from {addr}")

        # 1) Lock state
        # 2) Check if g_gameState['clientCount'] < MAX_CLIENTS
        #    otherwise, reject
        #
        # 3) find a free slot in g_clientSockets
        # 4) spawn a thread => threading.Thread(target=clientHandler, args=(slot,))
        pass

    # serverSock.close()

if __name__ == "__main__":
    main()
