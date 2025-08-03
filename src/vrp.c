#include "vrp.h"
#include <limits.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <omp.h>
#include <time.h>

//====================== Queue for BFS ======================
typedef struct QueueNode {
    int x, y;
    struct QueueNode *next;
} QueueNode;

typedef struct {
    QueueNode *front, *rear;
} Queue;

Queue *create_queue() {
    Queue *q = (Queue *)malloc(sizeof(Queue));
    q->front = q->rear = NULL;
    return q;
}
void enqueue(Queue *q, int x, int y) {
    QueueNode *node = (QueueNode *)malloc(sizeof(QueueNode));
    node->x = x; node->y = y; node->next = NULL;
    if (q->rear) q->rear->next = node;
    else q->front = node;
    q->rear = node;
}
void dequeue(Queue *q, int *x, int *y) {
    if (!q->front) { *x = *y = -1; return; }
    QueueNode *tmp = q->front;
    *x = tmp->x; *y = tmp->y;
    q->front = q->front->next;
    if (!q->front) q->rear = NULL;
    free(tmp);
}
int is_queue_empty(Queue *q) { return q->front == NULL; }
void free_queue(Queue *q) {
    int x, y;
    while (!is_queue_empty(q)) dequeue(q, &x, &y);
    free(q);
}

//================== BFS Shortest Path ======================
typedef struct { int x, y; } Coord;

// Find shortest path from (sx,sy) to (tx,ty). Returns 1 if found, 0 otherwise.
// All large arrays are heap-allocated and properly freed.
int find_path(int map[MAP_HEIGHT][MAP_WIDTH], int sx, int sy, int tx, int ty, Path *path) {
    int *visited = (int *)calloc(MAP_WIDTH * MAP_HEIGHT, sizeof(int));
    Coord *prev = (Coord *)malloc(MAP_WIDTH * MAP_HEIGHT * sizeof(Coord));
    if (!visited || !prev) {
        fprintf(stderr, "find_path: out of memory\n");
        if (visited) free(visited);
        if (prev) free(prev);
        path->length = 0; return 0;
    }
    Queue *q = create_queue(); enqueue(q, sx, sy);
    visited[sy * MAP_WIDTH + sx] = 1; prev[sy * MAP_WIDTH + sx] = (Coord){-1, -1};
    int found = 0, dx[] = {1, -1, 0, 0}, dy[] = {0, 0, 1, -1};
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
        // Reverse path in-place
        for (int i = 0; i < len / 2; i++) {
            int t = path->x[i]; path->x[i] = path->x[len - 1 - i]; path->x[len - 1 - i] = t;
            t = path->y[i]; path->y[i] = path->y[len - 1 - i]; path->y[len - 1 - i] = t;
        }
        path->length = len;
    } else path->length = 0;
    free(visited); free(prev);
    return found;
}

//================= VRP Helper Functions ====================

// Find the nearest resource of target_type from (sx,sy) that's not already used.
// All heap-allocated arrays are properly managed.
int find_nearest_resource(int map[MAP_HEIGHT][MAP_WIDTH], int sx, int sy, int target_type, int *used_flag, Path *out_path, int *out_x, int *out_y) {
    int min_dist = INT_MAX, found = 0;
    Path best_path = {0};
    for (int y = 0; y < MAP_HEIGHT; y++) for (int x = 0; x < MAP_WIDTH; x++) {
        if (map[y][x] == target_type && (!used_flag || !used_flag[y * MAP_WIDTH + x])) {
            Path p = {0};
            if (!find_path(map, sx, sy, x, y, &p)) continue;
            int dist = p.length ? (p.length - 1) : INT_MAX;
            if (dist < min_dist) {
                min_dist = dist; best_path = p;
                if (out_x) *out_x = x;
                if (out_y) *out_y = y;
                found = 1;
            }
        }
    }
    if (found) { if (out_path) *out_path = best_path; return min_dist; }
    return -1;
}

// Find the nearest dropoff from (sx,sy). Same memory safety.
int find_nearest_dropoff(int map[MAP_HEIGHT][MAP_WIDTH], int sx, int sy, Path *out_path, int *out_x, int *out_y) {
    int min_dist = INT_MAX, found = 0;
    Path best_path = {0};
    for (int y = 0; y < MAP_HEIGHT; y++) for (int x = 0; x < MAP_WIDTH; x++) {
        if (map[y][x] == CELL_DROP_OFF || map[y][x] == CELL_TOWN_CENTER) {
            Path p = {0};
            if (!find_path(map, sx, sy, x, y, &p)) continue;
            int dist = p.length ? (p.length - 1) : INT_MAX;
            if (dist < min_dist) {
                min_dist = dist; best_path = p;
                if (out_x) *out_x = x;
                if (out_y) *out_y = y;
                found = 1;
            }
        }
    }
    if (found) { if (out_path) *out_path = best_path; return min_dist; }
    return -1;
}

