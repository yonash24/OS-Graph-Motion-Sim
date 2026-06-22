#define _DEFAULT_SOURCE
#include "visualization.h"
#include "sched.h"
#include <signal.h>
#include <string.h>

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

#define NODE_OUTSIDE_OFFSET 48.0f

/* ── [M6-M7] מיקום מחוץ לצומת (המתנה / סיום קשת) ── */
static Vector2 outsideNodePos(Vector2* positions, int from, int to) {
    Vector2 dir = { positions[from].x - positions[to].x,
                    positions[from].y - positions[to].y };
    float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
    if (len <= 0.0f) return positions[to];
    dir.x /= len;
    dir.y /= len;
    return (Vector2){ positions[to].x + dir.x * NODE_OUTSIDE_OFFSET,
                      positions[to].y + dir.y * NODE_OUTSIDE_OFFSET };
}

static int isTravelerInsideNode(const TravelerState* s) {
    if (s->animState == ANIM_WAITING) return 0;
    if (s->isActive && s->animState == ANIM_MOVING) return 0;
    return 1;
}

/* ── [M5-M7] פיזור נוסעים בתוך צומת (זיהוי וизуלי) ── */
static Vector2 spreadAtNode(Vector2 center, int slot, int total) {
    if (total <= 1) return center;
    float angle = (float)slot * (2.0f * PI / (float)total) - PI / 2.0f;
    const float radius = 14.0f;
    return (Vector2){ center.x + radius * cosf(angle),
                        center.y + radius * sinf(angle) };
}

/* ═══════════════════════════════════════════════════════════
   [M5-M7] applyIPCMessage / drainIPCPipe — קריאת pipe + עדכון GUI
   [M7]    scheduler_on_request/enter/leave
   ═══════════════════════════════════════════════════════════ */
static void applyIPCMessage(const IPC_Message* msg, TravelerState* states,
                            int numTravelers, Vector2* positions, int milestone,
                            Scheduler* sched) {
    for (int i = 0; i < numTravelers; i++) {
        if (states[i].pid != msg->child_pid) continue;

        if (msg->status == STATUS_SCHEDULE_REQUEST && sched) {  /* [M7] */
            scheduler_on_request(sched, msg->next_node, msg->child_pid, i,
                                 msg->remaining_cost, msg->priority);
            break;
        }
        else if (msg->status == STATUS_ARRIVED_DEST) {
            if (sched) scheduler_on_leave(sched, msg->current_node);
            states[i].isFinished  = 1;
            states[i].isActive    = 0;
            states[i].animState   = ANIM_DONE;
            states[i].currentNode = msg->current_node;
            states[i].entPos      = positions[msg->current_node];
            printf("[PID=%d] arrived at node %d | DESTINATION\n", msg->child_pid, msg->current_node);
            fflush(stdout);
        }
        else if (msg->status == STATUS_WAITING_FOR_NODE && milestone >= 6) {  /* [M6-M7] */
            states[i].currentNode = msg->current_node;
            states[i].nextNode    = msg->next_node;
            states[i].isActive    = 0;
            states[i].animState   = ANIM_WAITING;
            states[i].entPos      = outsideNodePos(positions,
                                                    msg->current_node,
                                                    msg->next_node);
            printf("[PID=%d] waiting outside node %d\n", msg->child_pid, msg->next_node);
            fflush(stdout);
        }
        else if (msg->status == STATUS_INSIDE_NODE) {
#if MILESTONE == 6
            for (int j = 0; j < numTravelers; j++) {
                if (states[j].nextNode == msg->current_node && states[j].animState == ANIM_PAUSING) {
                    states[j].animState = ANIM_MOVING;
                    states[j].isActive = 1;
                }
            }
#endif
            if (sched) scheduler_on_enter(sched, msg->current_node);

            states[i].currentNode = msg->current_node;
            states[i].nextNode    = msg->next_node;
            states[i].isActive    = 0;
            states[i].animState   = ANIM_PAUSING;
            states[i].entPos      = positions[msg->current_node];
            printf("[PID=%d] arrived at node %d | next node: %d\n",
                   msg->child_pid, msg->current_node, msg->next_node);
            fflush(stdout);
        }
        else if (msg->status == STATUS_DRIVING) {
            if (sched) scheduler_on_leave(sched, msg->current_node);
            if (states[i].animState == ANIM_MOVING &&
                states[i].currentNode == msg->current_node &&
                states[i].nextNode == msg->next_node) {
                break;
            }
            states[i].currentNode = msg->current_node;
            states[i].nextNode    = msg->next_node;
            states[i].isActive    = 1;
            states[i].animState   = ANIM_MOVING;
            states[i].timer       = 0.0f;
            states[i].subJump     = 0;
        }
        break;
    }
}

