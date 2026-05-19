#define _DEFAULT_SOURCE
#include "visualization.h"
#include <signal.h>

/* ── helpers ──────────────────────────────────────────────── */

Vector2 lerpV2(Vector2 a, Vector2 b, float t) {
    return (Vector2){ a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
}

int findEdgeWeight(Graph* g, int from, int to) {
    Edge* e = g->nodes[from].edgeList;
    while (e) {
        if (e->dest == to) return e->weight;
        e = e->next;
    }
    return 1;
}

int isOnPath(int* path, int pathLen, int i, int dest) {
    for (int p = 0; p < pathLen - 1; p++)
        if (path[p] == i && path[p + 1] == dest) return 1;
    return 0;
}

/* ── helper: compute node positions on a circle ──────────── */

static Vector2* buildPositions(Graph* graph, int SCR_W, int SCR_H) {
    Vector2* positions = (Vector2*)malloc(graph->numNodes * sizeof(Vector2));
    float radius = 200.0f;
    Vector2 center = { (float)SCR_W / 2, (float)SCR_H / 2 };
    for (int i = 0; i < graph->numNodes; i++) {
        float angle = i * (2.0f * PI / graph->numNodes);
        positions[i].x = center.x + radius * cosf(angle);
        positions[i].y = center.y + radius * sinf(angle);
    }
    return positions;
}

/* ── helper: draw the graph (edges + nodes) ──────────────── */

static void drawGraph(Graph* graph, Vector2* positions,
                      int* path, int pathLen) {
    // Edges
    for (int i = 0; i < graph->numNodes; i++) {
        Edge* e = graph->nodes[i].edgeList;
        while (e) {
            Vector2 sp = positions[i];
            Vector2 ep = positions[e->dest];
            int     on = isOnPath(path, pathLen, i, e->dest);
            DrawLineEx(sp, ep, on ? 3.5f : 1.8f, on ? ORANGE : LIGHTGRAY);

            float midX = (sp.x + ep.x) / 2.0f;
            float midY = (sp.y + ep.y) / 2.0f;
            DrawCircle((int)midX, (int)midY, 10, RAYWHITE);
            DrawText(TextFormat("%d", e->weight),
                     (int)midX - 5, (int)midY - 5, 15, DARKBLUE);

            float ang = atan2f(ep.y - sp.y, ep.x - sp.x);
            DrawCircle((int)(ep.x - 22.0f * cosf(ang)),
                       (int)(ep.y - 22.0f * sinf(ang)), 4, DARKGRAY);
            e = e->next;
        }
    }
    // Nodes
    for (int i = 0; i < graph->numNodes; i++) {
        Color nc = BLUE;
        if (path && pathLen > 0 && i == path[0])           nc = GREEN;
        if (path && pathLen > 0 && i == path[pathLen - 1]) nc = RED;
        DrawCircleV(positions[i], 20, nc);
        DrawCircleLines((int)positions[i].x, (int)positions[i].y, 20, DARKBLUE);
        DrawText(TextFormat("%d", i),
                 (int)positions[i].x - 5, (int)positions[i].y - 8, 16, WHITE);
    }
}

/* ═══════════════════════════════════════════════════════════
   Milestone 1-3: single traveler
   ═══════════════════════════════════════════════════════════ */

void visualizeGraph(void* g_ptr, int* path, int pathLen,
                    int startNode, int endNode) {
    Graph* graph = (Graph*)g_ptr;
    const int SCR_W = 800, SCR_H = 600;

    InitWindow(SCR_W, SCR_H, "OS Graph Motion Sim - Milestone 3");
    SetTargetFPS(60);

    Vector2* positions = buildPositions(graph, SCR_W, SCR_H);

    int hasPath   = (pathLen > 1);
    int totalDist = 0;
    for (int p = 0; p < pathLen - 1; p++)
        totalDist += findEdgeWeight(graph, path[p], path[p + 1]);

    AnimState animState = ANIM_IDLE;
    int   playing = 0, edgeIdx = 0, subJump = 0;
    float timer   = 0.0f;
    Vector2 entPos = (pathLen > 0) ? positions[path[0]] : (Vector2){SCR_W/2.f, SCR_H/2.f};
    Rectangle btn = { 20, SCR_H - 50, 120, 32 };

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hasPath) {
            Vector2 mouse = GetMousePosition();
            if (CheckCollisionPointRec(mouse, btn)) {
                if (animState == ANIM_DONE) {
                    animState = ANIM_IDLE; edgeIdx = 0; subJump = 0;
                    timer = 0.0f; entPos = positions[path[0]]; playing = 0;
                } else {
                    playing = !playing;
                    if (animState == ANIM_IDLE && playing) animState = ANIM_MOVING;
                }
            }
        }

        if (playing) {
            if (animState == ANIM_MOVING) {
                int from = path[edgeIdx], to = path[edgeIdx + 1];
                int w = findEdgeWeight(graph, from, to);
                timer += dt;
                float jp = timer / JUMP_DURATION; if (jp > 1.0f) jp = 1.0f;
                entPos = lerpV2(positions[from], positions[to],
                                ((float)subJump + jp) / (float)w);
                if (timer >= JUMP_DURATION) {
                    timer -= JUMP_DURATION; subJump++;
                    if (subJump >= w) {
                        subJump = 0; edgeIdx++;
                        entPos = positions[path[edgeIdx]];
                        if (edgeIdx >= pathLen - 1) { animState = ANIM_DONE; playing = 0; }
                        else { animState = ANIM_PAUSING; timer = 0.0f; }
                    }
                }
            } else if (animState == ANIM_PAUSING) {
                timer += dt;
                if (timer >= NODE_PAUSE) { timer = 0.0f; animState = ANIM_MOVING; }
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);
        drawGraph(graph, positions, path, pathLen);
        if (hasPath) {
            if (animState != ANIM_IDLE) { DrawCircleV(entPos, 12, YELLOW); DrawCircleLines((int)entPos.x, (int)entPos.y, 12, GOLD); }
            else { DrawCircleV(positions[path[0]], 12, YELLOW); DrawCircleLines((int)positions[path[0]].x, (int)positions[path[0]].y, 12, GOLD); }
        }
        Color btnC = !hasPath ? GRAY : (animState == ANIM_DONE ? DARKBLUE : (playing ? RED : GREEN));
        DrawRectangleRec(btn, btnC); DrawRectangleLinesEx(btn, 2, DARKGRAY);
        const char* lbl = animState == ANIM_DONE ? "RESTART" : (playing ? "STOP" : "PLAY");
        int bw = MeasureText(lbl, 18);
        DrawText(lbl, (int)(btn.x + (btn.width - bw)/2), (int)(btn.y + (btn.height - 18)/2), 18, WHITE);
        DrawText("OS Graph Motion Sim - Milestone 3", 20, 15, 18, DARKGRAY);
        if (hasPath) DrawText(TextFormat("Shortest path: %d -> %d  |  Distance: %d", startNode, endNode, totalDist), 20, 42, 14, DARKGRAY);
        if (animState == ANIM_DONE) {
            const char* msg = "Destination Reached!"; int tw = MeasureText(msg, 30);
            DrawRectangle(SCR_W/2 - tw/2 - 14, SCR_H/2 - 28, tw + 28, 56, Fade(YELLOW, 0.92f));
            DrawText(msg, SCR_W/2 - tw/2, SCR_H/2 - 15, 30, DARKGREEN);
        }
        DrawText("ESC to exit", SCR_W - 95, SCR_H - 25, 13, GRAY);
        EndDrawing();
    }
    free(positions);
    CloseWindow();
}

