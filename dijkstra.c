#include <stdio.h>
#include <limits.h>
#include <stdlib.h>

typedef struct Edge {
    int dest;
    int weight;
    struct Edge* next; //other edge that go out from same node
} Edge;

typedef struct Node {
    int id;
    Edge* edgeList; //pointer to first edge
} Node;

typedef struct Graph {
    Node* nodes;
    int numNodes;
    int numEdges;
} Graph;

void runDijkstra(Graph*, int, int);
void freeGraph(Graph* graph);

int main(int argc, char* argv[])
{
    const char* filename = (argc > 1) ? argv[1] : "data.txt";

    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error: Could not open file.\n");
        return 1;
    }

    int N, M;
    if (fscanf(file, "%d %d", &N, &M) != 2) {
        printf("Error: not found N and M\n");
        fclose(file);
        return 1;
    }

    Graph graph; //init
    graph.numNodes = N;
    graph.numEdges = M;
    graph.nodes = (Node*)malloc(N * sizeof(Node));

    for (int i = 0; i < N; i++) {
        graph.nodes[i].id = i; //all vertex of the graph
        graph.nodes[i].edgeList = NULL;
    }

    int src, dest, weight;
    for (int i = 0; i < M; i++) {
        if (fscanf(file, "%d %d %d", &src, &dest, &weight) != 3) {
            printf("Error: file not valid\n");
            freeGraph(&graph);
            fclose(file);
            return 1;
        }

        if (weight < 0) {
            printf("Error: negative weight is not allowed\n");
            freeGraph(&graph);
            fclose(file);
            return 1;
        }

        Edge* newEdge = (Edge*)malloc(sizeof(Edge));
        newEdge->dest = dest;
        newEdge->weight = weight;

        newEdge->next = graph.nodes[src].edgeList; //add to array of edges that go out from same vertex
        graph.nodes[src].edgeList = newEdge; //update head of array
    }

    int startNode, endNode;
    if (fscanf(file, "%d %d", &startNode, &endNode) != 2) {
        printf("Error: file not valid\n");
        freeGraph(&graph);
        fclose(file);
        return 1;
    }

    fclose(file);

    runDijkstra(&graph, startNode, endNode);

    freeGraph(&graph);
    return 0;
}


void runDijkstra(Graph* graph, int startNode, int endNode) {
    int n = graph->numNodes;
    int dist[n];     // Minimum distance from source to each node
    int prev[n];     // Parent node in the shortest path
    int visited[n];  // Boolean array to mark finalized nodes

    //init
    for (int i = 0; i < n; i++) {
        dist[i] = INT_MAX;
        prev[i] = -1;
        visited[i] = 0;
    }

    // Special case: Source is the same as destination
    if (startNode == endNode) {
        printf("0\n0\n");
        return;
    }

    dist[startNode] = 0;

    // Main Loop
    for (int count = 0; count < n; count++) {

        // Find the node with the minimum distance that hasn't been visited yet
        int u = -1;
        int min_dist = INT_MAX;

        for (int i = 0; i < n; i++) {
            if (!visited[i] && dist[i] < min_dist) {
                min_dist = dist[i];
                u = i;
            }
        }

        // Break if no reachable nodes remain or if graph is disconnected
        if (u == -1 || dist[u] == INT_MAX) break;

        visited[u] = 1;

        // Early exit if we reached the target node
        if (u == endNode) break;

        // Relaxation: Check all outgoing edges from the current node u
        Edge* edge = graph->nodes[u].edgeList;
        while (edge != NULL) {
            int v = edge->dest;
            int w = edge->weight;

            // If a shorter path to v is found via u
            if (!visited[v] && dist[u] + w < dist[v]) {
                dist[v] = dist[u] + w;
                prev[v] = u;
            }
            edge = edge->next; // Move to the next edge in the adjacency list
        }
    }

    //output Results

    //if no path found
    if (dist[endNode] == INT_MAX) {
        printf("No path found\n");
        return;
    }

    // Path Reconstruction using the 'prev' array
    int path[n];
    int pathCount = 0;
    int curr = endNode;

    while (curr != -1) {
        path[pathCount++] = curr;
        curr = prev[curr];
    }

    // Print path
    for (int i = pathCount - 1; i >= 0; i--) {
        printf("%d", path[i]);
        if (i > 0) printf(" -> ");
    }
    printf("\n");

    // Print the total weight
    printf("%d\n", dist[endNode]);
}


void freeGraph(Graph* graph) {
    for (int i = 0; i < graph->numNodes; i++) {
        Edge* curr = graph->nodes[i].edgeList;
        while (curr != NULL) {
            Edge* temp = curr;
            curr = curr->next;
            free(temp);
        }
    }
    free(graph->nodes);
}
