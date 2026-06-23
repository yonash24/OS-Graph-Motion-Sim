#define _DEFAULT_SOURCE
#include "main.h"
#include "sched.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>

void handle_sigusr1(int sig) {
    printf("[PID %d] Received SIGUSR1 instead of death, sleeping...\n", getpid());
    fflush(stdout);
    sleep(2);
    exit(0);
}

/* Default to Milestone 6 if not specified via -DMILESTONE=N */
#ifndef MILESTONE
#define MILESTONE 6
#endif

/* ═══════════════════════════════════════════════════════════
   [M7] parseM7Args — parsing: ./sim -schd fcfs|sjf <file>
   ═══════════════════════════════════════════════════════════ */
#if MILESTONE == 7
static int parseM7Args(int argc, char** argv, SchedPolicy* policy, const char** file) {
    if (argc != 4) return 0;
    if (strcmp(argv[1], "-schd") != 0) return 0;
    if (strcmp(argv[2], "fcfs") == 0) *policy = SCHED_FCFS;
    else if (strcmp(argv[2], "sjf") == 0) *policy = SCHED_SJF;
    else return 0;
    *file = argv[3];
    return 1;
}
#endif

/* ═══════════════════════════════════════════════════════════
   [M5-M7] childComputePath — Dijkstra אוטונומי בילד
   ═══════════════════════════════════════════════════════════ */
#if MILESTONE == 5 || MILESTONE == 6 || MILESTONE == 7

static int* childComputePath(Graph* graph, int startNode, int endNode, int* outLen) {
    int* outPath = (int*)malloc(graph->numNodes * sizeof(int));
    runDijkstra(graph, startNode, endNode, outPath, outLen);
    return outPath;
}

/* ── [M5] childProcess — IPC בלבד, בלי סנכרון צמתים ── */
#if MILESTONE == 5
static void childProcess(Graph* graph, int startNode, int endNode,
                         int pipe_write_fd) {
    int outLen = 0;
    int* outPath = childComputePath(graph, startNode, endNode, &outLen);

    if (outLen == 0) {
        IPC_Message msg = { getpid(), startNode, -1, STATUS_ARRIVED_DEST, 0, 0 };
        write(pipe_write_fd, &msg, sizeof(msg));
        free(outPath);
        exit(0);
    }

    for (int i = 0; i < outLen; i++) {
        int curr = outPath[i];
        int next = (i < outLen - 1) ? outPath[i + 1] : -1;

        IPC_Message msg = { getpid(), curr, next,
                            (next == -1) ? STATUS_ARRIVED_DEST : STATUS_INSIDE_NODE, 0, 0 };
        write(pipe_write_fd, &msg, sizeof(msg));

        if (next != -1) {
            if (i > 0) sleep(1);
            IPC_Message msg_drive = { getpid(), curr, next, STATUS_DRIVING, 0, 0 };
            write(pipe_write_fd, &msg_drive, sizeof(msg_drive));
            usleep(5000);

            int weight = findEdgeWeight(graph, curr, next);
            usleep(weight * 300 * 1000);
        }
    }

    free(outPath);
    exit(0);
}
#endif /* [M5] */

/* ── [M6] childProcess — sem_wait/post על node_sems (mmap) ── */
#if MILESTONE == 6
static void childProcess(Graph* graph, int startNode, int endNode,
                         int pipe_write_fd, sem_t* node_sems) {
    int outLen = 0;
    int* outPath = childComputePath(graph, startNode, endNode, &outLen);

    if (outLen == 0) {
        IPC_Message msg = { getpid(), startNode, -1, STATUS_ARRIVED_DEST, 0, 0 };
        write(pipe_write_fd, &msg, sizeof(msg));
        free(outPath);
        exit(0);
    }

    for (int i = 0; i < outLen; i++) {
        int curr = outPath[i];
        int next = (i < outLen - 1) ? outPath[i + 1] : -1;

        if (i > 0) {
            int prev = outPath[i - 1];
            IPC_Message msg_wait = { getpid(), prev, curr, STATUS_WAITING_FOR_NODE, 0, 0 };
            write(pipe_write_fd, &msg_wait, sizeof(msg_wait));
            usleep(5000);
        }

        sem_t* sem = &node_sems[curr];
        sem_wait(sem);

        IPC_Message msg_inside = { getpid(), curr, next,
                                   (next == -1) ? STATUS_ARRIVED_DEST : STATUS_INSIDE_NODE, 0, 0 };
        write(pipe_write_fd, &msg_inside, sizeof(msg_inside));

        sleep(1);

        if (next != -1) {
            IPC_Message msg_drive = { getpid(), curr, next, STATUS_DRIVING, 0, 0 };
            write(pipe_write_fd, &msg_drive, sizeof(msg_drive));
            usleep(5000);
        }

        sem_post(sem);

        if (next != -1) {
            int weight = findEdgeWeight(graph, curr, next);
            usleep(weight * 300 * 1000);
        }
    }

    free(outPath);
    exit(0);
}
#endif /* [M6] */

