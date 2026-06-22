#define _DEFAULT_SOURCE
#include "sched.h"
#include <stdio.h>
#include <time.h>

/* ═══════════════════════════════════════════════════════════
   [M7] sched.c — תור המתנה לכל צומת, FCFS / SJF
   pick_next()  — בוחר מי הבא בתור (+ anti-starvation)
   try_admit()  — משחרר sem_post לנוסע שנבחר
   ═══════════════════════════════════════════════════════════ */

static long now_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000L + ts.tv_nsec / 1000L;
}

static long entry_wait_us(const SchedWaitEntry* e) {
    return now_us() - e->arrival_us;
}

/* ממתין >= 5 שניות → נכנס לפי FCFS (מי שהמתין הכי הרבה) */
static int pick_starved(const Scheduler* s, int node) {
    int len = s->queue_len[node];
    int best = -1;

    for (int i = 0; i < len; i++) {
        const SchedWaitEntry* e = &s->queue[node][i];
        if (entry_wait_us(e) < SCHED_STARVATION_TIMEOUT_US) continue;

        if (best < 0 || e->arrival_us < s->queue[node][best].arrival_us)
            best = i;
    }
    return best;
}

static int pick_next(const Scheduler* s, int node) {  /* [M7] FCFS / SJF */
    int len = s->queue_len[node];
    if (len <= 0) return -1;

    int starved = pick_starved(s, node);
    if (starved >= 0) return starved;

    int best = 0;
    for (int i = 1; i < len; i++) {
        const SchedWaitEntry* a = &s->queue[node][best];
        const SchedWaitEntry* b = &s->queue[node][i];

        if (s->policy == SCHED_SJF) {
            if (b->remaining_cost < a->remaining_cost ||
                (b->remaining_cost == a->remaining_cost && b->priority < a->priority) ||
                (b->remaining_cost == a->remaining_cost && b->priority == a->priority &&
                 b->arrival_us < a->arrival_us)) {
                best = i;
            }
        } else {
            if (b->arrival_us < a->arrival_us) best = i;
        }
    }
    return best;
}

static void try_admit(Scheduler* s, int node) {
    if (node < 0 || node >= s->num_nodes) return;
    if (s->node_busy[node] || s->queue_len[node] <= 0) return;

    int pick = pick_next(s, node);
    if (pick < 0) return;

    SchedWaitEntry chosen = s->queue[node][pick];
    long waited_us = entry_wait_us(&chosen);
    int anti_starvation = (waited_us >= SCHED_STARVATION_TIMEOUT_US);

    for (int i = pick; i < s->queue_len[node] - 1; i++)
        s->queue[node][i] = s->queue[node][i + 1];
    s->queue_len[node]--;

    s->node_busy[node] = 1;

    if (anti_starvation) {
        printf("[Scheduler/%s] granted node %d to PID=%d (anti-starvation, waited %.1fs)\n",
               sched_policy_name(s->policy), node, (int)chosen.pid,
               waited_us / 1000000.0);
    } else {
        printf("[Scheduler/%s] granted node %d to PID=%d (remaining=%d, priority=%d)\n",
               sched_policy_name(s->policy), node, (int)chosen.pid,
               chosen.remaining_cost, chosen.priority);
    }
    fflush(stdout);

    sem_post(&s->grant_sems[chosen.traveler_idx]);
}

void scheduler_init(Scheduler* s, SchedPolicy policy, int num_nodes, sem_t* grant_sems) {
    s->policy = policy;
    s->num_nodes = num_nodes;
    s->grant_sems = grant_sems;
    for (int i = 0; i < SCHED_MAX_NODES; i++) {
        s->node_busy[i] = 0;
        s->queue_len[i] = 0;
    }
}

void scheduler_on_request(Scheduler* s, int node, pid_t pid, int traveler_idx,
                          int remaining_cost, int priority) {
    if (node < 0 || node >= s->num_nodes) return;
    if (s->queue_len[node] >= SCHED_MAX_TRAVELERS) return;

    SchedWaitEntry* e = &s->queue[node][s->queue_len[node]++];
    e->pid = pid;
    e->traveler_idx = traveler_idx;
    e->remaining_cost = remaining_cost;
    e->priority = priority;
    e->arrival_us = now_us();

    printf("[Scheduler/%s] PID=%d queued for node %d (remaining=%d, priority=%d, queue=%d)\n",
           sched_policy_name(s->policy), (int)pid, node, remaining_cost, priority,
           s->queue_len[node]);
    fflush(stdout);

    try_admit(s, node);
}

void scheduler_on_enter(Scheduler* s, int node) {
    if (node < 0 || node >= s->num_nodes) return;
    s->node_busy[node] = 1;
}

void scheduler_on_leave(Scheduler* s, int node) {
    if (node < 0 || node >= s->num_nodes) return;
    s->node_busy[node] = 0;
    try_admit(s, node);
}

void scheduler_poll(Scheduler* s) {
    if (!s) return;
    for (int node = 0; node < s->num_nodes; node++) {
        if (s->queue_len[node] > 0 && !s->node_busy[node])
            try_admit(s, node);
    }
}

const char* sched_policy_name(SchedPolicy policy) {
    return (policy == SCHED_SJF) ? "SJF" : "FCFS";
}
