#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "raylib.h"

#define JUMP_DURATION 0.3f
#define NODE_PAUSE 1.0f

typedef enum { ANIM_IDLE, ANIM_MOVING, ANIM_PAUSING, ANIM_DONE } AnimState;

typedef struct Edge {
    int dest;
    int weight;
    struct Edge* next;
} Edge;

typedef struct {
    int id;
    Edge* edgeList;
} Node;

typedef struct {
    int numNodes;
    int numEdges;
    Node* nodes;
} Graph;

typedef struct {
    pid_t pid;
    int* path;
    int pathLen;
    int edgeIdx;
    int subJump;
    float timer;
    Vector2 currentPos;
    Color color;
    AnimState state;
    bool arrived;
} Traveler;

int loadGraphExtended(const char* filename, Graph* graph, Traveler** travelers, int* numTravelers);
void runDijkstra(Graph* graph, int startNode, int endNode, int* outPath, int* outLen);
void visualizeGraph(Graph* graph, Traveler* travelers, int numTravelers);
int findEdgeWeight(Graph* g, int from, int to);
void freeGraph(Graph* graph);
Vector2 lerpV2(Vector2 a, Vector2 b, float t);

#endif
