#ifndef SCHED_H
#define SCHED_H

#include <semaphore.h>
#include <sys/types.h>

#define SCHED_MAX_NODES     16
#define SCHED_MAX_TRAVELERS 10

typedef enum {
    SCHED_FCFS,
    SCHED_SJF
} SchedPolicy;

typedef struct {
    pid_t pid;
    int   traveler_idx;
    int   remaining_cost;
    long  arrival_us;
} SchedWaitEntry;

typedef struct {
    SchedPolicy policy;
    int         num_nodes;
    int         node_busy[SCHED_MAX_NODES];
    SchedWaitEntry queue[SCHED_MAX_NODES][SCHED_MAX_TRAVELERS];
    int         queue_len[SCHED_MAX_NODES];
    sem_t*      grant_sems;
} Scheduler;

void          scheduler_init(Scheduler* s, SchedPolicy policy, int num_nodes, sem_t* grant_sems);
void          scheduler_on_request(Scheduler* s, int node, pid_t pid, int traveler_idx,
                                   int remaining_cost);
void          scheduler_on_enter(Scheduler* s, int node);
void          scheduler_on_leave(Scheduler* s, int node);
const char*   sched_policy_name(SchedPolicy policy);

#endif
