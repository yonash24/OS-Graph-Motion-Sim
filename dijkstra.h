#ifndef DIJKSTRA_H
#define DIJKSTRA_H

/*
 * [M1] מבני גרף + Dijkstra
 * [M4+] Traveler, loadGraphAndTravelers — קובץ עם # travelers
 */

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

typedef struct {
    int startNode;
    int endNode;
    int priority;   /* [M7] עדיפות (נמוך = גבוה יותר); ברירת מחדל 0 */
} Traveler;



int loadGraph(const char* filename, Graph* graph, int* startNode, int* endNode);
int loadGraphAndTravelers(const char* filename, Graph* graph, Traveler** outTravelers, int* outNumTravelers);
void runDijkstra(Graph* graph, int startNode, int endNode, int* outPath, int* outLen);
void freeGraph(Graph* graph);


#endif //DIJKSTRA_H
