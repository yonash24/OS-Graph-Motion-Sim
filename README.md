OS Graph Motion Sim

A C-based Linux project simulating concurrent process movement on a directed graph using Dijkstra's shortest-path algorithm and the raylib library.

Build Commands

The project includes a Makefile with the following mandatory targets:

make milestone1    # Builds the ./dijkstra executable
make milestone2    # Builds the ./sim executable
make milestone3    # Builds the ./sim executable
make milestone4    # Builds the ./sim executable for Milestone 4
make milestone5    # Builds the ./sim executable for Milestone 5
make milestone6    # Builds the ./sim executable for Milestone 6
make milestone7    # Builds the ./sim executable for Milestone 7
make clean         # Removes all compiled binaries and object files

Run Commands

Run the executables by providing the input file as an argument:

./dijkstra <file_name>           # Milestone 1
./sim <file_name>                # Milestones 2–6
./sim -schd fcfs data_m5.txt     # Milestone 7 – FCFS demo
./sim -schd sjf  data_m7.txt     # Milestone 7 – SJF demo

Input File Format

The program reads graph data from a text file (e.g., data.txt):





Line 1: N (nodes) and M (edges).



M lines: source, destination, and weight for each edge.



For milestones 4-5, the file also includes a # travelers section:





Line: number of travelers



Following lines: startNode and endNode for each traveler.

Implementation Details





Milestone 1: Shortest Path Calculation using Dijkstra's algorithm.



Milestone 2: Static Graph Visualization using raylib.



Milestone 3: Animation and Motion Logic (Movement Speed: 300ms per edge weight unit, Intermediate Stops: 1 second).



Milestone 4: Multiple Processes. The parent process calculates all paths, forks multiple child processes (one for each traveler), and manages the concurrent GUI display where each traveler has a unique color.



Milestone 5: IPC Communication. The child processes are fully autonomous and calculate their own Dijkstra paths. They report their status to the parent process using IPC.



Milestone 6: Node Synchronization (Critical Sections). Ensures mutual exclusion inside nodes. When multiple travelers reach the same node simultaneously, only one may enter, while the others wait outside.



Milestone 7: Node Entry Scheduling (FCFS / SJF). The parent maintains a waiting queue per node and grants entry according to the scheduler selected on the command line.

Node Entry Scheduling (Milestone 7)

When several travelers wait outside the same node, the parent process manages a per-node queue and picks the next traveler to enter:







Algorithm



Selection rule





FCFS



First traveler that requested the node





SJF



Traveler with the smallest remaining path weight (sum of edge weights left to destination)

The choice is made at runtime:

./sim -schd fcfs data_m5.txt   # demo: FCFS (staggered arrival at node 3)
./sim -schd sjf  data_m7.txt   # demo: SJF (different remaining path weights)

How it works: each child sends STATUS_SCHEDULE_REQUEST and blocks on a personal grant semaphore. The parent enqueues the request and calls sem_post only for the traveler selected by FCFS or SJF when the node is free.

Demo input files:







File



Scheduler



Why





data_m5.txt



FCFS



Different edge weights to node 3 → travelers arrive at different times (2→3 weight 2, 4→3 weight 5, 0→3 weight 8). FCFS entry order at node 3: 2 → 4 → 0.





data_m7.txt



SJF



Same arrival time (all weight 2 to node 3), but different remaining weights after node 3. SJF entry order: 2→4 (remaining 2), 4→1 (4), 0→5 (8).

Compare the [Scheduler/...] granted node 3 lines in the terminal for each run.

IPC Method Selection (Milestone 5)

For inter-process communication between the parent and the children, we chose to use Pipes (pipe()).
Reasoning:





Unidirectional Flow: The data flow is strictly from the child processes to the parent (reporting location and destination arrival), making a pipe the perfect one-way channel.



Simplicity: Pipes do not require complex synchronization mechanisms (like semaphores or mutexes) compared to Shared Memory (shmget).



Non-blocking I/O: By setting the read-end of the pipe to non-blocking mode (O_NONBLOCK), the parent process can continuously run the Raylib GUI loop at 60 FPS without hanging while waiting for updates from the children.

Synchronization Mechanism Selection (Milestone 6)

To enforce node capacity constraints and solve race conditions between completely independent processes, we chose to implement Process-Shared Anonymous Semaphores combined with Shared Memory (mmap).
Reasoning:





True Cross-Process Mutex: Standard POSIX Mutexes are designed for threads within the same memory space. By storing POSIX anonymous semaphores (sem_t) inside an actual shared memory segment mapped via mmap(..., MAP_SHARED | MAP_ANONYMOUS, -1, 0), all independent child processes can access the exact same lock structure.



Built-in Kernel Queue Management: Each node's semaphore is initialized using sem_init(&sem, 1, 1). Setting the second parameter (pshared) to 1 explicitly instructs the OS kernel to manage an atomic, block-and-wait queue for separate process IDs (PIDs).



Deadlock & Race Avoidance: To prevent visual state desynchronization in the GUI, child processes post their STATUS_DRIVING message to the pipe before calling sem_post(). This guarantees that the parent registers a traveler's departure before the next waiting process unblocks and occupies the node.



Visual Feedback: When a traveler is blocked, the GUI dynamically calculates a push-back vector on the incoming edge, preventing overlapping animations and rendering an active flashing state with the serial traveler number and system PID: T_ (PID:____) WAIT.

