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

void visualizeGraph(void* g_ptr) {
    Graph* graph = (Graph*)g_ptr;
    const int screenWidth = 800;
    const int screenHeight = 600;

    // אתחול חלון - חשוב לוודא שזה קורה רק פעם אחת
    InitWindow(screenWidth, screenHeight, "Graph Milestone 2 - Shahar Shaatal");
    SetTargetFPS(60);

    // חישוב ושמירת מיקומי הצמתים במערך וקטורים
    Vector2* positions = (Vector2*)malloc(graph->numNodes * sizeof(Vector2));
    float radius = 200.0f;
    Vector2 center = { (float)screenWidth / 2, (float)screenHeight / 2 };

    for (int i = 0; i < graph->numNodes; i++) {
        float angle = i * (2.0f * PI / graph->numNodes);
        positions[i].x = center.x + radius * cosf(angle);
        positions[i].y = center.y + radius * sinf(angle);
    }

    // לולאת הגרפיקה הראשית
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // 1. ציור הקשתות (Edges), חצים ומשקלים
        for (int i = 0; i < graph->numNodes; i++) {
            Edge* currEdge = graph->nodes[i].edgeList;
            while (currEdge != NULL) {
                Vector2 startPos = positions[i];
                Vector2 endPos = positions[currEdge->dest];

                // ציור הקו בין הצמתים
                DrawLineEx(startPos, endPos, 2.0f, LIGHTGRAY);

                // חישוב אמצע הקו לטובת ציור המשקל
                float midX = (startPos.x + endPos.x) / 2.0f;
                float midY = (startPos.y + endPos.y) / 2.0f;
                
                // ציור רקע קטן למשקל כדי שיהיה קריא
                DrawCircle(midX, midY, 10, RAYWHITE);
                DrawText(TextFormat("%d", currEdge->weight), midX - 5, midY - 5, 15, DARKBLUE);

                // סימון חץ (עיגול קטן ליד היעד כדי להראות כיווניות)
                float angle = atan2f(endPos.y - startPos.y, endPos.x - startPos.x);
                float arrowDist = 22.0f; // מרחק מהמרכז (קצת יותר מרדיוס הצומת)
                float arrowX = endPos.x - arrowDist * cosf(angle);
                float arrowY = endPos.y - arrowDist * sinf(angle);
                DrawCircle(arrowX, arrowY, 4, DARKGRAY);

                currEdge = currEdge->next;
            }
        }

        // 2. ציור הצמתים (Nodes) - מציירים אחרי הקווים כדי שיהיו "מעליהם"
        for (int i = 0; i < graph->numNodes; i++) {
            DrawCircleV(positions[i], 20, BLUE);
            DrawCircleLines(positions[i].x, positions[i].y, 20, DARKBLUE); // מסגרת לעיגול
            DrawText(TextFormat("%d", i), positions[i].x - 5, positions[i].y - 8, 16, WHITE);
        }

        DrawText("OS Graph Simulator - Milestone 2", 20, 20, 20, DARKGRAY);
        DrawText("Press ESC to exit", 20, 50, 15, GRAY);

        EndDrawing();
    }

    free(positions);
    CloseWindow();
}