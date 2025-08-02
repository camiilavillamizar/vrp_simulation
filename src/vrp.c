#include "vrp.h"
#include <limits.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include <omp.h>
#include <stdio.h>
#include <string.h> // for memset

// ===========================================
// Queue Implementation for BFS Pathfinding
// ===========================================

typedef struct QueueNode {
    int x, y;
    struct QueueNode *next;
} QueueNode;

typedef struct {
    QueueNode *front;
    QueueNode *rear;
} Queue;

Queue *create_queue() {
    Queue *q = (Queue *)malloc(sizeof(Queue));
    q->front = q->rear = NULL;
    return q;
}

void enqueue(Queue *q, int x, int y) {
    QueueNode *node = (QueueNode *)malloc(sizeof(QueueNode));
    node->x = x;
    node->y = y;
    node->next = NULL;
    if (q->rear) {
        q->rear->next = node;
    } else {
        q->front = node;
    }
    q->rear = node;
}

void dequeue(Queue *q, int *x, int *y) {
    if (!q->front) {
        *x = *y = -1;
        return;
    }
    QueueNode *temp = q->front;
    *x = temp->x;
    *y = temp->y;
    q->front = q->front->next;
    if (!q->front)
        q->rear = NULL;
    free(temp);
}

int is_queue_empty(Queue *q) {
    return q->front == NULL;
}

void free_queue(Queue *q) {
    int x, y;
    while (!is_queue_empty(q)) {
        dequeue(q, &x, &y);
    }
    free(q);
}

// ===========================================
// Pathfinding and Reachability (SAFE FOR LARGE MAP)
// ===========================================

typedef struct { int x, y; } Coord;

int find_path(int map[MAP_HEIGHT][MAP_WIDTH], int sx, int sy, int tx, int ty, Path *path) {
    int *visited = (int *)calloc(MAP_WIDTH * MAP_HEIGHT, sizeof(int));
    Coord *prev = (Coord *)malloc(MAP_WIDTH * MAP_HEIGHT * sizeof(Coord));
    if (!visited || !prev) {
        fprintf(stderr, "find_path: out of memory\n");
        if (visited) free(visited);
        if (prev) free(prev);
        path->length = 0;
        return 0;
    }
    Queue *q = create_queue();
    enqueue(q, sx, sy);
    visited[sy * MAP_WIDTH + sx] = 1;
    prev[sy * MAP_WIDTH + sx] = (Coord){-1, -1};

    int found = 0;
    int dx[] = {1, -1, 0, 0}, dy[] = {0, 0, 1, -1};
    while (!is_queue_empty(q)) {
        int cx, cy; dequeue(q, &cx, &cy);
        for (int d = 0; d < 4; d++) {
            int nx = cx + dx[d], ny = cy + dy[d];
            if (nx < 0 || nx >= MAP_WIDTH || ny < 0 || ny >= MAP_HEIGHT) continue;
            if (visited[ny * MAP_WIDTH + nx]) continue;
            if (map[ny][nx] > CELL_TITAN_GATE) continue;
            prev[ny * MAP_WIDTH + nx] = (Coord){cx, cy};
            visited[ny * MAP_WIDTH + nx] = 1;
            enqueue(q, nx, ny);
            if (nx == tx && ny == ty) { found = 1; goto done; }
        }
    }
done:
    free_queue(q);
    if (found) {
        int px = tx, py = ty, len = 0;
        while (px != -1 && py != -1 && len < MAX_PATH_LEN) {
            path->x[len] = px; path->y[len] = py; len++;
            Coord p = prev[py * MAP_WIDTH + px]; px = p.x; py = p.y;
        }
        for (int i = 0; i < len / 2; i++) {
            int t = path->x[i]; path->x[i] = path->x[len - 1 - i]; path->x[len - 1 - i] = t;
            t = path->y[i]; path->y[i] = path->y[len - 1 - i]; path->y[len - 1 - i] = t;
        }
        path->length = len;
    } else {
        path->length = 0;
    }
    free(visited);
    free(prev);
    return found;
}

// ===========================================
// Strategy Implementations (unchanged logic, now safe)
// ===========================================

