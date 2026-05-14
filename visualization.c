#include "main.h"

Vector2 lerpV2(Vector2 a, Vector2 b, float t) {
    return (Vector2){ a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
}

void visualizeGraph(Graph* graph, Traveler* travelers, int numTravelers) {
    InitWindow(800, 600, "OS Graph Motion Sim - Milestone 4");
    SetTargetFPS(60);

    Vector2* nodePos = (Vector2*)malloc(graph->numNodes * sizeof(Vector2));
    for (int i = 0; i < graph->numNodes; i++) {
        float angle = i * (2.0f * PI / graph->numNodes);
        nodePos[i] = (Vector2){ 400 + 220 * cosf(angle), 300 + 220 * sinf(angle) };
    }

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        for (int i = 0; i < numTravelers; i++) {
            Traveler* t = &travelers[i];
            if (t->arrived) continue;

            if (t->state == ANIM_MOVING) {
                int from = t->path[t->edgeIdx];
                int to = t->path[t->edgeIdx + 1];
                int w = findEdgeWeight(graph, from, to);

                t->timer += dt;
                float progress = ((float)t->subJump + (t->timer / JUMP_DURATION)) / (float)w;
                t->currentPos = lerpV2(nodePos[from], nodePos[to], fminf(progress, 1.0f));

                if (t->timer >= JUMP_DURATION) {
                    t->timer = 0;
                    t->subJump++;
                    if (t->subJump >= w) {
                        t->subJump = 0;
                        t->edgeIdx++;
                        if (t->edgeIdx >= t->pathLen - 1) {
                            t->state = ANIM_DONE;
                            t->arrived = true;
                            kill(t->pid, SIGTERM);
                        } else {
                            t->state = ANIM_PAUSING;
                        }
                    }
                }
            } else if (t->state == ANIM_PAUSING) {
                t->timer += dt;
                t->currentPos = nodePos[t->path[t->edgeIdx]];
                if (t->timer >= NODE_PAUSE) {
                    t->timer = 0;
                    t->state = ANIM_MOVING;
                }
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);
        for (int i = 0; i < graph->numNodes; i++) {
            Edge* e = graph->nodes[i].edgeList;
            while (e) {
                DrawLineV(nodePos[i], nodePos[e->dest], LIGHTGRAY);
                e = e->next;
            }
        }
        for (int i = 0; i < graph->numNodes; i++) {
            DrawCircleV(nodePos[i], 20, BLUE);
            DrawText(TextFormat("%d", i), nodePos[i].x-5, nodePos[i].y-8, 16, WHITE);
        }
        for (int i = 0; i < numTravelers; i++) {
            if (!travelers[i].arrived) {
                DrawCircleV(travelers[i].currentPos, 12, travelers[i].color);
                DrawCircleLines(travelers[i].currentPos.x, travelers[i].currentPos.y, 12, BLACK);
            }
        }
        EndDrawing();
    }
    free(nodePos);
    CloseWindow();
}
