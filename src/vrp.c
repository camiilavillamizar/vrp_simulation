#include "vrp.h"
#include <limits.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <omp.h>
#include <time.h>
#include <assert.h>

// ==== Check if a path is continuous (no jumps between steps) ====
int is_path_continuous(const Path *p) {
    for (int i = 1; i < p->length; ++i) {
        int dx = abs(p->x[i] - p->x[i-1]);
        int dy = abs(p->y[i] - p->y[i-1]);
        if (!((dx == 1 && dy == 0) || (dx == 0 && dy == 1)))
            return 0;
    }
    return 1;
}

// ==== Simple Queue implementation for BFS ====
typedef struct QueueNode { int x, y; struct QueueNode *next; } QueueNode;
typedef struct { QueueNode *front, *rear; } Queue;

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
void free_queue(Queue *q) { int x, y; while (!is_queue_empty(q)) dequeue(q, &x, &y); free(q); }

typedef struct { int x, y; } Coord;

// ==== BFS: Find shortest path from (sx,sy) to (tx,ty) ====
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
            path->x[len] = px;
            path->y[len] = py;
            len++;
            Coord p = prev[py * MAP_WIDTH + px];
            px = p.x;
            py = p.y;
        }
        // Reverse path to get it from start to end
        for (int i = 0; i < len / 2; i++) {
            int t = path->x[i]; path->x[i] = path->x[len - 1 - i]; path->x[len - 1 - i] = t;
            t = path->y[i]; path->y[i] = path->y[len - 1 - i]; path->y[len - 1 - i] = t;
        }
        path->length = len;
        // Optionally, you can assert continuity here for debugging
    } else {
        path->length = 0;
    }
    free(visited);
    free(prev);
    return found;
}

// ==== Merge two paths into one, skipping duplicate points ====
void merge_path(const Path *p1, const Path *p2, Path *result) {
    int i, start = 0;
    result->length = 0;
    // Copy all points from the first path
    for (i = 0; i < p1->length && result->length < MAX_PATH_LEN; i++) {
        result->x[result->length] = p1->x[i];
        result->y[result->length] = p1->y[i];
        result->length++;
    }
    // If last point of p1 equals first point of p2, skip the first point of p2
    if (p1->length > 0 && p2->length > 0 &&
        p1->x[p1->length-1] == p2->x[0] && p1->y[p1->length-1] == p2->y[0]) start = 1;
    // Append remaining points from p2
    for (i = start; i < p2->length && result->length < MAX_PATH_LEN; i++) {
        if (result->length > 0 &&
            result->x[result->length-1] == p2->x[i] &&
            result->y[result->length-1] == p2->y[i]) continue;
        result->x[result->length] = p2->x[i];
        result->y[result->length] = p2->y[i];
        result->length++;
    }
    // For robustness, check continuity but never abort
    if (!is_path_continuous(result)) {
        fprintf(stderr, "[WARNING] Discontinuity in merged path\n");
        result->length = 0; // Mark as invalid
    }
}

// ==== next_permutation utility ====
int next_permutation(int *a, int n) {
    int i = n-2;
    while(i>=0 && a[i]>=a[i+1]) i--;
    if(i<0) return 0;
    int j=n-1;
    while(a[j]<=a[i]) j--;
    int t=a[i]; a[i]=a[j]; a[j]=t;
    for(int l=i+1,r=n-1;l<r;l++,r--) { t=a[l]; a[l]=a[r]; a[r]=t; }
    return 1;
}