// Merge two paths (skip duplicate start). Checks path length bounds.
void merge_path(const Path *p1, const Path *p2, Path *result) {
    int i;
    for (i = 0; i < p1->length && result->length < MAX_PATH_LEN; i++) {
        result->x[result->length] = p1->x[i];
        result->y[result->length] = p1->y[i];
        result->length++;
    }
    for (i = 1; i < p2->length && result->length < MAX_PATH_LEN; i++) {
        result->x[result->length] = p2->x[i];
        result->y[result->length] = p2->y[i];
        result->length++;
    }
}
void swap_int(int *a, int *b) { int t = *a; *a = *b; *b = t; }
float frand() { return rand()/(float)RAND_MAX; }

//========== 1. Brute-force Optimal Permutation ===========
void permute(int *types, int n, int *best_order, int *min_dist, Map *map, int sx, int sy, Path *best_path) {
    if (n == 1) {
        int cur_x = sx, cur_y = sy;
        int *used_flag = (int*)calloc(MAP_HEIGHT * MAP_WIDTH, sizeof(int));
        int total_dist = 0, valid = 1;
        Path *path_acc = (Path*)calloc(1, sizeof(Path));
        Path *temp_path = (Path*)calloc(1, sizeof(Path));
        for (int i = 0; i < 3; i++) {
            int tx, ty;
            int d = find_nearest_resource(map->cells, cur_x, cur_y, types[i], used_flag, temp_path, &tx, &ty);
            if (d < 0) { valid = 0; break; }
            used_flag[ty * MAP_WIDTH + tx] = 1;
            merge_path(path_acc, temp_path, path_acc);
            total_dist += (temp_path->length ? temp_path->length - 1 : 0);
            cur_x = tx; cur_y = ty;
        }
        int dropoff_x, dropoff_y;
        Path to_drop = {0};
        int d = find_nearest_dropoff(map->cells, cur_x, cur_y, &to_drop, &dropoff_x, &dropoff_y);
        if (d < 0) valid = 0;
        merge_path(path_acc, &to_drop, path_acc);
        total_dist += (to_drop.length ? to_drop.length - 1 : 0);
        if (valid && total_dist < *min_dist) {
            *min_dist = total_dist;
            memcpy(best_order, types, 3 * sizeof(int));
            *best_path = *path_acc;
        }
        free(used_flag); free(path_acc); free(temp_path);
        return;
    }
    for (int i = 0; i < n; i++) {
        permute(types, n-1, best_order, min_dist, map, sx, sy, best_path);
        if (n % 2 == 0) swap_int(&types[i], &types[n-1]);
        else swap_int(&types[0], &types[n-1]);
    }
}

// Assigns resource collection using brute-force optimal permutation for all villagers.
StrategyResult assign_task_optimal_permutation(Map *map, Villager *villagers, int num_villagers, Path *villager_paths) {
    StrategyResult result = {0, 0};
    #pragma omp parallel for
    for (int i = 0; i < num_villagers; i++) {
        int arr[3] = {CELL_GOLD, CELL_FOOD, CELL_WOOD};
        int best_order[3], min_dist = INT_MAX;
        Path best_path = {0};
        permute(arr, 3, best_order, &min_dist, map, villagers[i].x, villagers[i].y, &best_path);
        villager_paths[i] = best_path;
        result.total_ticks += min_dist;
        result.total_collected += VILLAGER_CAPACITY * 3;
        printf("[OPTIMAL] Villager %d order: [%d %d %d], path length: %d\n", i, best_order[0], best_order[1], best_order[2], best_path.length);
    }
    return result;
}