/* ═══════════════════════════════════════════════════════════
   Milestones 4 & 5: multi-traveler
   pipe_fd == -1  → Milestone 4 (parent-driven from path[])
   pipe_fd >= 0   → Milestone 5 (IPC-driven via pipe)
   ═══════════════════════════════════════════════════════════ */

void visualizeMultiTravelers(void* g_ptr, TravelerState* states,
                             int numTravelers, int pipe_fd) {
    Graph* graph = (Graph*)g_ptr;
    const int SCR_W = 800, SCR_H = 600;
    int milestone = (pipe_fd == -1) ? 4 : 5;

    InitWindow(SCR_W, SCR_H,
               milestone == 4
                   ? "OS Graph Motion Sim - Milestone 4"
                   : "OS Graph Motion Sim - Milestone 5");
    SetTargetFPS(60);

    Vector2* positions = buildPositions(graph, SCR_W, SCR_H);

    // Place each traveler at its start node
    for (int i = 0; i < numTravelers; i++)
        states[i].entPos = positions[states[i].currentNode];

    // Milestone 5 auto-starts; Milestone 4 waits for PLAY button
    int playing     = (milestone == 5) ? 1 : 0;
    int allFinished = 0;
    Rectangle btn   = { 20, SCR_H - 50, 120, 32 };

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        /* ── Button ── */
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !allFinished) {
            Vector2 mouse = GetMousePosition();
            if (CheckCollisionPointRec(mouse, btn)) {
                playing = !playing;
                // Milestone 4: activate travelers on first PLAY
                if (milestone == 4 && playing) {
                    for (int i = 0; i < numTravelers; i++)
                        if (states[i].animState == ANIM_IDLE && states[i].pathLen > 1) {
                            states[i].animState = ANIM_MOVING;
                            states[i].isActive  = 1;
                        }
                }
            }
        }

        if (playing) {
            /* ── Milestone 5: read IPC messages from pipe ── */
            if (milestone == 5) {
                IPC_Message msg;
                while (read(pipe_fd, &msg, sizeof(msg)) == (ssize_t)sizeof(IPC_Message)) {
                    for (int i = 0; i < numTravelers; i++) {
                        if (states[i].pid != msg.child_pid) continue;
                        if (msg.is_destination) {
                            states[i].isFinished  = 1;
                            states[i].isActive    = 0;
                            states[i].animState   = ANIM_DONE;
                            states[i].currentNode = msg.current_node;
                            states[i].entPos      = positions[msg.current_node];
                            printf("[PID=%d] arrived at node %d | DESTINATION\n",
                                   msg.child_pid, msg.current_node);
                            fflush(stdout);
                        } else {
                            states[i].currentNode = msg.current_node;
                            states[i].nextNode    = msg.next_node;
                            states[i].isActive    = 1;
                            states[i].animState   = ANIM_MOVING;
                            states[i].timer       = 0.0f;
                            states[i].subJump     = 0;
                            states[i].entPos      = positions[msg.current_node];
                            printf("[PID=%d] arrived at node %d | next node: %d\n",
                                   msg.child_pid, msg.current_node, msg.next_node);
                            fflush(stdout);
                        }
                        break;
                    }
                }
            }

            /* ── Animation update (both milestones share the same drawing;
                   only the *source* of movement differs) ── */
            allFinished = 1;
            for (int i = 0; i < numTravelers; i++) {
                if (states[i].isFinished) continue;
                allFinished = 0;

                if (milestone == 4) {
                    /* Parent drives the animation using pre-computed path[] */
                    if (states[i].animState == ANIM_MOVING && states[i].pathLen > 1) {
                        int from = states[i].path[states[i].edgeIdx];
                        int to   = states[i].path[states[i].edgeIdx + 1];
                        int w    = findEdgeWeight(graph, from, to);

                        states[i].timer += dt;
                        float jp = states[i].timer / JUMP_DURATION;
                        if (jp > 1.0f) jp = 1.0f;
                        states[i].entPos = lerpV2(positions[from], positions[to],
                                                  ((float)states[i].subJump + jp) / (float)w);

                        if (states[i].timer >= JUMP_DURATION) {
                            states[i].timer -= JUMP_DURATION;
                            states[i].subJump++;
                            if (states[i].subJump >= w) {
                                states[i].subJump    = 0;
                                states[i].edgeIdx++;
                                states[i].currentNode = states[i].path[states[i].edgeIdx];
                                states[i].entPos      = positions[states[i].currentNode];
                                if (states[i].edgeIdx >= states[i].pathLen - 1) {
                                    states[i].animState  = ANIM_DONE;
                                    states[i].isFinished = 1;
                                    kill(states[i].pid, SIGTERM);
                                } else {
                                    states[i].animState = ANIM_PAUSING;
                                    states[i].timer     = 0.0f;
                                }
                            }
                        }
                    } else if (states[i].animState == ANIM_PAUSING) {
                        states[i].timer += dt;
                        if (states[i].timer >= NODE_PAUSE) {
                            states[i].timer     = 0.0f;
                            states[i].animState = ANIM_MOVING;
                        }
                    }
                } else {
                    /* Milestone 5: interpolate between currentNode and nextNode
                       while waiting for the next IPC update from the child */
                    if (states[i].isActive) {
                        int from = states[i].currentNode;
                        int to   = states[i].nextNode;
                        int w    = findEdgeWeight(graph, from, to);

                        states[i].timer += dt;
                        float jp = states[i].timer / JUMP_DURATION;
                        if (jp > 1.0f) jp = 1.0f;
                        
                        float edgeT = ((float)states[i].subJump + jp) / (float)w;
                        if (edgeT >= 1.0f) {
                            states[i].currentNode = to;
                            states[i].isActive    = 0;
                            states[i].entPos      = positions[to];
                        } else {
                            states[i].entPos = lerpV2(positions[from], positions[to], edgeT);
                            if (states[i].timer >= JUMP_DURATION) {
                                states[i].timer -= JUMP_DURATION;
                                states[i].subJump++;
                            }
                        }
                    }
                }
            }
        }

        /* ── Drawing ── */
        BeginDrawing();
        ClearBackground(RAYWHITE);

        drawGraph(graph, positions, NULL, 0);

        // Travelers
        for (int i = 0; i < numTravelers; i++) {
            Color c = states[i].isFinished ? Fade(states[i].color, 0.5f) : states[i].color;
            DrawCircleV(states[i].entPos, 12, c);
            DrawCircleLines((int)states[i].entPos.x, (int)states[i].entPos.y, 12, DARKGRAY);
        }

        // Button
        Color btnColor = allFinished ? DARKBLUE : (playing ? RED : GREEN);
        DrawRectangleRec(btn, btnColor);
        DrawRectangleLinesEx(btn, 2, DARKGRAY);
        const char* btnLabel = allFinished ? "DONE" : (playing ? "STOP" : "PLAY");
        int bw = MeasureText(btnLabel, 18);
        DrawText(btnLabel,
                 (int)(btn.x + (btn.width  - bw) / 2),
                 (int)(btn.y + (btn.height - 18) / 2), 18, WHITE);

        DrawText(milestone == 4
                     ? "OS Graph Motion Sim - Milestone 4"
                     : "OS Graph Motion Sim - Milestone 5",
                 20, 15, 18, DARKGRAY);

        EndDrawing();
    }

    free(positions);
    CloseWindow();
}