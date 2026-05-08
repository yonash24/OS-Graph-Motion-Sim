
#include "main.h"




int main(int argc, char* argv[])
{
    if (argc < 2) {
        printf("Usage: %s <file_name>\n", argv[0]);
        return 1;
    }
    const char* filename = argv[1];

    Graph graph;
    int startNode, endNode;

    if (!loadGraph(filename, &graph, &startNode, &endNode)) {
        return 1;
    }

    int* outPath = (int*)malloc(graph.numNodes * sizeof(int));
    int outLen = 0;

    if (outPath == NULL) {
        freeGraph(&graph);
        return 1;
    }

    runDijkstra(&graph, startNode, endNode, outPath, &outLen);

    visualizeGraph(&graph, outPath, outLen, startNode, endNode);

    free(outPath);
    freeGraph(&graph);

    return 0;
}