//========== 2. Greedy Nearest Neighbor ==========
StrategyResult assign_task_greedy_nearest(Map *map, Villager *villagers, int num_villagers, Path *villager_paths) {
    StrategyResult result = {0, 0};
    #pragma omp parallel for
    for (int i = 0; i < num_villagers; i++) {
        int left_types[3] = {CELL_GOLD, CELL_FOOD, CELL_WOOD};
        int used_types[3] = {0, 0, 0};
        int *used_flag = (int*)calloc(MAP_HEIGHT * MAP_WIDTH, sizeof(int));
        int cur_x = villagers[i].x, cur_y = villagers[i].y, total_dist = 0;
        Path *total_path = (Path*)calloc(1, sizeof(Path));
        Path *temp_path = (Path*)calloc(1, sizeof(Path));
        int fail = 0;
        for (int step = 0; step < 3; step++) {
            int min_d = INT_MAX, min_idx = -1, tx = -1, ty = -1;
            for (int j = 0; j < 3; j++) {
                if (used_types[j]) continue;
                int dummy_x, dummy_y;
                int d = find_nearest_resource(map->cells, cur_x, cur_y, left_types[j], used_flag, NULL, &dummy_x, &dummy_y);
                if (d >= 0 && d < min_d) {
                    min_d = d; min_idx = j; tx = dummy_x; ty = dummy_y;
                }
            }
            if (min_idx == -1) { fail=1; break; }
            used_types[min_idx] = 1; used_flag[ty * MAP_WIDTH + tx] = 1;
            find_nearest_resource(map->cells, cur_x, cur_y, left_types[min_idx], used_flag, temp_path, &tx, &ty);
            merge_path(total_path, temp_path, total_path);
            cur_x = tx; cur_y = ty;
            total_dist += (temp_path->length ? temp_path->length-1 : 0);
        }
        if (!fail) {
            int dropoff_x, dropoff_y;
            Path to_drop = {0};
            int d = find_nearest_dropoff(map->cells, cur_x, cur_y, &to_drop, &dropoff_x, &dropoff_y);
            if (d < 0) fail=1;
            else {
                merge_path(total_path, &to_drop, total_path);
                total_dist += (to_drop.length ? to_drop.length-1 : 0);
                villager_paths[i] = *total_path;
                result.total_ticks += total_dist;
                result.total_collected += VILLAGER_CAPACITY * 3;
            }
        }
        free(used_flag); free(total_path); free(temp_path);
    }
    return result;
}

//========== 3. Simulated Annealing ==========
StrategyResult assign_task_simulated_annealing(Map *map, Villager *villagers, int num_villagers, Path *villager_paths) {
    StrategyResult result = {0, 0};
    #pragma omp parallel for
    for (int i = 0; i < num_villagers; i++) {
        int seq[3] = {CELL_GOLD, CELL_FOOD, CELL_WOOD}, best_seq[3];
        int min_dist = INT_MAX;
        Path best_path = {0};
        float T = 100.0, Tmin = 0.01, alpha = 0.98;
        unsigned int seed = (unsigned int)(time(NULL) + i * 12345);
        while (T > Tmin) {
            int a = rand_r(&seed) % 3, b = rand_r(&seed) % 3;
            if (a != b) swap_int(&seq[a], &seq[b]);
            int cur_x = villagers[i].x, cur_y = villagers[i].y, valid = 1, total_dist = 0;
            int *used_flag = (int*)calloc(MAP_HEIGHT * MAP_WIDTH, sizeof(int));
            Path *path_acc = (Path*)calloc(1, sizeof(Path));
            Path *temp_path = (Path*)calloc(1, sizeof(Path));
            for (int j = 0; j < 3; j++) {
                int tx, ty;
                int d = find_nearest_resource(map->cells, cur_x, cur_y, seq[j], used_flag, temp_path, &tx, &ty);
                if (d < 0) { valid = 0; break; }
                used_flag[ty * MAP_WIDTH + tx] = 1;
                merge_path(path_acc, temp_path, path_acc);
                total_dist += (temp_path->length ? temp_path->length-1 : 0);
                cur_x = tx; cur_y = ty;
            }
            int dropoff_x, dropoff_y;
            Path to_drop = {0};
            int d = find_nearest_dropoff(map->cells, cur_x, cur_y, &to_drop, &dropoff_x, &dropoff_y);
            if (d < 0) valid = 0;
            merge_path(path_acc, &to_drop, path_acc);
            total_dist += (to_drop.length ? to_drop.length-1 : 0);
            if (valid && (total_dist < min_dist || frand() < exp((min_dist-total_dist)/T))) {
                min_dist = total_dist; memcpy(best_seq, seq, 3*sizeof(int)); best_path = *path_acc;
            }
            T *= alpha;
            free(used_flag); free(path_acc); free(temp_path);
        }
        villager_paths[i] = best_path;
        result.total_ticks += min_dist;
        result.total_collected += VILLAGER_CAPACITY * 3;
        printf("[SA] Villager %d best order: [%d %d %d], path len: %d\n", i, best_seq[0], best_seq[1], best_seq[2], best_path.length);
    }
    return result;
}