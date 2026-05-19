# OS Graph Motion Sim

A C-based Linux project simulating concurrent process movement on a directed graph using Dijkstra's shortest-path algorithm and the `raylib` library.

## Build Commands

The project includes a `Makefile` with the following mandatory targets:

```bash
make milestone1    # Builds the ./dijkstra executable
make milestone2    # Builds the ./sim executable
make milestone3    # Builds the ./sim executable
make milestone4    # Builds the ./sim executable for Milestone 4
make milestone5    # Builds the ./sim executable for Milestone 5
make clean         # Removes all compiled binaries and object files
```

## Run Commands
Run the executables by providing the input file as an argument:
```bash
./dijkstra <file_name>           # Milestone 1
./sim <file_name>                # Milestones 2–6
./sim -schd fcfs <file_name>     # Milestone 7
./sim -schd sjf <file_name>      # Milestone 7
```

## Input File Format
The program reads graph data from a text file (e.g., data.txt):
- Line 1: N (nodes) and M (edges).
- M lines: source, destination, and weight for each edge.
- For milestones 4-5, the file also includes a `# travelers` section:
  - Line: number of travelers
  - Following lines: startNode and endNode for each traveler.

## Implementation Details

- **Milestone 1:** Shortest Path Calculation using Dijkstra's algorithm.
- **Milestone 2:** Static Graph Visualization using raylib.
- **Milestone 3:** Animation and Motion Logic (Movement Speed: 300ms per edge weight unit, Intermediate Stops: 1 second).
- **Milestone 4:** Multiple Processes. The parent process calculates all paths, forks multiple child processes (one for each traveler), and manages the concurrent GUI display where each traveler has a unique color.
- **Milestone 5:** IPC Communication. The child processes are fully autonomous and calculate their own Dijkstra paths. They report their status to the parent process using IPC.

### IPC Method Selection (Milestone 5)
For inter-process communication between the parent and the children, we chose to use **Pipes (`pipe()`)**.
**Reasoning:**
1. **Unidirectional Flow:** The data flow is strictly from the child processes to the parent (reporting location and destination arrival), making a pipe the perfect one-way channel.
2. **Simplicity:** Pipes do not require complex synchronization mechanisms (like semaphores or mutexes) compared to Shared Memory (`shmget`).
3. **Non-blocking I/O:** By setting the read-end of the pipe to non-blocking mode (`O_NONBLOCK`), the parent process can continuously run the Raylib GUI loop at 60 FPS without hanging while waiting for updates from the children.

