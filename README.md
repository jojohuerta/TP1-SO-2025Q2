# ChompChamps: IPC Game Engine & Champion AI
This project was developed during the second semester of 2025

## Overview
Multi-process snake-like game engine and autonomous Artificial Intelligence built entirely in C. The custom AI algorithm achieved **1st Place** in the global university tournament out of approximately 30 independent algorithms.

In this game, players are placed on a rectangular grid containing rewards, and as they move across the board, they collect points from the cells they visit.

## Core System Architecture
The engine relies on advanced Inter-Process Communication (IPC) mechanisms, utilizing shared memory and pipes to synchronize three distinct process types:

*   **Master Process (`ChompChamps`):** The core engine. It manages a Round-Robin scheduler using `select()` multiplexing to enforce strict timeouts for each player. It oversees shared memory allocation and implements signal handlers for graceful resource teardown.
*   **Player Process / AI (`player`):** Autonomous entities that interact with the shared game state by solving the classic Reader-Writer synchronization problem using semaphores. 
*   **View Process (`view`):** Decoupled rendering engine that waits for synchronization signals to print the real-time matrix state.

## Explanation of the Winning AI Algorithm
Instead of blindly chasing immediate points, the AI utilizes a dual-heuristic approach to survive and dominate:
*   **Breadth-First Search (BFS):** Implements an algorithm to analyze the local board state and calculate escape routes.
*   **Weighted Decision Making:** The next move is calculated by maximizing `cellValue + SPACE_SCORE_MULTIPLIER * accessibleCells`, ensuring the AI never traps itself.

---

## Requirements

- **Docker installed.**
- **Docker Image** provided by the university:  
  `agodio/itba-so-multi-platform:3.0`
- Clone of this repository.
- PVS-Studio with a valid license (Optional).

## Environment Initilization

To start the controlled development environment, run the following Docker command:

```bash
docker run --rm -v ${PWD}:/root --security-opt seccomp:unconfined -it agodio/itba-so-multi-platform:3.0
```

Then, navigate to the working root directory:

```bash
cd root
```

---

## Compilation & Makefile Features

The project includes a `Makefile` to easily compile the system binaries.

**Compile all binaries:**
```bash
make all
```

This command generates:
- `ChompChamps`
- `player`
- `view` 

They can also be compiled individually, by doing `make ChompChamps`, `make view` or `make player`.

**Clean generated binaries:**
  ```bash
  make clean
  ```
**Clean temporary files, analyze with PVS-Studio and generate a report**
  ```bash
  make pvs
  ```
To execute this command is requiered to have PVS-STUDIO installed and have a valid license. The HTML report shall appear in `informe_completo.html`.
 
---

## Execution

The main binary `ChompChamps` accepts multiple parameters to configure the match:

```bash
./ChompChamps -p player [player2 ... playerN] [-v view] [-w WIDTH] [-h HEIGHT] [-d DELAY] [-t TIMEOUT] [-s SEED]
```

### Parameters Configuration

●​[-p]: Path(s) to the player binaries. Minimum: 1, Maximum: 9.

●​[-w]: Board width. Default and minimum: 10.

●​[-h]: Board height. Default and minimum: 10.

●​[-d]: Delay (in milliseconds) the master waits after each state print. Default: 200.

●​[-t]: Timeout (in seconds) to receive valid move requests. Default: 10.

●​[-s]: Seed used for board generation. Default: time(NULL).

●​[-v]: Path to the view binary. Default: No view.

### All parameters are optional except -p.
