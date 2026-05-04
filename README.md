# OS Graph Motion Sim

Simulation of motion on a weighted directed graph using Dijkstra's shortest-path algorithm.

## Build commands

```bash
make milestone1   # builds ./dijkstra
make milestone2   # builds ./sim
make milestone3   # builds ./sim
make clean        # removes compiled binaries
```

## Run commands

```bash
./dijkstra <file_name>          # Milestone 1
./sim <file_name>               # Milestones 2–6
./sim -schd fcfs <file_name>    # Milestone 7
./sim -schd sjf  <file_name>    # Milestone 7
```

## Input file format

```
<N> <M>
<src> <dest> <weight>
...
<startNode> <endNode>
```

- `N` — number of nodes, `M` — number of edges
- Each edge line: source node, destination node, weight (non-negative)
- Last line: source and destination for Dijkstra

Example: `data.txt`

## Milestones

| Milestone | Description |
|-----------|-------------|
| 1 | Dijkstra's algorithm — prints shortest path and total distance to stdout |
| 2 | Graph visualisation with raylib — nodes, edges, weights displayed |
| 3 | Animation — entity moves along shortest path; PLAY/STOP button; speed proportional to edge weight (each jump ≤ 300 ms); "Destination Reached" message on arrival |
