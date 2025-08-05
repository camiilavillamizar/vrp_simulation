#include "VRP.h"
#include "game_rules.h"
#include "map.h"
#include "villager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <math.h>
#include <omp.h>
#include <mpi.h>

#define FOLDER "output/ticks"
#define MAX_DROPOFF 100
#define K_NEAREST 5

// Manhattan distance helper
static inline int manhattan(int x1, int y1, int x2, int y2) {
    return abs(x1-x2) + abs(y1-y2);
}

// Find all drop-off points on the map
void find_all_dropoffs(int *xs, int *ys, int *n) {
    *n = 0;
    for (int y = 0; y < game_map.height; ++y)
        for (int x = 0; x < game_map.width; ++x)
            if (game_map.cells[y][x] == CELL_DROP_OFF || game_map.cells[y][x] == CELL_TOWN_CENTER) {
                xs[*n] = x;
                ys[*n] = y;
                (*n)++;
                if (*n >= MAX_DROPOFF) return;
            }
}

// Find the nearest drop-off from a given point
void nearest_dropoff(int from_x, int from_y, int *drop_x, int *drop_y) {
    int xs[MAX_DROPOFF], ys[MAX_DROPOFF], n;
    find_all_dropoffs(xs, ys, &n);
    int min_dist = 1e9, best = 0;
    for (int i = 0; i < n; ++i) {
        int d = manhattan(from_x, from_y, xs[i], ys[i]);
        if (d < min_dist) { min_dist = d; best = i; }
    }
    *drop_x = xs[best]; *drop_y = ys[best];
}

// Save tick state with full per-villager action log
void save_tick_state(
    const char *folder, int strategy_id, int tick,
    int villager_x[], int villager_y[],
    VillagerAction tick_actions[MAX_VILLAGERS][VILLAGER_CAPACITY],
    int action_counts[MAX_VILLAGERS]
) {
    struct stat st = {0};
    if (stat(folder, &st) == -1) mkdir(folder, 0700);
    char filename[256];
    snprintf(filename, sizeof(filename), "%s/strategy%d_tick%03d.txt", folder, strategy_id, tick);
    FILE *f = fopen(filename, "w");
    if (!f) return;

    fprintf(f, "Tick %d\nVillagers (final position):\n", tick);
    for (int i = 0; i < MAX_VILLAGERS; ++i)
        fprintf(f, "V%d: (%d,%d)\n", i, villager_x[i], villager_y[i]);

    fprintf(f, "Collection Log This Tick:\n");
    for (int i = 0; i < MAX_VILLAGERS; ++i) {
        for (int j = 0; j < action_counts[i]; ++j) {
            VillagerAction* act = &tick_actions[i][j];
            const char* rtype = (act->resource_type == CELL_GOLD ? "GOLD" :
                                (act->resource_type == CELL_FOOD ? "FOOD" :
                                (act->resource_type == CELL_WOOD ? "WOOD" : "UNKNOWN")));
            fprintf(f, "V%d action %d: %s at (%d,%d), amount=%d\n",
                act->villager_id, act->action_idx,
                rtype, act->x, act->y, act->amount);
        }
    }

    fprintf(f, "Resources:\n");
    for (int y = 0; y < game_map.height; ++y)
        for (int x = 0; x < game_map.width; ++x)
            if (game_map.resources[y][x].amount > 0)
                fprintf(f, "Res(%d) @(%d,%d): amt=%d\n",
                    game_map.resources[y][x].type, x, y, game_map.resources[y][x].amount);
    fclose(f);
}

