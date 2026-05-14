#include "main.h"

int loadGraphExtended(const char* filename, Graph* graph, Traveler** travelersOut, int* numTravelersOut) {
    FILE *file = fopen(filename, "r");
    if (!file) return 0;

    int N, M;
    if (fscanf(file, "%d %d", &N, &M) != 2) { fclose(file); return 0; }
    
    graph->numNodes = N;
    graph->nodes = (Node*)malloc(N * sizeof(Node));
    for (int i = 0; i < N; i++) {
        graph->nodes[i].id = i;
        graph->nodes[i].edgeList = NULL;
    }

    for (int i = 0; i < M; i++) {
        int src, dst, w;
        fscanf(file, "%d %d %d", &src, &dst, &w);
        Edge* newEdge = (Edge*)malloc(sizeof(Edge));
        newEdge->dest = dst;
        newEdge->weight = w;
        newEdge->next = graph->nodes[src].edgeList;
        graph->nodes[src].edgeList = newEdge;
    }

    // קריאת מספר הנוסעים
    if (fscanf(file, "%d", numTravelersOut) != 1) { fclose(file); return 0; }
    
    *travelersOut = (Traveler*)malloc((*numTravelersOut) * sizeof(Traveler));
    Traveler* tArr = *travelersOut;
    
    Color palette[] = { RED, BLUE, GREEN, ORANGE, PURPLE, MAROON, LIME, GOLD, DARKGREEN, SKYBLUE };

    for (int i = 0; i < *numTravelersOut; i++) {
        int src, dst;
        fscanf(file, "%d %d", &src, &dst);
        
        tArr[i].path = (int*)malloc(N * sizeof(int));
        runDijkstra(graph, src, dst, tArr[i].path, &tArr[i].pathLen);
        
        tArr[i].color = palette[i % 10];
        tArr[i].state = (tArr[i].pathLen > 1) ? ANIM_MOVING : ANIM_DONE;
        tArr[i].edgeIdx = 0;
        tArr[i].subJump = 0;
        tArr[i].timer = 0;
        tArr[i].arrived = (tArr[i].pathLen <= 1);
        tArr[i].currentPos = (Vector2){0,0};
    }

    fclose(file);
    return 1;
}

void runDijkstra(Graph* graph, int startNode, int endNode, int* outPath, int* outLen) {
    int n = graph->numNodes;
    int dist[n], prev[n], visited[n];
    for (int i = 0; i < n; i++) { dist[i] = INT_MAX; prev[i] = -1; visited[i] = 0; }

    if (startNode == endNode) { outPath[0] = startNode; *outLen = 1; return; }

    dist[startNode] = 0;
    for (int count = 0; count < n; count++) {
        int u = -1, min_dist = INT_MAX;
        for (int i = 0; i < n; i++) {
            if (!visited[i] && dist[i] < min_dist) { min_dist = dist[i]; u = i; }
        }
        if (u == -1 || dist[u] == INT_MAX) break;
        visited[u] = 1;
        if (u == endNode) break;

        Edge* e = graph->nodes[u].edgeList;
        while (e) {
            if (!visited[e->dest] && dist[u] + e->weight < dist[e->dest]) {
                dist[e->dest] = dist[u] + e->weight;
                prev[e->dest] = u;
            }
            e = e->next;
        }
    }

    if (dist[endNode] == INT_MAX) { *outLen = 0; return; }
    int temp[n], count = 0, curr = endNode;
    while (curr != -1) { temp[count++] = curr; curr = prev[curr]; }
    *outLen = count;
    for (int i = 0; i < count; i++) outPath[i] = temp[count - 1 - i];
}

int findEdgeWeight(Graph* g, int from, int to) {
    Edge* e = g->nodes[from].edgeList;
    while (e) {
        if (e->dest == to) return e->weight;
        e = e->next;
    }
    return 1;
}

void freeGraph(Graph* graph) {
    for (int i = 0; i < graph->numNodes; i++) {
        Edge* curr = graph->nodes[i].edgeList;
        while (curr) {
            Edge* temp = curr;
            curr = curr->next;
            free(temp);
        }
    }
    free(graph->nodes);
}