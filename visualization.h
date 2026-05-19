#ifndef VISUALIZATION_H
#define VISUALIZATION_H

#include "raylib.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "dijkstra.h"
#include "ipc.h"

typedef enum {
    ANIM_IDLE,      // לפני לחיצת PLAY
    ANIM_MOVING,    // ישות נעה לאורך קשת
    ANIM_PAUSING,   // ישות מחכה בצומת ביניים (שנייה שלמה)
    ANIM_DONE       // הגיעה ליעד
} AnimState;

// State for each traveler – shared between Milestone 4 and 5.
// path/pathLen/edgeIdx are used by Milestone 4 (parent-driven).
// For Milestone 5 they are NULL/0 (child-driven via IPC).
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

    // Milestone 4: pre-computed path by parent (NULL in Milestone 5)
    int* path;
    int  pathLen;
    int  edgeIdx;
} TravelerState;

#define JUMP_DURATION 0.30f  // 300ms per sub-jump
#define NODE_PAUSE    1.00f  // 1 second pause at intermediate nodes

Vector2 lerpV2(Vector2 a, Vector2 b, float t);
int     findEdgeWeight(Graph* g, int from, int to);
int     isOnPath(int* path, int pathLen, int i, int dest);

// Single-traveler visualization (Milestones 1-3)
void visualizeGraph(void* g_ptr, int* path, int pathLen, int startNode, int endNode);

// Multi-traveler visualization (Milestones 4 and 5).
// pipe_fd == -1  → Milestone 4: parent drives the animation from pre-computed paths.
// pipe_fd >= 0   → Milestone 5: parent reads IPC messages from children.
void visualizeMultiTravelers(void* g_ptr, TravelerState* states, int numTravelers, int pipe_fd);

#endif // VISUALIZATION_H