/* ── [M7] childProcess — SCHEDULE_REQUEST + grant_sems ── */
#if MILESTONE == 7
static int remainingPathWeight(Graph* graph, int* path, int pathLen, int fromIdx) {
    int total = 0;
    for (int j = fromIdx; j < pathLen - 1; j++)
        total += findEdgeWeight(graph, path[j], path[j + 1]);
    return total;
}

static void childProcess(Graph* graph, int startNode, int endNode,
                         int pipe_write_fd, sem_t* grant_sems, int traveler_idx,
                         int priority) {
    int outLen = 0;
    int* outPath = childComputePath(graph, startNode, endNode, &outLen);

    if (outLen == 0) {
        IPC_Message msg = { getpid(), startNode, -1, STATUS_ARRIVED_DEST, 0, priority };
        write(pipe_write_fd, &msg, sizeof(msg));
        free(outPath);
        exit(0);
    }

    for (int i = 0; i < outLen; i++) {
        int curr = outPath[i];
        int next = (i < outLen - 1) ? outPath[i + 1] : -1;
        int remaining = remainingPathWeight(graph, outPath, outLen, i);

        if (i > 0) {
            int prev = outPath[i - 1];
            IPC_Message msg_wait = { getpid(), prev, curr, STATUS_WAITING_FOR_NODE, remaining, priority };
            write(pipe_write_fd, &msg_wait, sizeof(msg_wait));
            usleep(5000);
        }

        IPC_Message msg_req = { getpid(), (i > 0) ? outPath[i - 1] : curr, curr,
                                STATUS_SCHEDULE_REQUEST, remaining, priority };
        write(pipe_write_fd, &msg_req, sizeof(msg_req));

        sem_wait(&grant_sems[traveler_idx]);

        IPC_Message msg_inside = { getpid(), curr, next,
                                   (next == -1) ? STATUS_ARRIVED_DEST : STATUS_INSIDE_NODE,
                                   remaining, priority };
        write(pipe_write_fd, &msg_inside, sizeof(msg_inside));

        sleep(1);

        if (next != -1) {
            IPC_Message msg_drive = { getpid(), curr, next, STATUS_DRIVING, remaining, priority };
            write(pipe_write_fd, &msg_drive, sizeof(msg_drive));
            usleep(5000);

            int weight = findEdgeWeight(graph, curr, next);
            usleep(weight * 300 * 1000);
        }
    }

    free(outPath);
    exit(0);
}
#endif /* [M7] */

#endif /* [M5-M7] child processes */

/* ═══════════════════════════════════════════════════════════
   [M1-M3] main — Dijkstra + (M2+) GUI
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
    visualizeGraph(&graph, path, pathLen, startNode, endNode);  /* [M2-M3] */