// Main "knapsack" collection routine for a single villager, with action log
int villager_collect_knapsack(
    int tid, int *vx, int *vy, int *dist_sum, int selector,
    VillagerAction tick_actions[MAX_VILLAGERS][VILLAGER_CAPACITY],
    int action_counts[MAX_VILLAGERS]
) {
    int left = VILLAGER_CAPACITY;
    int ticket = 0;
    int start_x = vx[tid], start_y = vy[tid];

    while (left > 0) {
        int best_x = -1, best_y = -1, best_type = -1, take = 0;
        double best_score = -1e9;

        for (int y = 0; y < game_map.height; ++y) {
            for (int x = 0; x < game_map.width; ++x) {
                Resource *res = &game_map.resources[y][x];
                if (res->amount > 0) {
                    int t_per = 0;
                    if (res->type == CELL_GOLD) t_per = TICKET_PER_GOLD;
                    else if (res->type == CELL_WOOD) t_per = TICKET_PER_WOOD;
                    else if (res->type == CELL_FOOD) t_per = TICKET_PER_FOOD;
                    else continue;
                    int d = manhattan(start_x, start_y, x, y);
                    int can_take = (res->amount > left) ? left : res->amount;
                    if (can_take == 0) continue;
                    double score = 0.0;
                    if (selector == 0) score = -d; // Greedy: nearest
                    else if (selector == 1) score = (double)(can_take * t_per) / (d+1); // Max profit per distance
                    else if (selector == 2) {
                        // KNN: find among K nearest, take max capacity
                        static int kx[K_NEAREST], ky[K_NEAREST], kd[K_NEAREST], kn = 0;
                        kn = 0;
                        for (int yy = 0; yy < game_map.height; ++yy) for (int xx = 0; xx < game_map.width; ++xx) {
                            Resource *r = &game_map.resources[yy][xx];
                            if (r->amount > 0 && (r->type == res->type)) {
                                int dd = manhattan(start_x, start_y, xx, yy);
                                if (kn < K_NEAREST) { kx[kn]=xx; ky[kn]=yy; kd[kn]=dd; kn++; }
                                else {
                                    int max_idx = 0;
                                    for (int j = 1; j < K_NEAREST; ++j) if (kd[j] > kd[max_idx]) max_idx = j;
                                    if (dd < kd[max_idx]) { kx[max_idx]=xx; ky[max_idx]=yy; kd[max_idx]=dd; }
                                }
                            }
                        }
                        int max_cap = -1;
                        for (int j = 0; j < kn; ++j) {
                            Resource *rr = &game_map.resources[ky[j]][kx[j]];
                            if (rr->amount > max_cap) { max_cap = rr->amount; best_x = kx[j]; best_y = ky[j]; best_type = res->type; }
                        }
                        if (max_cap > 0) { take = (max_cap > left) ? left : max_cap; }
                        break;
                    }
                    if (selector != 2 && score > best_score) {
                        best_score = score;
                        best_x = x; best_y = y; best_type = res->type;
                        take = can_take;
                    }
                }
            }
        }
        if (best_x == -1 || take == 0) break;

        int drop_x, drop_y;
        nearest_dropoff(best_x, best_y, &drop_x, &drop_y);
        int go_to = manhattan(start_x, start_y, best_x, best_y);
        int back = manhattan(best_x, best_y, drop_x, drop_y);

        int got = 0;
        #pragma omp critical
        {
            Resource *target = &game_map.resources[best_y][best_x];
            int canreally = (target->amount > take) ? take : target->amount;
            target->amount -= canreally;
            ticket += canreally * ((best_type==CELL_GOLD)?TICKET_PER_GOLD : (best_type==CELL_WOOD)?TICKET_PER_WOOD : TICKET_PER_FOOD);
            got = canreally;
            left -= canreally;
        }
        // Log action for this tick
        int act_idx = action_counts[tid]++;
        tick_actions[tid][act_idx].villager_id = tid;
        tick_actions[tid][act_idx].action_idx = act_idx;
        tick_actions[tid][act_idx].resource_type = best_type;
        tick_actions[tid][act_idx].x = best_x;
        tick_actions[tid][act_idx].y = best_y;
        tick_actions[tid][act_idx].amount = got;

        *dist_sum += go_to + back;
        vx[tid] = drop_x; vy[tid] = drop_y;
        start_x = drop_x; start_y = drop_y;
    }
    return ticket;
}

// Simulation loop with logging per tick
void run_strategy_simulation(
    int strategy_id,
    int *total_ticket,
    int *used_ticks,
    long long *total_distance,
    int mpi_rank
) {
    int tick = 0, finished = 0;
    int villager_x[MAX_VILLAGERS], villager_y[MAX_VILLAGERS];
    int dist_sum[MAX_VILLAGERS] = {0};
    int total_wood_local = 0;
    int total_gold_local = 0;
    int total_food_local = 0;

    int drop_x, drop_y;
    nearest_dropoff(game_map.width / 2, game_map.height / 2, &drop_x, &drop_y);
    for (int i = 0; i < MAX_VILLAGERS; ++i) {
        villager_x[i] = drop_x; villager_y[i] = drop_y;
    }

    *total_ticket = 0; *used_ticks = 0; *total_distance = 0;
    double t_start = MPI_Wtime();

    while (!finished) {
        int round_ticket = 0;
        VillagerAction tick_actions[MAX_VILLAGERS][VILLAGER_CAPACITY] = {{{0}}};
        int action_counts[MAX_VILLAGERS] = {0};

        #pragma omp parallel for reduction(+:round_ticket)
        for (int tid = 0; tid < MAX_VILLAGERS; ++tid) {
            int t = villager_collect_knapsack(
                tid, villager_x, villager_y, &dist_sum[tid], strategy_id,
                tick_actions, action_counts
            );
            round_ticket += t;
        }
        total_ticket_local += round_ticket;

        save_tick_state(FOLDER, strategy_id, tick, villager_x, villager_y, tick_actions, action_counts);
        tick++;

        if (total_wood_local >= GOAL_WOOD &&
            total_gold_local >= GOAL_GOLD &&
            total_food_local >= GOAL_FOOD) {
            finished = 1;
}
    }
    double t_end = MPI_Wtime();
    *used_ticks = tick;
    for (int i = 0; i < MAX_VILLAGERS; ++i)
        *total_distance += dist_sum[i];
    *total_ticket = total_ticket_local;
    printf("[Rank %d] Finished: Ticks=%d, Time=%.3fs, Ticket=%d, TotalDist=%lld\n",
        mpi_rank, *used_ticks, t_end-t_start, *total_ticket, *total_distance);
}