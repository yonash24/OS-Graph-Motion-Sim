# OS Graph Motion Sim

A C-based Linux project simulating concurrent process movement on a directed graph using Dijkstra's shortest-path algorithm and the `raylib` library.

## Build Commands

The project includes a `Makefile` with the following mandatory targets:

```bash
make milestone1    # Builds the ./dijkstra executable
make milestone2    # Builds the ./sim executable
make milestone3    # Builds the ./sim executable
make clean         # Removes all compiled binaries and object files

Run CommandsRun
the executables by providing the input file as an argument:
Bash.
/dijkstra <file_name>           # Milestone 1
./sim <file_name>               # Milestones 2–6
./sim -schd fcfs <file_name>    # Milestone 7
./sim -schd sjf <file_name>     # Milestone 7

Input File Format:
The program reads graph data from a text file (e.g., data.txt):
Line 1: N (nodes) and M (edges).
M lines: source, destination, and weight for each edge.
Final line: startNode and endNode for the path simulation.

Implementation Details:
Milestone 1: Shortest Path CalculationImplemented Dijkstra's algorithm to find the most efficient route. The program outputs the sequence of nodes and the total path distance to the standard output.
Milestone 2: Static Graph VisualizationDeveloped a graphical interface using raylib. It renders all nodes as circles, directed edges as arrows, and displays their respective weights on the screen.
Milestone 3: Animation and Motion LogicAdded dynamic movement to the simulation:Movement Speed: For an edge with weight $W$, the entity performs $W$ discrete jumps. Each jump takes exactly 300ms.
Intermediate Stops: The entity waits for 1 second at each node along the path before moving to the next edge.User Interface: A PLAY/STOP button allows the user to pause and resume the animation.
Completion: A "Destination Reached" notification is displayed when the entity arrives at the target node.