static void drainIPCPipe(int pipe_fd, TravelerState* states, int numTravelers,
                         Vector2* positions, int milestone, Scheduler* sched) {
    IPC_Message msg;
    while (read(pipe_fd, &msg, sizeof(msg)) == (ssize_t)sizeof(IPC_Message))
        applyIPCMessage(&msg, states, numTravelers, positions, milestone, sched);
}

/* ── [M5-M7] STOP/PLAY — SIGSTOP/SIGCONT לילדים ── */
static void setChildrenPaused(TravelerState* states, int numTravelers, int pause) {
    for (int i = 0; i < numTravelers; i++) {
        if (states[i].isFinished) continue;
        kill(states[i].pid, pause ? SIGSTOP : SIGCONT);
    }
}

/* ── [M2+] buildPositions / drawGraph — ציור הגרף ── */

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
   [M2-M3] visualizeGraph — נוסע בודד, PLAY/STOP, אנימציה
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
   [M4-M7] visualizeMultiTravelers — נוסעים מרובים
     M4 — האב מניע אנימציה (path מוכן מראש)
     M5 — IPC מהילדים
     M6 — WAIT מחוץ לצומת
     M7 — תצוגת FCFS/SJF
   ═══════════════════════════════════════════════════════════ */

void visualizeMultiTravelers(void* g_ptr, TravelerState* states,
                             int numTravelers, int pipe_fd, Scheduler* sched) {
    Graph* graph = (Graph*)g_ptr;
    const int SCR_W = 800, SCR_H = 600;

#ifndef MILESTONE
    int milestone = 6;
#else
    int milestone = MILESTONE;
#endif

    InitWindow(SCR_W, SCR_H,
               (milestone == 7 && sched)
                   ? TextFormat("OS Graph Motion Sim - Milestone 7 (%s)",
                                sched_policy_name(sched->policy))
                   : TextFormat("OS Graph Motion Sim - Milestone %d", milestone));
    SetTargetFPS(60);

    Vector2* positions = buildPositions(graph, SCR_W, SCR_H);

    // Place each traveler at its start node
    for (int i = 0; i < numTravelers; i++)
        states[i].entPos = positions[states[i].currentNode];

    int playing     = (milestone >= 5) ? 1 : 0;
    int allFinished = 0;
    Rectangle btn   = { 20, SCR_H - 50, 120, 32 };

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        /* ── Button Control ── */
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mouse = GetMousePosition();
            if (CheckCollisionPointRec(mouse, btn)) {
                if (allFinished) {
                    break; // Smooth GUI exit when simulation is DONE
                }
                playing = !playing;
                if (milestone >= 5) {
                    if (!playing) {
                        setChildrenPaused(states, numTravelers, 1);
                        drainIPCPipe(pipe_fd, states, numTravelers, positions, milestone, sched);
                    } else {
                        setChildrenPaused(states, numTravelers, 0);
                    }
                } else if (milestone == 4 && playing) {
                    for (int i = 0; i < numTravelers; i++)
                        if (states[i].animState == ANIM_IDLE && states[i].pathLen > 1) {
                            states[i].animState = ANIM_MOVING;
                            states[i].isActive  = 1;
                        }
                }
            }
        }

        allFinished = 1;
        for (int i = 0; i < numTravelers; i++) {
            if (!states[i].isFinished) allFinished = 0;
        }

        if (playing) {
            /* ── Read IPC Messages from Pipe ── */
            if (milestone >= 5)
                drainIPCPipe(pipe_fd, states, numTravelers, positions, milestone, sched);

            if (milestone == 7 && sched)
                scheduler_poll(sched);

            /* ── Animation Update Loop ── */
            for (int i = 0; i < numTravelers; i++) {
                if (states[i].isFinished) continue;

                if (milestone == 4) {
                    /* ── [M4] אנימציה מונעת הורה ── */
                    if (states[i].animState == ANIM_MOVING && states[i].pathLen > 1) {
                        int from = states[i].path[states[i].edgeIdx];
                        int to   = states[i].path[states[i].edgeIdx + 1];
                        int w    = findEdgeWeight(graph, from, to);

                        states[i].timer += dt;
                        float jp = states[i].timer / JUMP_DURATION;
                        if (jp > 1.0f) jp = 1.0f;
                        states[i].entPos = lerpV2(positions[from], positions[to], ((float)states[i].subJump + jp) / (float)w);

                        if (states[i].timer >= JUMP_DURATION) {
                            states[i].timer -= JUMP_DURATION;
                            states[i].subJump++;
                            if (states[i].subJump >= w) {
                                states[i].subJump     = 0;
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
                    /* ── [M5-M7] אנימציה מונעת IPC מהילדים ── */
                    if (states[i].isActive && states[i].animState == ANIM_MOVING) {
                        int from = states[i].currentNode;
                        int to   = states[i].nextNode;
                        int w    = findEdgeWeight(graph, from, to);

                        states[i].timer += dt;
                        float jp = states[i].timer / JUMP_DURATION;
                        if (jp > 1.0f) jp = 1.0f;

                        float edgeT = ((float)states[i].subJump + jp) / (float)w;
                        Vector2 edgeTarget = (milestone >= 6)  /* [M6-M7] עצירה מחוץ לצומת */
                            ? outsideNodePos(positions, from, to)
                            : positions[to];

                        if (edgeT >= 1.0f) {
                            states[i].isActive = 0;
                            states[i].entPos   = edgeTarget;
                            if (milestone < 6)
                                states[i].currentNode = to;
                        } else {
                            states[i].entPos = lerpV2(positions[from], edgeTarget, edgeT);
                            if (states[i].timer >= JUMP_DURATION) {
                                states[i].timer -= JUMP_DURATION;
                                states[i].subJump++;
                            }
                        }
                    }
                }
            }
        }

        /* ── Drawing Screen ── */
        BeginDrawing();
        ClearBackground(RAYWHITE);

        drawGraph(graph, positions, NULL, 0);

        int nodeCounts[graph->numNodes];
        int nodeNextSlot[graph->numNodes];
        memset(nodeCounts, 0, (size_t)graph->numNodes * sizeof(int));
        for (int i = 0; i < numTravelers; i++) {
            if (isTravelerInsideNode(&states[i]))
                nodeCounts[states[i].currentNode]++;
        }
        memset(nodeNextSlot, 0, (size_t)graph->numNodes * sizeof(int));

        // Draw Travelers
        for (int i = 0; i < numTravelers; i++) {
            Color c = states[i].color;
            Vector2 drawPos = states[i].entPos;

            if (states[i].isFinished) {
                c = Fade(states[i].color, 0.4f);
            }
            else if (states[i].animState == ANIM_WAITING) {
                c = Fade(states[i].color,
                         ((int)(GetTime() * 4) % 2 == 0) ? 1.0f : 0.55f);
                drawPos = states[i].entPos;
            }
            else if (isTravelerInsideNode(&states[i])) {
                int node = states[i].currentNode;
                int slot = nodeNextSlot[node]++;
                drawPos = spreadAtNode(positions[node], slot, nodeCounts[node]);
            }

            DrawCircleV(drawPos, 12, c);
            DrawCircleLines((int)drawPos.x, (int)drawPos.y, 12, DARKGRAY);

            if (isTravelerInsideNode(&states[i])) {
                const char* idText = TextFormat("%d", i + 1);
                int textW = MeasureText(idText, 12);
                DrawText(idText, (int)drawPos.x - textW / 2, (int)drawPos.y - 6, 12, RAYWHITE);
            }

            if (states[i].animState == ANIM_WAITING) {
                const char* waitText = TextFormat("T%d (PID:%d) WAIT", i + 1, states[i].pid);
                int textW = MeasureText(waitText, 11);

                DrawRectangle((int)drawPos.x - textW/2 - 4, (int)drawPos.y - 26, textW + 8, 14, Fade(RAYWHITE, 0.85f));
                DrawRectangleLines((int)drawPos.x - textW/2 - 4, (int)drawPos.y - 26, textW + 8, 14, MAROON);
                DrawText(waitText, (int)drawPos.x - textW/2, (int)drawPos.y - 24, 11, MAROON);
            }
        }

        // Action Button Display
        Color btnColor = allFinished ? DARKBLUE : (playing ? RED : GREEN);
        DrawRectangleRec(btn, btnColor);
        DrawRectangleLinesEx(btn, 2, DARKGRAY);
        const char* btnLabel = allFinished ? "DONE" : (playing ? "STOP" : "PLAY");
        int bw = MeasureText(btnLabel, 18);
        DrawText(btnLabel, (int)(btn.x + (btn.width - bw) / 2), (int)(btn.y + (btn.height - 18) / 2), 18, WHITE);

        DrawText(TextFormat("OS Graph Motion Sim - Milestone %d", milestone), 20, 15, 18, DARKGRAY);
        if (milestone == 7 && sched)
            DrawText(TextFormat("Node scheduler: %s", sched_policy_name(sched->policy)),
                     20, 42, 14, DARKBLUE);

        EndDrawing();
    }

    free(positions);
    CloseWindow();
}