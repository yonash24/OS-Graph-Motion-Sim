
#ifndef VISUALIZATION_H
#define VISUALIZATION_H

#include "raylib.h"
#include <math.h>
#include <stdlib.h>
#include "dijkstra.h"

#ifdef __MINGW32__
#include <sys/types.h>
#include <sys/stat.h>

extern int stat64i32(const char *path, struct _stat *buffer) {
    return _stat(path, buffer);
}

#endif


typedef enum {
    ANIM_IDLE,      // לפני לחיצת PLAY
    ANIM_MOVING,    // ישות נעה לאורך קשת
    ANIM_PAUSING,   // ישות מחכה בצומת ביניים (שנייה שלמה)
    ANIM_DONE       // הגיעה ליעד
} AnimState;

#define JUMP_DURATION 0.30f  // 300ms לכל קפיצה
#define NODE_PAUSE    1.00f  // שנייה שלמה בכל צומת ביניים

Vector2 lerpV2(Vector2 a, Vector2 b, float t);

int findEdgeWeight(Graph* g, int from, int to);

int isOnPath(int* path, int pathLen, int i, int dest);

void visualizeGraph(void* g_ptr, int* path, int pathLen, int startNode, int endNode);

#endif //VISUALIZATION_H