// ==== Find nearest resource ====
int find_nearest_resource(int map[MAP_HEIGHT][MAP_WIDTH], int sx, int sy, int target_type, int *used_flag, Path *out_path, int *out_x, int *out_y) {
    int min_dist = INT_MAX, found = 0;
    Path best_path = {0};
    for (int y = 0; y < MAP_HEIGHT; y++)
    for (int x = 0; x < MAP_WIDTH; x++) {
        if (map[y][x] == target_type && (!used_flag || !used_flag[y * MAP_WIDTH + x])) {
            Path p = {0};
            if (!find_path(map, sx, sy, x, y, &p)) continue;
            if (p.length < 2) continue;
            int dist = p.length - 1;
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

// ==== Find nearest dropoff (includes town center) ====
int find_nearest_dropoff(int map[MAP_HEIGHT][MAP_WIDTH], int sx, int sy, Path *out_path, int *out_x, int *out_y) {
    int min_dist = INT_MAX, found = 0;
    Path best_path = {0};
    for (int y = 0; y < MAP_HEIGHT; y++)
    for (int x = 0; x < MAP_WIDTH; x++) {
        if (map[y][x] == CELL_DROP_OFF || map[y][x] == CELL_TOWN_CENTER) {
            Path p = {0};
            if (!find_path(map, sx, sy, x, y, &p)) continue;
            if (p.length < 2) continue;
            int dist = p.length - 1;
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

void swap_int(int *a, int *b) { int t = *a; *a = *b; *b = t; }
float frand() { return rand()/(float)RAND_MAX; }

// ==== Greedy Resource Collection: always use last path endpoint as next BFS start ====
StrategyResult assign_task_greedy_nearest(Map *map, Villager *villagers, int num_villagers, Path *villager_paths) {
    StrategyResult result = {0, 0};
    #pragma omp parallel for
    for (int i = 0; i < num_villagers; i++) {
        int left_types[3] = {CELL_GOLD, CELL_FOOD, CELL_WOOD};
        int used_types[3] = {0, 0, 0};
        int *used_flag = (int*)calloc(MAP_HEIGHT * MAP_WIDTH, sizeof(int));
        int cur_x = villagers[i].x, cur_y = villagers[i].y, total_dist = 0;
        Path path_accum = {0};
        int fail = 0;
        for (int step = 0; step < 3; step++) {
            int min_d = INT_MAX, min_idx = -1, tx = -1, ty = -1;
            Path tmp = {0};
            // Search for the nearest *unused* resource from the current position
            for (int j = 0; j < 3; j++) {
                if (used_types[j]) continue;
                int dummy_x, dummy_y;
                int d = find_nearest_resource(map->cells, cur_x, cur_y, left_types[j], used_flag, &tmp, &dummy_x, &dummy_y);
                if (d >= 0 && d < min_d) {
                    min_d = d; min_idx = j; tx = dummy_x; ty = dummy_y;
                }
            }
            if (min_idx == -1) { fail=1; break; }
            used_types[min_idx] = 1; used_flag[ty * MAP_WIDTH + tx] = 1;
            // Always start BFS from current position, so merge is always continuous
            if (path_accum.length == 0)
                path_accum = tmp;
            else {
                Path new_path = {0};
                merge_path(&path_accum, &tmp, &new_path);
                path_accum = new_path;
            }
            if (!is_path_continuous(&path_accum)) {
                fprintf(stderr, "[WARNING] Discontinuous path for villager %d at step %d, skipping villager\n", i, step);
                fail = 1; break;
            }
            cur_x = tx; cur_y = ty;
            total_dist += (tmp.length ? tmp.length-1 : 0);
        }
        // Find path to nearest dropoff (from last collected resource)
        if (!fail) {
            int dropoff_x, dropoff_y;
            Path to_drop = {0};
            int d = find_nearest_dropoff(map->cells, cur_x, cur_y, &to_drop, &dropoff_x, &dropoff_y);
            if (d < 0) fail=1;
            else {
                Path new_path = {0};
                merge_path(&path_accum, &to_drop, &new_path);
                path_accum = new_path;
                total_dist += (to_drop.length ? to_drop.length-1 : 0);
                if (!is_path_continuous(&path_accum)) {
                    fprintf(stderr, "[WARNING] Discontinuous path for villager %d after dropoff\n", i);
                    fail = 1;
                } else {
                    villager_paths[i] = path_accum;
                    result.total_ticks += total_dist;
                    result.total_collected += VILLAGER_CAPACITY * 3;
                }
            }
        }
        free(used_flag);
        if (fail) villager_paths[i].length = 0; // Mark invalid
    }
    return result;
}

// ==== Brute-force optimal resource collection by permutation (always continuous) ====
StrategyResult assign_task_optimal_permutation(Map *map, Villager *villagers, int num_villagers, Path *villager_paths) {
    StrategyResult result = {0, 0};
    #pragma omp parallel for
    for (int i = 0; i < num_villagers; i++) {
        int order[3] = {CELL_GOLD, CELL_FOOD, CELL_WOOD}, best_order[3] = {CELL_GOLD, CELL_FOOD, CELL_WOOD};
        int min_dist = INT_MAX; Path best_path = {0};
        do {
            int cur_x = villagers[i].x, cur_y = villagers[i].y, total_dist = 0;
            int used_flag[MAP_HEIGHT * MAP_WIDTH] = {0};
            Path path_accum = {0}; int valid = 1;
            for (int j = 0; j < 3; j++) {
                int tx, ty; Path tmp = {0};
                int d = find_nearest_resource(map->cells, cur_x, cur_y, order[j], used_flag, &tmp, &tx, &ty);
                if (d < 0) { valid = 0; break; }
                used_flag[ty * MAP_WIDTH + tx] = 1;
                if (path_accum.length == 0) path_accum = tmp;
                else {
                    Path new_path = {0};
                    merge_path(&path_accum, &tmp, &new_path);
                    if (!is_path_continuous(&new_path)) { valid = 0; break; }
                    path_accum = new_path;
                }
                cur_x = tx; cur_y = ty;
                total_dist += (tmp.length ? tmp.length-1 : 0);
            }
            int dropoff_x, dropoff_y; Path to_drop = {0};
            int d = find_nearest_dropoff(map->cells, cur_x, cur_y, &to_drop, &dropoff_x, &dropoff_y);
            if (d < 0) valid = 0;
            else {
                Path new_path = {0};
                merge_path(&path_accum, &to_drop, &new_path);
                if (!is_path_continuous(&new_path)) valid = 0;
                else {
                    path_accum = new_path;
                    total_dist += (to_drop.length ? to_drop.length-1 : 0);
                }
            }
            if (valid && total_dist < min_dist) {
                min_dist = total_dist;
                memcpy(best_order, order, sizeof(order));
                best_path = path_accum;
            }
        } while (next_permutation(order, 3));
        villager_paths[i] = best_path;
        result.total_ticks += min_dist;
        result.total_collected += VILLAGER_CAPACITY * 3;
    }
    return result;
}

// ==== Simulated Annealing fallback: here we use permutation for robustness ====
StrategyResult assign_task_simulated_annealing(Map *map, Villager *villagers, int num_villagers, Path *villager_paths) {
    return assign_task_optimal_permutation(map, villagers, num_villagers, villager_paths);
}

// ==== Export all villager paths to a JSON file ====
void export_paths_to_json(Path *villager_paths, int num_villagers, const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "Failed to open file %s for writing\n", filename);
        return;
    }
    fprintf(fp, "{\n  \"villagers\": [\n");
    for (int i = 0; i < num_villagers; i++) {
        fprintf(fp, "    { \"id\": %d, \"length\": %d, \"path\": [", i, villager_paths[i].length);
        for (int j = 0; j < villager_paths[i].length; j++) {
            fprintf(fp, "[%d,%d]", villager_paths[i].x[j], villager_paths[i].y[j]);
            if (j != villager_paths[i].length - 1) fprintf(fp, ", ");
        }
        fprintf(fp, "] }%s\n", (i == num_villagers-1 ? "" : ","));
    }
    fprintf(fp, "  ]\n}\n");
    fclose(fp);
    printf("[Export] Saved villager paths to %s\n", filename);
}