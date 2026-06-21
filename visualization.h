#ifndef VISUALIZATION_H
#define VISUALIZATION_H

#include "raylib.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "dijkstra.h"
#include "ipc.h"
#include "sched.h"

typedef enum {
    ANIM_IDLE,      // לפני לחיצת PLAY
    ANIM_MOVING,    // ישות נעה לאורך קשת
    ANIM_PAUSING,   // ישות נמצאת בתוך צומת (שנייה שלמה)
    ANIM_WAITING,   // ישות ממתינה מחוץ לצומת (חסומה על ידי סמפור)
    ANIM_DONE       // הגיעה ליעד
} AnimState;

typedef struct {
    pid_t  pid;
    int    currentNode;
    int    nextNode;
    int    isFinished;
    int    isActive;
    Color  color;

    // Animation
    float    timer;
    int      subJump;
    Vector2  entPos;
    AnimState animState;

    // Milestone 4: pre-computed path by parent (NULL in Milestone 5/6)
    int* path;
    int  pathLen;
    int  edgeIdx;
} TravelerState;

#define JUMP_DURATION 0.30f  // 300ms per sub-jump
#define NODE_PAUSE    1.00f  // 1 second pause at intermediate nodes

Vector2 lerpV2(Vector2 a, Vector2 b, float t);
int     findEdgeWeight(Graph* g, int from, int to);
int     isOnPath(int* path, int pathLen, int i, int dest);

void visualizeGraph(void* g_ptr, int* path, int pathLen, int startNode, int endNode);
void visualizeMultiTravelers(void* g_ptr, TravelerState* states, int numTravelers,
                             int pipe_fd, Scheduler* sched);

#endif // VISUALIZATION_H