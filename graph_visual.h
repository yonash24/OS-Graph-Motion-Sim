#ifndef GRAPH_VISUAL_H
#define GRAPH_VISUAL_H


typedef struct Edge Edge;
typedef struct Node Node;
typedef struct Graph Graph;


void visualizeGraph(void* graph, int* path, int pathLen, int startNode, int endNode);

#endif
