#ifndef IPC_H
#define IPC_H

#include <sys/types.h>

typedef struct {
    pid_t child_pid;
    int current_node;
    int next_node;
    int is_destination;
} IPC_Message;

#endif