StrategyResult assign_task_greedy(Map *map, Villager *villagers, int num_villagers, Path *villager_paths) {
    StrategyResult result = {0, 0};
    int total_ticks = 0, total_collected = 0;
    #pragma omp parallel for reduction(+:total_ticks, total_collected)
    for (int i = 0; i < num_villagers; i++) {
        Task best_task = {{-1, -1}, CELL_EMPTY};
        int min_dist = INT_MAX, found = 0;
        Path best_path = {0};
        for (int y = 0; y < MAP_HEIGHT; y++) for (int x = 0; x < MAP_WIDTH; x++) {
            int cell = map->cells[y][x];
            if (cell != CELL_WOOD && cell != CELL_FOOD && cell != CELL_GOLD) continue;
            Path p = {0};
            if (!find_path(map->cells, villagers[i].x, villagers[i].y, x, y, &p)) continue;
            int dist = p.length ? (p.length - 1) : INT_MAX;
            if (dist < min_dist) {
                min_dist = dist;
                best_task.target.x = x; best_task.target.y = y; best_task.type = cell;
                best_path = p; found = 1;
            }
        }
        if (found && best_task.type != CELL_EMPTY) {
            int gather_ticks = 0;
            switch (best_task.type) {
                case CELL_WOOD: gather_ticks = GATHER_TICK_PER_WOOD * VILLAGER_CAPACITY; break;
                case CELL_FOOD: gather_ticks = GATHER_TICK_PER_FOOD * VILLAGER_CAPACITY; break;
                case CELL_GOLD: gather_ticks = GATHER_TICK_PER_GOLD * VILLAGER_CAPACITY; break;
            }
            total_ticks += min_dist + gather_ticks;
            total_collected += VILLAGER_CAPACITY;
            villager_paths[i] = best_path;
        } else {
            villager_paths[i].length = 0;
        }
    }
    result.total_ticks = total_ticks;
    result.total_collected = total_collected;
    return result;
}

StrategyResult assign_task_max_profit(Map *map, Villager *villagers, int num_villagers, Path *villager_paths) {
    StrategyResult result = {0, 0};
    int total_ticks = 0, total_collected = 0;
    #pragma omp parallel for reduction(+:total_ticks, total_collected)
    for (int i = 0; i < num_villagers; i++) {
        Task best_task = {{-1, -1}, CELL_EMPTY};
        float max_score = -INFINITY;
        Path best_path = {0};
        int found = 0;
        for (int y = 0; y < MAP_HEIGHT; y++) for (int x = 0; x < MAP_WIDTH; x++) {
            int cell = map->cells[y][x], gather_ticks_per_unit = 0;
            switch (cell) {
                case CELL_WOOD: gather_ticks_per_unit = GATHER_TICK_PER_WOOD; break;
                case CELL_FOOD: gather_ticks_per_unit = GATHER_TICK_PER_FOOD; break;
                case CELL_GOLD: gather_ticks_per_unit = GATHER_TICK_PER_GOLD; break;
                default: continue;
            }
            Path p = {0};
            if (!find_path(map->cells, villagers[i].x, villagers[i].y, x, y, &p)) continue;
            int dist = p.length ? (p.length - 1) : INT_MAX;
            int t_ticks = dist + gather_ticks_per_unit * VILLAGER_CAPACITY;
            if (t_ticks <= 0) continue;
            float score = (float)VILLAGER_CAPACITY / t_ticks;
            if (score > max_score) {
                max_score = score;
                best_task.target.x = x; best_task.target.y = y; best_task.type = cell;
                best_path = p; found = 1;
            }
        }
        if (found && best_task.type != CELL_EMPTY) {
            int gather_ticks = 0;
            switch (best_task.type) {
                case CELL_WOOD: gather_ticks = GATHER_TICK_PER_WOOD * VILLAGER_CAPACITY; break;
                case CELL_FOOD: gather_ticks = GATHER_TICK_PER_FOOD * VILLAGER_CAPACITY; break;
                case CELL_GOLD: gather_ticks = GATHER_TICK_PER_GOLD * VILLAGER_CAPACITY; break;
            }
            total_ticks += (best_path.length ? best_path.length - 1 : 0) + gather_ticks;
            total_collected += VILLAGER_CAPACITY;
            villager_paths[i] = best_path;
        } else {
            villager_paths[i].length = 0;
        }
    }
    result.total_ticks = total_ticks;
    result.total_collected = total_collected;
    return result;
}

