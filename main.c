#define _DEFAULT_SOURCE
#include "main.h"
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

/* Default to Milestone 5 if not specified via -DMILESTONE=N */
#ifndef MILESTONE
#define MILESTONE 5
#endif

/* ═══════════════════════════════════════════════════════════
   Milestone 5 only: child process runs Dijkstra autonomously
   and reports each step to the parent via pipe.
   ═══════════════════════════════════════════════════════════ */
#if MILESTONE == 5
static void childProcess(Graph* graph, int startNode, int endNode,
                         int pipe_write_fd) {
    int* outPath = (int*)malloc(graph->numNodes * sizeof(int));
    int  outLen  = 0;
    runDijkstra(graph, startNode, endNode, outPath, &outLen);

    if (outLen == 0) {
        /* No path: notify parent immediately */
        IPC_Message msg = { getpid(), startNode, -1, 1 };
        write(pipe_write_fd, &msg, sizeof(msg));
        free(outPath);
        exit(0);
    }

    /* Walk the path, sending one message per edge */
    for (int i = 0; i < outLen - 1; i++) {
        int curr   = outPath[i];
        int next   = outPath[i + 1];
        int weight = findEdgeWeight(graph, curr, next);

        IPC_Message msg = { getpid(), curr, next, 0 };
        write(pipe_write_fd, &msg, sizeof(msg));

        /* Sleep proportional to edge weight (same timing as animation) */
        usleep((weight * 300 * 1000) + 1000000);
    }

    /* Final message: arrived at destination */
    IPC_Message msg = { getpid(), endNode, -1, 1 };
    write(pipe_write_fd, &msg, sizeof(msg));

    free(outPath);
    exit(0);
}
#endif /* MILESTONE == 5 */

/* ═══════════════════════════════════════════════════════════
   main
   ═══════════════════════════════════════════════════════════ */
#if MILESTONE <= 3
int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <file_name>\n", argv[0]);
        return 1;
    }

    Graph graph;
    int startNode = 0;
    int endNode = 0;

    if (!loadGraph(argv[1], &graph, &startNode, &endNode)) {
        printf("Failed to load graph.\n");
        return 1;
    }

    int* path = (int*)malloc(graph.numNodes * sizeof(int));
    int pathLen = 0;
    runDijkstra(&graph, startNode, endNode, path, &pathLen);

#if MILESTONE >= 2
    visualizeGraph(&graph, path, pathLen, startNode, endNode);
#endif

    free(path);
    freeGraph(&graph);
    return 0;
}
#else /* MILESTONE >= 4 */
int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <file_name>\n", argv[0]);
        return 1;
    }

    Graph    graph;
    Traveler* travelers   = NULL;
    int       numTravelers = 0;

    if (!loadGraphAndTravelers(argv[1], &graph, &travelers, &numTravelers)) {
        printf("Failed to load graph and travelers.\n");
        return 1;
    }

    Color colors[] = { RED, GREEN, BLUE, ORANGE, PURPLE,
                       PINK, BROWN, LIME, VIOLET, DARKGREEN };
    int numColors = 10;

    TravelerState* states =
        (TravelerState*)calloc(numTravelers, sizeof(TravelerState));

/* ─────────────────────────────────────────────────────────
   MILESTONE 4
   Parent computes all paths, forks children that just sleep.
   ───────────────────────────────────────────────────────── */
#if MILESTONE == 4

    for (int i = 0; i < numTravelers; i++) {
        /* Parent pre-computes the path for child i */
        int* path    = (int*)malloc(graph.numNodes * sizeof(int));
        int  pathLen = 0;
        runDijkstra(&graph, travelers[i].startNode, travelers[i].endNode,
                    path, &pathLen);

        pid_t pid = fork();
        if (pid == -1) { perror("fork"); return 1; }

        if (pid == 0) {
            /* Child: announce existence, then sleep until signalled */
            printf("[%d] started\n", getpid());
            fflush(stdout);
            while (1) pause();   /* waits for SIGTERM from parent */
            exit(0);
        }

        /* Parent: store state */
        states[i].pid         = pid;
        states[i].isActive    = 0;
        states[i].isFinished  = 0;
        states[i].color       = colors[i % numColors];
        states[i].animState   = ANIM_IDLE;
        states[i].currentNode = travelers[i].startNode;
        states[i].nextNode    = travelers[i].startNode;
        states[i].path        = path;
        states[i].pathLen     = pathLen;
        states[i].edgeIdx     = 0;
    }

    /* Run GUI (pipe_fd == -1 → parent-driven animation) */
    visualizeMultiTravelers(&graph, states, numTravelers, -1);

    /* After GUI closes: terminate children, collect exit status */
    for (int i = 0; i < numTravelers; i++) {
        kill(states[i].pid, SIGTERM);
        int status;
        pid_t done = waitpid(states[i].pid, &status, 0);
        if (done > 0) printf("[PID=%d] finished\n", done);
        free(states[i].path);
    }

/* ─────────────────────────────────────────────────────────
   MILESTONE 5
   Children are autonomous: each computes its own path and
   reports progress to the parent via a shared pipe.
   ───────────────────────────────────────────────────────── */
#else  /* MILESTONE == 5 */

    int pipefd[2];
    if (pipe(pipefd) == -1) { perror("pipe"); return 1; }

    for (int i = 0; i < numTravelers; i++) {
        pid_t pid = fork();
        if (pid == -1) { perror("fork"); return 1; }

        if (pid == 0) {
            close(pipefd[0]);   /* child only writes */
            childProcess(&graph, travelers[i].startNode,
                         travelers[i].endNode, pipefd[1]);
        }

        states[i].pid         = pid;
        states[i].isActive    = 0;
        states[i].isFinished  = 0;
        states[i].color       = colors[i % numColors];
        states[i].animState   = ANIM_IDLE;
        states[i].currentNode = travelers[i].startNode;
        states[i].nextNode    = travelers[i].startNode;
        states[i].path        = NULL;
        states[i].pathLen     = 0;
        states[i].edgeIdx     = 0;
    }

    close(pipefd[1]);   /* parent only reads */

    /* Make read-end non-blocking so GUI loop is never blocked */
    int flags = fcntl(pipefd[0], F_GETFL, 0);
    fcntl(pipefd[0], F_SETFL, flags | O_NONBLOCK);

    /* Run GUI (pipe_fd >= 0 → IPC-driven animation) */
    visualizeMultiTravelers(&graph, states, numTravelers, pipefd[0]);

    /* Collect children; parent prints "finished" line */
    for (int i = 0; i < numTravelers; i++) {
        int status;
        pid_t done = wait(&status);
        if (done > 0) printf("[PID=%d] finished\n", done);
    }

#endif /* MILESTONE */

    free(states);
    free(travelers);
    freeGraph(&graph);
    return 0;
}
#endif /* MILESTONE */
