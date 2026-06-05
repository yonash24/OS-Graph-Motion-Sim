#define _DEFAULT_SOURCE
#include "main.h"
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>

/* Default to Milestone 6 if not specified via -DMILESTONE=N */
#ifndef MILESTONE
#define MILESTONE 6
#endif

/* ═══════════════════════════════════════════════════════════
   Milestone 5 & 6: child process runs Dijkstra autonomously
   ═══════════════════════════════════════════════════════════ */
#if MILESTONE == 5 || MILESTONE == 6
static void childProcess(Graph* graph, int startNode, int endNode,
                         int pipe_write_fd, sem_t* node_sems) {
    int* outPath = (int*)malloc(graph->numNodes * sizeof(int));
    int  outLen  = 0;
    runDijkstra(graph, startNode, endNode, outPath, &outLen);

    if (outLen == 0) {
        IPC_Message msg = { getpid(), startNode, -1, STATUS_ARRIVED_DEST };
        write(pipe_write_fd, &msg, sizeof(msg));
        free(outPath);
        exit(0);
    }

    /* Milestone 6 logic (With Correct Shared Memory Node Access Synchronization) */
    for (int i = 0; i < outLen; i++) {
        int curr = outPath[i];
        int next = (i < outLen - 1) ? outPath[i + 1] : -1;

        // 1. הגענו מחוץ לצומת curr - נדווח לאבא שאנחנו ממתינים בכניסה אליו!
        if (i > 0) {
            int prev = outPath[i - 1]; // הצומת שממנו הגענו פיזית ברגע זה

            // ═══════════════════════════════════════════════════════════
            // תיקון קריטי: הנוסע בא מ-prev ומחכה מחוץ ל-curr!
            // ═══════════════════════════════════════════════════════════
            IPC_Message msg_wait = { getpid(), prev, curr, STATUS_WAITING_FOR_NODE };
            write(pipe_write_fd, &msg_wait, sizeof(msg_wait));
            usleep(5000);
        }

        sem_t* sem = &node_sems[curr];

        // 2. חסימה: ממתינים מחוץ לצומת עד שיתפנה
        sem_wait(sem);

        // 3. עברנו את החסימה ונכנסנו לצומת בהצלחה!
        IPC_Message msg_inside = { getpid(), curr, next, (next == -1) ? STATUS_ARRIVED_DEST : STATUS_INSIDE_NODE };
        write(pipe_write_fd, &msg_inside, sizeof(msg_inside));

        // שהייה קריטית של 3 שניות בתוך הצומת
        sleep(1);

        // 4. מדווחים על יציאה לנסיעה *לפני* פתיחת המנעול למניעת מירוץ תהליכים
        if (next != -1) {
            IPC_Message msg_drive = { getpid(), curr, next, STATUS_DRIVING };
            write(pipe_write_fd, &msg_drive, sizeof(msg_drive));
            usleep(5000);
        }

        // 5. שחרור המנעול לבא בתור
        sem_post(sem);

        // 6. הנסיעה הפיזית על הקשת
        if (next != -1) {
            int weight = findEdgeWeight(graph, curr, next);
            usleep(weight * 300 * 1000);
        }
    }

    free(outPath);
    exit(0);
}
#endif /* MILESTONE 5 || 6 */

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
   ───────────────────────────────────────────────────────── */
#if MILESTONE == 4
    for (int i = 0; i < numTravelers; i++) {
        int* path    = (int*)malloc(graph.numNodes * sizeof(int));
        int  pathLen = 0;
        runDijkstra(&graph, travelers[i].startNode, travelers[i].endNode,
                    path, &pathLen);

        pid_t pid = fork();
        if (pid == -1) { perror("fork"); return 1; }

        if (pid == 0) {
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

    visualizeMultiTravelers(&graph, states, numTravelers, -1);

    for (int i = 0; i < numTravelers; i++) {
        kill(states[i].pid, SIGTERM);
        int status;
        pid_t done = waitpid(states[i].pid, &status, 0);
        if (done > 0) printf("[PID=%d] finished\n", done);
        free(states[i].path);
    }

/* ─────────────────────────────────────────────────────────
   MILESTONE 5 & 6
   ───────────────────────────────────────────────────────── */
#else  /* MILESTONE == 5 || 6 */

    sem_t* node_sems = NULL;

#if MILESTONE == 6
    // הקצאת זיכרון משותף אנונימי (Shared Memory) עבור מערך הסמפורים של הצמתים
    node_sems = (sem_t*)mmap(NULL, graph.numNodes * sizeof(sem_t),
                             PROT_READ | PROT_WRITE,
                             MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if (node_sems == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }

    // אתחול כל סמפור בזיכרון המשותף עם הגדרת סנכרון חוצת תהליכים (pshared = 1)
    for (int i = 0; i < graph.numNodes; i++) {
        if (sem_init(&node_sems[i], 1, 1) == -1) {
            perror("sem_init failed");
            return 1;
        }
    }
#endif

    int pipefd[2];
    if (pipe(pipefd) == -1) { perror("pipe"); return 1; }

    for (int i = 0; i < numTravelers; i++) {
        pid_t pid = fork();
        if (pid == -1) { perror("fork"); return 1; }

        if (pid == 0) {
            close(pipefd[0]);   /* child only writes */
            childProcess(&graph, travelers[i].startNode,
                         travelers[i].endNode, pipefd[1], node_sems);
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

#if MILESTONE == 6
    // הריסת הסמפורים המשותפים ושחרור מיפוי הזיכרון בסיום התוכנית
    for (int i = 0; i < graph.numNodes; i++) {
        sem_destroy(&node_sems[i]);
    }
    munmap(node_sems, graph.numNodes * sizeof(sem_t));
#endif

#endif /* MILESTONE */

    free(states);
    free(travelers);
    freeGraph(&graph);
    return 0;
}
#endif /* MILESTONE */