#endif

    free(path);
    freeGraph(&graph);
    return 0;
}
#else /* [M4-M7] main — נוסעים מרובים + fork */
int main(int argc, char* argv[]) {
    const char* input_file = NULL;
#if MILESTONE == 7
    SchedPolicy policy = SCHED_FCFS;
    if (!parseM7Args(argc, argv, &policy, &input_file)) {       /* [M7] */
        fprintf(stderr, "Usage: %s -schd fcfs|sjf <file_name>\n", argv[0]);
        return 1;
    }
#else
    if (argc < 2) {
        printf("Usage: %s <file_name>\n", argv[0]);
        return 1;
    }
    input_file = argv[1];
#endif

    Graph    graph;
    Traveler* travelers   = NULL;
    int       numTravelers = 0;

    if (!loadGraphAndTravelers(input_file, &graph, &travelers, &numTravelers)) {
        printf("Failed to load graph and travelers.\n");
        return 1;
    }

    Color colors[] = { RED, GREEN, BLUE, ORANGE, PURPLE,
                       PINK, BROWN, LIME, VIOLET, DARKGREEN };
    int numColors = 10;

    TravelerState* states =
        (TravelerState*)calloc(numTravelers, sizeof(TravelerState));

/* ═══════════════════════════════════════════════════════════
   [M4] fork — האב מחשב מסלול, ילדים ב-pause()
   ═══════════════════════════════════════════════════════════ */
#if MILESTONE == 4
    for (int i = 0; i < numTravelers; i++) {
        int* path    = (int*)malloc(graph.numNodes * sizeof(int));
        int  pathLen = 0;
        runDijkstra(&graph, travelers[i].startNode, travelers[i].endNode,
                    path, &pathLen);

        pid_t pid = fork();
        if (pid == -1) { perror("fork"); return 1; }

        if (pid == 0) {
            signal(SIGUSR1, handle_sigusr1);
            printf("[%d] started\n", getpid());
            fflush(stdout);
            while (1) pause();
            exit(0);
        }

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

    visualizeMultiTravelers(&graph, states, numTravelers, -1, NULL);

    for (int i = 0; i < numTravelers; i++) {
        printf("Parent sending SIGUSR1 to child PID %d...\n", states[i].pid);
        kill(states[i].pid, SIGUSR1);
        int status;
        pid_t done = waitpid(states[i].pid, &status, 0);
        if (done > 0) printf("[PID=%d] finished\n", done);
        free(states[i].path);
    }

/* ═══════════════════════════════════════════════════════════
   [M5-M7] pipe + fork + (M6) node_sems / (M7) grant_sems
   ═══════════════════════════════════════════════════════════ */
#else  /* MILESTONE == 5 || 6 || 7 */

#if MILESTONE == 6
    /* [M6] סמפורים משותפים — מנעול לכל צומת */
    sem_t* node_sems = (sem_t*)mmap(NULL, graph.numNodes * sizeof(sem_t),
                             PROT_READ | PROT_WRITE,
                             MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if (node_sems == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }

    for (int i = 0; i < graph.numNodes; i++) {
        if (sem_init(&node_sems[i], 1, 1) == -1) {
            perror("sem_init failed");
            return 1;
        }
    }
#endif

#if MILESTONE == 7
    /* [M7] סמפור grant לכל נוסע + Scheduler */
    sem_t* grant_sems = (sem_t*)mmap(NULL, (size_t)numTravelers * sizeof(sem_t),
                                     PROT_READ | PROT_WRITE,
                                     MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (grant_sems == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }
    for (int i = 0; i < numTravelers; i++) {
        if (sem_init(&grant_sems[i], 1, 0) == -1) {
            perror("sem_init failed");
            return 1;
        }
    }

    Scheduler sched;
    scheduler_init(&sched, policy, graph.numNodes, grant_sems);
#endif

    int pipefd[2];
    if (pipe(pipefd) == -1) { perror("pipe"); return 1; }  /* [M5-M7] IPC */

    for (int i = 0; i < numTravelers; i++) {
        pid_t pid = fork();                                 /* [M4-M7] */
        if (pid == -1) { perror("fork"); return 1; }

        if (pid == 0) {
            close(pipefd[0]);   /* child only writes */
#if MILESTONE == 5
            childProcess(&graph, travelers[i].startNode,
                         travelers[i].endNode, pipefd[1]);
#elif MILESTONE == 6
            childProcess(&graph, travelers[i].startNode,
                         travelers[i].endNode, pipefd[1], node_sems);
#else
            childProcess(&graph, travelers[i].startNode,
                         travelers[i].endNode, pipefd[1], grant_sems, i,
                         travelers[i].priority);
#endif
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

    int flags = fcntl(pipefd[0], F_GETFL, 0);
    fcntl(pipefd[0], F_SETFL, flags | O_NONBLOCK);           /* [M5-M7] */

#if MILESTONE == 7
    visualizeMultiTravelers(&graph, states, numTravelers, pipefd[0], &sched);
#else
    visualizeMultiTravelers(&graph, states, numTravelers, pipefd[0], NULL);
#endif

    for (int i = 0; i < numTravelers; i++) {
        int status;
        pid_t done = waitpid(states[i].pid, &status, 0);  /* [M4-M7] */
        if (done > 0) printf("[PID=%d] finished\n", done);
    }

#if MILESTONE == 6
    for (int i = 0; i < graph.numNodes; i++) {
        sem_destroy(&node_sems[i]);
    }
    munmap(node_sems, graph.numNodes * sizeof(sem_t));
#endif

#if MILESTONE == 7
    for (int i = 0; i < numTravelers; i++) {
        sem_destroy(&grant_sems[i]);
    }
    munmap(grant_sems, (size_t)numTravelers * sizeof(sem_t));
#endif

#endif /* [M4] vs [M5-M7] */

    free(states);
    free(travelers);
    freeGraph(&graph);
    return 0;
}
#endif /* [M1-M3] vs [M4-M7] */