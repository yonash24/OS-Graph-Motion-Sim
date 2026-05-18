#define _DEFAULT_SOURCE
#include "main.h"
#include "ipc.h"
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

void childProcess(Graph* graph, int startNode, int endNode, int pipe_write_fd) {
    int* outPath = (int*)malloc(graph->numNodes * sizeof(int));
    int outLen = 0;
    runDijkstra(graph, startNode, endNode, outPath, &outLen);
    
    if (outLen == 0) {
        IPC_Message msg;
        msg.child_pid = getpid();
        msg.is_destination = 1;
        write(pipe_write_fd, &msg, sizeof(msg));
        free(outPath);
        exit(0);
    }
    
    for (int i = 0; i < outLen - 1; i++) {
        int curr = outPath[i];
        int next = outPath[i+1];
        
        IPC_Message msg;
        msg.child_pid = getpid();
        msg.current_node = curr;
        msg.next_node = next;
        msg.is_destination = 0;
        write(pipe_write_fd, &msg, sizeof(msg));
        
        int weight = findEdgeWeight(graph, curr, next);
        usleep((weight * 300 * 1000) + 1000000); 
    }
    
    IPC_Message msg;
    msg.child_pid = getpid();
    msg.current_node = endNode;
    msg.next_node = -1;
    msg.is_destination = 1;
    write(pipe_write_fd, &msg, sizeof(msg));
    
    free(outPath);
    exit(0);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <file_name>\n", argv[0]);
        return 1;
    }
    const char* filename = argv[1];

    Graph graph;
    Traveler* travelers = NULL;
    int numTravelers = 0;

    if (!loadGraphAndTravelers(filename, &graph, &travelers, &numTravelers)) {
        printf("Failed to load graph and travelers.\n");
        return 1;
    }

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return 1;
    }

    TravelerState* states = (TravelerState*)calloc(numTravelers, sizeof(TravelerState));
    Color colors[] = { RED, GREEN, BLUE, ORANGE, PURPLE, PINK, BROWN, LIME, VIOLET, DARKGREEN };
    int numColors = 10;

    for (int i = 0; i < numTravelers; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            return 1;
        } else if (pid == 0) {
            close(pipefd[0]);
            childProcess(&graph, travelers[i].startNode, travelers[i].endNode, pipefd[1]);
        } else {
            states[i].pid = pid;
            states[i].isActive = 0;
            states[i].isFinished = 0;
            states[i].color = colors[i % numColors];
            states[i].animState = ANIM_IDLE;
            states[i].currentNode = travelers[i].startNode;
            states[i].nextNode = travelers[i].startNode;
        }
    }
    close(pipefd[1]);

    int flags = fcntl(pipefd[0], F_GETFL, 0);
    fcntl(pipefd[0], F_SETFL, flags | O_NONBLOCK);

    visualizeMultiTravelers(&graph, states, numTravelers, pipefd[0]);

    // Parent waits for children
    for (int i = 0; i < numTravelers; i++) {
        int status;
        pid_t finished_pid = wait(&status);
        if (finished_pid > 0) {
            printf("[PID=%d] finished\n", finished_pid);
        }
    }

    free(states);
    free(travelers);
    freeGraph(&graph);

    return 0;
}
