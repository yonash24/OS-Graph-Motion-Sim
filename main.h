#ifndef MAIN_H
#define MAIN_H

/*
 * ═══════════════════════════════════════════════════════════════════════════
 *  מפת אבני דרך — לאן ללכת במבחן
 * ═══════════════════════════════════════════════════════════════════════════
 *
 *  M1  Dijkstra, קריאת קובץ, הדפסת מסלול
 *      → dijkstra.c : loadGraph(), runDijkstra()
 *      → main.c     : main() תחת #if MILESTONE <= 3
 *
 *  M2  GUI סטטי (raylib)
 *      → visualization.c : drawGraph(), buildPositions()
 *      → main.c          : visualizeGraph()  (#if MILESTONE >= 2)
 *
 *  M3  אנימציה (PLAY/STOP, 300ms/weight, 1s pause)
 *      → visualization.c : visualizeGraph() — לולאת אנימציה
 *      → visualization.h : JUMP_DURATION, NODE_PAUSE
 *
 *  M4  fork(), נוסעים מרובים, האב מחשב מסלולים
 *      → main.c          : #if MILESTONE == 4 (fork, pause, SIGTERM)
 *      → visualization.c : visualizeMultiTravelers() — milestone == 4
 *      → dijkstra.c      : loadGraphAndTravelers()
 *
 *  M5  IPC (pipe), ילדים אוטונומיים, לוג מההורה
 *      → ipc.h           : IPC_Message, TravelerStatus
 *      → main.c          : childProcess M5, pipe(), fork
 *      → visualization.c : applyIPCMessage(), drainIPCPipe()
 *
 *  M6  סנכרון צמתים (sem_t + mmap)
 *      → main.c          : node_sems, childProcess M6 (sem_wait/post)
 *      → visualization.c : STATUS_WAITING, outsideNodePos, ANIM_WAITING
 *
 *  M7  תזמון FCFS/SJF (-schd fcfs|sjf)
 *      → sched.c/h       : Scheduler, pick_next(), try_admit(), scheduler_poll()
 *      → main.c          : parseM7Args(), grant_sems, childProcess M7
 *      → ipc.h           : STATUS_SCHEDULE_REQUEST, remaining_cost
 *      → visualization.c : scheduler_on_* ב-applyIPCMessage()
 *
 *  Makefile : make milestoneN  →  -DMILESTONE=N
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "dijkstra.h"
#include "visualization.h"
#include "ipc.h"
#include "sched.h"

#endif // MAIN_H
