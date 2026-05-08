//
// Created by gnsyt on 08/05/2026.
//

#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

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


int loadGraph(const char* filename, Graph* graph, int* startNode, int* endNode);
void runDijkstra(Graph* graph, int startNode, int endNode, int* outPath, int* outLen);
void freeGraph(Graph* graph);


#endif //DIJKSTRA_H