StrategyResult assign_task_stage_based(Map *map, Villager *villagers, int num_villagers, Path *villager_paths) {
    StrategyResult result = {0, 0};
    int total_ticks = 0, total_collected = 0;
    #pragma omp parallel for reduction(+:total_ticks, total_collected)
    for (int i = 0; i < num_villagers; i++) {
        int preferred_resource = CELL_WOOD;
        Task best_task = {{-1, -1}, CELL_EMPTY};
        int min_dist = INT_MAX, found = 0;
        Path best_path = {0};
        for (int y = 0; y < MAP_HEIGHT; y++) for (int x = 0; x < MAP_WIDTH; x++) {
            if (map->cells[y][x] != preferred_resource) continue;
            Path p = {0};
            if (!find_path(map->cells, villagers[i].x, villagers[i].y, x, y, &p)) continue;
            int dist = p.length ? (p.length - 1) : INT_MAX;
            if (dist < min_dist) {
                min_dist = dist;
                best_task.target.x = x; best_task.target.y = y; best_task.type = preferred_resource;
                best_path = p; found = 1;
            }
        }
        if (!found) {
            for (int y = 0; y < MAP_HEIGHT; y++) for (int x = 0; x < MAP_WIDTH; x++) {
                int cell = map->cells[y][x];
                if (cell != CELL_WOOD && cell != CELL_FOOD && cell != CELL_GOLD) continue;
                Path p = {0};
                if (!find_path(map->cells, villagers[i].x, villagers[i].y, x, y, &p)) continue;
                int dist = p.length ? (p.length - 1) : INT_MAX;
                if (dist < min_dist) {
                    min_dist = dist;
                    best_task.target.x = x; best_task.target.y = y; best_task.type = cell;
                    best_path = p; found = 1;
                }
            }
        }
        if (found && best_task.type != CELL_EMPTY) {
            int gather_ticks = 0;
            switch (best_task.type) {
                case CELL_WOOD: gather_ticks = GATHER_TICK_PER_WOOD * VILLAGER_CAPACITY; break;
                case CELL_FOOD: gather_ticks = GATHER_TICK_PER_FOOD * VILLAGER_CAPACITY; break;
                case CELL_GOLD: gather_ticks = GATHER_TICK_PER_GOLD * VILLAGER_CAPACITY; break;
            }
            total_ticks += min_dist + gather_ticks;
            total_collected += VILLAGER_CAPACITY;
            villager_paths[i] = best_path;
        } else {
            villager_paths[i].length = 0;
        }
    }
    result.total_ticks = total_ticks;
    result.total_collected = total_collected;
    return result;
}

StrategyResult assign_task_region_based(Map *map, Villager *villagers, int num_villagers, Path *villager_paths) {
    StrategyResult result = {0, 0};
    int total_ticks = 0, total_collected = 0;
    const int region_width = MAP_WIDTH / 2, region_height = MAP_HEIGHT / 2;
    #pragma omp parallel for reduction(+:total_ticks, total_collected)
    for (int i = 0; i < num_villagers; i++) {
        int region_x = villagers[i].x / region_width;
        int region_y = villagers[i].y / region_height;
        int x_min = region_x * region_width, x_max = x_min + region_width;
        int y_min = region_y * region_height, y_max = y_min + region_height;
        // 修正边界，防止越界
        if (x_max > MAP_WIDTH) x_max = MAP_WIDTH;
        if (y_max > MAP_HEIGHT) y_max = MAP_HEIGHT;
        Task best_task = {{-1, -1}, CELL_EMPTY};
        int min_dist = INT_MAX, found = 0;
        Path best_path = {0};
        // Search for resources in the villager's region
        for (int y = y_min; y < y_max; y++) for (int x = x_min; x < x_max; x++) {
            int cell = map->cells[y][x];
            if (cell != CELL_WOOD && cell != CELL_FOOD && cell != CELL_GOLD) continue;
            Path p = {0};
            if (!find_path(map->cells, villagers[i].x, villagers[i].y, x, y, &p)) continue;
            int dist = p.length ? (p.length - 1) : INT_MAX;
            if (dist < min_dist) {
                min_dist = dist;
                best_task.target.x = x; best_task.target.y = y; best_task.type = cell;
                best_path = p; found = 1;
            }
        }
        // If no resources found in region, search entire map
        if (!found) {
            for (int y = 0; y < MAP_HEIGHT; y++) for (int x = 0; x < MAP_WIDTH; x++) {
                int cell = map->cells[y][x];
                if (cell != CELL_WOOD && cell != CELL_FOOD && cell != CELL_GOLD) continue;
                Path p = {0};
                if (!find_path(map->cells, villagers[i].x, villagers[i].y, x, y, &p)) continue;
                int dist = p.length ? (p.length - 1) : INT_MAX;
                if (dist < min_dist) {
                    min_dist = dist;
                    best_task.target.x = x; best_task.target.y = y; best_task.type = cell;
                    best_path = p; found = 1;
                }
            }
        }
        if (found && best_task.type != CELL_EMPTY) {
            int gather_ticks = 0;
            switch (best_task.type) {
                case CELL_WOOD: gather_ticks = GATHER_TICK_PER_WOOD * VILLAGER_CAPACITY; break;
                case CELL_FOOD: gather_ticks = GATHER_TICK_PER_FOOD * VILLAGER_CAPACITY; break;
                case CELL_GOLD: gather_ticks = GATHER_TICK_PER_GOLD * VILLAGER_CAPACITY; break;
            }
            total_ticks += min_dist + gather_ticks;
            total_collected += VILLAGER_CAPACITY;
            villager_paths[i] = best_path;
        } else {
            villager_paths[i].length = 0;
        }
    }
    result.total_ticks = total_ticks;
    result.total_collected = total_collected;
    return result;
}