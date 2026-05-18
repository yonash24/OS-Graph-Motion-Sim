#include "dijkstra.h"







int loadGraph(const char* filename, Graph* graph, int* startNode, int* endNode) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error: Could not open file %s.\n", filename);
        return 0;
    }

    int N, M;
    if (fscanf(file, "%d %d", &N, &M) != 2) {
        printf("Error: Could not read N and M.\n");
        fclose(file);
        return 0;
    }

    // אתחול מבנה הגרף
    graph->numNodes = N;
    graph->numEdges = M;
    graph->nodes = (Node*)malloc(N * sizeof(Node));
    if (graph->nodes == NULL) {
        fclose(file);
        return 0;
    }

    for (int i = 0; i < N; i++) {
        graph->nodes[i].id = i;
        graph->nodes[i].edgeList = NULL;
    }

    // קריאת הקשתות
    int src, dest, weight;
    for (int i = 0; i < M; i++) {
        if (fscanf(file, "%d %d %d", &src, &dest, &weight) != 3) {
            printf("Error: Invalid edge data at line %d.\n", i + 2);
            freeGraph(graph);
            fclose(file);
            return 0;
        }

        if (weight < 0) {
            printf("Error: Negative weight is not allowed.\n");
            freeGraph(graph);
            fclose(file);
            return 0;
        }

        Edge* newEdge = (Edge*)malloc(sizeof(Edge));
        if (!newEdge) return 0;

        newEdge->dest = dest;
        newEdge->weight = weight;
        newEdge->next = graph->nodes[src].edgeList;
        graph->nodes[src].edgeList = newEdge;
    }

    // קריאת נקודות ההתחלה והסיום בסוף הקובץ
    if (fscanf(file, "%d %d", startNode, endNode) != 2) {
        printf("Error: Could not read start and end nodes.\n");
        freeGraph(graph);
        fclose(file);
        return 0;
    }

    fclose(file);
    return 1; // הצלחה
}

int loadGraphAndTravelers(const char* filename, Graph* graph, Traveler** outTravelers, int* outNumTravelers) {
    FILE *file = fopen(filename, "r");
    if (!file) return 0;

    char line[256];
    int N = -1, M = -1;

    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        if (sscanf(line, "%d %d", &N, &M) == 2) break;
    }
    if (N == -1 || M == -1) { fclose(file); return 0; }

    graph->numNodes = N;
    graph->numEdges = M;
    graph->nodes = (Node*)malloc(N * sizeof(Node));
    for (int i = 0; i < N; i++) {
        graph->nodes[i].id = i;
        graph->nodes[i].edgeList = NULL;
    }

    int edgesRead = 0;
    while (edgesRead < M && fgets(line, sizeof(line), file)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        int src, dest, weight;
        if (sscanf(line, "%d %d %d", &src, &dest, &weight) == 3) {
            Edge* newEdge = (Edge*)malloc(sizeof(Edge));
            newEdge->dest = dest;
            newEdge->weight = weight;
            newEdge->next = graph->nodes[src].edgeList;
            graph->nodes[src].edgeList = newEdge;
            edgesRead++;
        }
    }

    int numTravelers = -1;
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        if (sscanf(line, "%d", &numTravelers) == 1) break;
    }

    if (numTravelers <= 0) {
        freeGraph(graph);
        fclose(file);
        return 0; 
    }

    *outNumTravelers = numTravelers;
    *outTravelers = (Traveler*)malloc(numTravelers * sizeof(Traveler));

    int travRead = 0;
    while (travRead < numTravelers && fgets(line, sizeof(line), file)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        int start, end;
        if (sscanf(line, "%d %d", &start, &end) == 2) {
            (*outTravelers)[travRead].startNode = start;
            (*outTravelers)[travRead].endNode = end;
            travRead++;
        }
    }

    fclose(file);
    return 1;
}



void runDijkstra(Graph* graph, int startNode, int endNode, int* outPath, int* outLen) {
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
        outPath[0] = startNode;
        *outLen = 1;
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
        *outLen = 0;
        return;
    }

    // Path Reconstruction using the 'prev' array (built in reverse: end -> start)
    int revPath[n];
    int revCount = 0;
    int curr = endNode;

    while (curr != -1) {
        revPath[revCount++] = curr;
        curr = prev[curr];
    }

    // Store path in forward order (start -> end) for the animation
    *outLen = revCount;
    for (int i = 0; i < revCount; i++)
        outPath[i] = revPath[revCount - 1 - i];

    // Print path (forward order)
    for (int i = revCount - 1; i >= 0; i--) {
        printf("%d", revPath[i]);
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