#include "graph_visual.h"
#include "raylib.h"
#include <math.h>
#include <stdlib.h>

// הגדרת המבנים מחדש בתוך ה-C כדי שתהיה גישה לנתונים
typedef struct Edge {
    int dest;
    int weight;
    struct Edge* next;
} Edge;

typedef struct Node {
    int id;
    Edge* edgeList;
} Node;

typedef struct Graph {
    Node* nodes;
    int numNodes;
    int numEdges;
} Graph;

// מצבי האנימציה
typedef enum {
    ANIM_IDLE,      // לפני לחיצת PLAY
    ANIM_MOVING,    // ישות נעה לאורך קשת
    ANIM_PAUSING,   // ישות מחכה בצומת ביניים (שנייה שלמה)
    ANIM_DONE       // הגיעה ליעד
} AnimState;

#define JUMP_DURATION 0.30f  // 300ms לכל קפיצה
#define NODE_PAUSE    1.00f  // שנייה שלמה בכל צומת ביניים

static Vector2 lerpV2(Vector2 a, Vector2 b, float t) {
    return (Vector2){ a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
}

// מחזיר את משקל הקשת מ-from ל-to, או 1 אם לא נמצאה
static int findEdgeWeight(Graph* g, int from, int to) {
    Edge* e = g->nodes[from].edgeList;
    while (e) {
        if (e->dest == to) return e->weight;
        e = e->next;
    }
    return 1;
}

// בודק אם הקשת (i->dest) היא חלק מהמסלול הקצר ביותר
static int isOnPath(int* path, int pathLen, int i, int dest) {
    for (int p = 0; p < pathLen - 1; p++)
        if (path[p] == i && path[p + 1] == dest) return 1;
    return 0;
}

void visualizeGraph(void* g_ptr, int* path, int pathLen, int startNode, int endNode) {
    Graph* graph = (Graph*)g_ptr;
    const int SCR_W = 800;
    const int SCR_H = 600;

    InitWindow(SCR_W, SCR_H, "OS Graph Motion Sim - Milestone 3");
    SetTargetFPS(60);

    // חישוב ושמירת מיקומי הצמתים במעגל
    Vector2* positions = (Vector2*)malloc(graph->numNodes * sizeof(Vector2));
    float radius = 200.0f;
    Vector2 center = { (float)SCR_W / 2, (float)SCR_H / 2 };

    for (int i = 0; i < graph->numNodes; i++) {
        float angle = i * (2.0f * PI / graph->numNodes);
        positions[i].x = center.x + radius * cosf(angle);
        positions[i].y = center.y + radius * sinf(angle);
    }

    int hasPath = (pathLen > 1);

    // חישוב סך המרחק למסלול
    int totalDist = 0;
    for (int p = 0; p < pathLen - 1; p++)
        totalDist += findEdgeWeight(graph, path[p], path[p + 1]);

    // מצב אנימציה
    AnimState animState = ANIM_IDLE;
    int playing  = 0;
    int edgeIdx  = 0;   // אינדקס קשת נוכחית ב-path: path[edgeIdx]->path[edgeIdx+1]
    int subJump  = 0;   // מספר הקפיצה הנוכחית בתוך הקשת (0 עד weight-1)
    float timer  = 0.0f;
    Vector2 entPos = (pathLen > 0) ? positions[path[0]] : center;

    // כפתור PLAY/STOP
    Rectangle btn = { 20, SCR_H - 50, 120, 32 };

    // לולאת הגרפיקה הראשית
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // --- טיפול בלחיצת כפתור ---
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hasPath) {
            Vector2 mouse = GetMousePosition();
            if (CheckCollisionPointRec(mouse, btn)) {
                if (animState == ANIM_DONE) {
                    // איפוס האנימציה
                    animState = ANIM_IDLE;
                    edgeIdx = 0; subJump = 0; timer = 0.0f;
                    entPos = positions[path[0]];
                    playing = 0;
                } else {
                    playing = !playing;
                    // הפעלה ראשונה: מעבר ממצב המתנה למצב תנועה
                    if (animState == ANIM_IDLE && playing)
                        animState = ANIM_MOVING;
                }
            }
        }

        // --- עדכון האנימציה ---
        if (playing) {
            if (animState == ANIM_MOVING) {
                int from = path[edgeIdx];
                int to   = path[edgeIdx + 1];
                int w    = findEdgeWeight(graph, from, to);

                timer += dt;

                // עדכון מיקום הישות לאורך הקשת
                float jumpProg = timer / JUMP_DURATION;
                if (jumpProg > 1.0f) jumpProg = 1.0f;
                float edgeT = ((float)subJump + jumpProg) / (float)w;
                entPos = lerpV2(positions[from], positions[to], edgeT);

                // סיום קפיצה אחת (300ms עברו)
                if (timer >= JUMP_DURATION) {
                    timer -= JUMP_DURATION;
                    subJump++;

                    if (subJump >= w) {
                        // סיום קשת שלמה - הגענו לצומת הבא
                        subJump = 0;
                        edgeIdx++;
                        entPos = positions[path[edgeIdx]];  // חיבור לצומת

                        if (edgeIdx >= pathLen - 1) {
                            // הגענו לצומת היעד
                            animState = ANIM_DONE;
                            playing = 0;
                        } else {
                            // צומת ביניים: עצירה של שנייה שלמה
                            animState = ANIM_PAUSING;
                            timer = 0.0f;
                        }
                    }
                }

            } else if (animState == ANIM_PAUSING) {
                // המתנה בצומת ביניים
                timer += dt;
                if (timer >= NODE_PAUSE) {
                    timer = 0.0f;
                    animState = ANIM_MOVING;
                }
            }
        }

        // --- ציור ---
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // 1. ציור הקשתות (Edges), חצים ומשקלים
        for (int i = 0; i < graph->numNodes; i++) {
            Edge* currEdge = graph->nodes[i].edgeList;
            while (currEdge != NULL) {
                Vector2 startPos = positions[i];
                Vector2 endPos   = positions[currEdge->dest];

                // הדגשת קשתות שנמצאות במסלול הקצר ביותר
                int onPath = isOnPath(path, pathLen, i, currEdge->dest);
                Color edgeColor = onPath ? ORANGE : LIGHTGRAY;
                float lineThick = onPath ? 3.5f : 1.8f;

                DrawLineEx(startPos, endPos, lineThick, edgeColor);

                // ציור רקע קטן למשקל כדי שיהיה קריא
                float midX = (startPos.x + endPos.x) / 2.0f;
                float midY = (startPos.y + endPos.y) / 2.0f;
                DrawCircle((int)midX, (int)midY, 10, RAYWHITE);
                DrawText(TextFormat("%d", currEdge->weight), (int)midX - 5, (int)midY - 5, 15, DARKBLUE);

                // סימון חץ (עיגול קטן ליד היעד כדי להראות כיווניות)
                float ang = atan2f(endPos.y - startPos.y, endPos.x - startPos.x);
                float arrowDist = 22.0f;
                DrawCircle(
                    (int)(endPos.x - arrowDist * cosf(ang)),
                    (int)(endPos.y - arrowDist * sinf(ang)),
                    4, DARKGRAY
                );

                currEdge = currEdge->next;
            }
        }

        // 2. ציור הצמתים (Nodes) - מעל הקווים
        for (int i = 0; i < graph->numNodes; i++) {
            Color nodeColor = BLUE;
            if (hasPath && i == path[0])           nodeColor = GREEN;  // מקור
            if (hasPath && i == path[pathLen - 1]) nodeColor = RED;    // יעד

            DrawCircleV(positions[i], 20, nodeColor);
            DrawCircleLines((int)positions[i].x, (int)positions[i].y, 20, DARKBLUE);
            DrawText(TextFormat("%d", i),
                     (int)positions[i].x - 5, (int)positions[i].y - 8, 16, WHITE);
        }

        // 3. ציור הישות הנעה (עיגול צהוב)
        if (animState != ANIM_IDLE && hasPath) {
            DrawCircleV(entPos, 12, YELLOW);
            DrawCircleLines((int)entPos.x, (int)entPos.y, 12, GOLD);
        } else if (animState == ANIM_IDLE && hasPath) {
            // לפני הפעלה: הישות ממוקמת ליד צומת המקור
            DrawCircleV(positions[path[0]], 12, YELLOW);
            DrawCircleLines((int)positions[path[0]].x, (int)positions[path[0]].y, 12, GOLD);
        }

        // 4. כפתור PLAY / STOP / RESTART
        Color btnColor = playing ? RED : GREEN;
        if (animState == ANIM_DONE) btnColor = DARKBLUE;
        if (!hasPath) btnColor = GRAY;

        DrawRectangleRec(btn, btnColor);
        DrawRectangleLinesEx(btn, 2, DARKGRAY);

        const char* btnLabel;
        if      (animState == ANIM_DONE)    btnLabel = "RESTART";
        else if (playing)                   btnLabel = "STOP";
        else                                btnLabel = "PLAY";

        int bw = MeasureText(btnLabel, 18);
        DrawText(btnLabel,
                 (int)(btn.x + (btn.width  - bw) / 2),
                 (int)(btn.y + (btn.height - 18) / 2),
                 18, WHITE);

        // 5. כותרת ומידע
        DrawText("OS Graph Motion Sim - Milestone 3", 20, 15, 18, DARKGRAY);

        if (hasPath) {
            DrawText(TextFormat("Shortest path: %d -> %d  |  Distance: %d",
                                startNode, endNode, totalDist),
                     20, 42, 14, DARKGRAY);
        } else {
            DrawText("No path found between source and destination", 20, 42, 14, RED);
        }

        // אגנדה
        DrawCircle(20, SCR_H - 90, 8, GREEN);
        DrawText("Source", 35, SCR_H - 97, 13, DARKGRAY);
        DrawCircle(20, SCR_H - 72, 8, RED);
        DrawText("Destination", 35, SCR_H - 79, 13, DARKGRAY);
        DrawCircle(20, SCR_H - 54, 8, YELLOW);
        DrawText("Moving entity", 35, SCR_H - 61, 13, DARKGRAY);

        DrawText("ESC to exit", SCR_W - 95, SCR_H - 25, 13, GRAY);

        // 6. הודעת הגעה ליעד
        if (animState == ANIM_DONE) {
            const char* msg  = "Destination Reached!";
            int tw = MeasureText(msg, 30);
            int rx = SCR_W / 2 - tw / 2 - 14;
            int ry = SCR_H / 2 - 28;
            DrawRectangle(rx, ry, tw + 28, 56, Fade(YELLOW, 0.92f));
            DrawRectangleLines(rx, ry, tw + 28, 56, GOLD);
            DrawText(msg, SCR_W / 2 - tw / 2, SCR_H / 2 - 15, 30, DARKGREEN);
        }

        EndDrawing();
    }

    free(positions);
    CloseWindow();
}
