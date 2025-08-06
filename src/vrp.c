#include "VRP.h"
#include "game_rules.h"
#include "status_report.h"
#include "map.h"
#include "villager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <math.h>
#include <omp.h>
#include <mpi.h>

#define FOLDER "output/logs"
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
    VillagerAction tick_actions[NUMBER_OF_VILLAGERS][VILLAGER_CAPACITY],
    int action_counts[NUMBER_OF_VILLAGERS]
) {
    struct stat st = {0};
    if (stat(folder, &st) == -1) mkdir(folder, 0700);
    char filename[256];
    snprintf(filename, sizeof(filename), "%s/strategy%d_tick%03d.txt", folder, strategy_id, tick);
    FILE *f = fopen(filename, "w");
    if (!f) return;

    fprintf(f, "Tick %d\nVillagers (final position):\n", tick);
    for (int i = 0; i < NUMBER_OF_VILLAGERS; ++i)
        fprintf(f, "V%d: (%d,%d)\n", i, villager_x[i], villager_y[i]);

    fprintf(f, "Collection Log This Tick:\n");
    for (int i = 0; i < NUMBER_OF_VILLAGERS; ++i) {
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
    VillagerAction tick_actions[NUMBER_OF_VILLAGERS][VILLAGER_CAPACITY],
    int action_counts[NUMBER_OF_VILLAGERS],
    int* wood_collected, int* gold_collected, int* food_collected
) {
    int left = VILLAGER_CAPACITY;
    int collectionEffort = 0;
    int start_x = vx[tid], start_y = vy[tid];

    while (left > 0) {
        int best_x = -1, best_y = -1, best_type = -1, take = 0;
        double best_score = -1e9;

        if (selector == 2) {
            // Estrategia 2: K Nearest + más cantidad
            const int K = 5;
            int kx[K], ky[K], kd[K], kn = 0;

            for (int y = 0; y < game_map.height; ++y) {
                for (int x = 0; x < game_map.width; ++x) {
                    Resource *res = &game_map.resources[y][x];
                    if (res->amount > 0 &&
                        (res->type == CELL_GOLD || res->type == CELL_WOOD || res->type == CELL_FOOD)) {

                        int d = manhattan(start_x, start_y, x, y);
                        if (kn < K) {
                            kx[kn] = x; ky[kn] = y; kd[kn] = d; kn++;
                        } else {
                            int max_idx = 0;
                            for (int j = 1; j < K; ++j)
                                if (kd[j] > kd[max_idx]) max_idx = j;
                            if (d < kd[max_idx]) {
                                kx[max_idx] = x; ky[max_idx] = y; kd[max_idx] = d;
                            }
                        }
                    }
                }
            }

            int max_amt = -1;
            for (int j = 0; j < kn; ++j) {
                Resource *res = &game_map.resources[ky[j]][kx[j]];
                if (res->amount > max_amt) {
                    max_amt = res->amount;
                    best_x = kx[j]; best_y = ky[j]; best_type = res->type;
                }
            }

            if (max_amt > 0) {
                take = (max_amt > left) ? left : max_amt;
            }

        } else {
            // Estrategias 0 y 1
            for (int y = 0; y < game_map.height; ++y) {
                for (int x = 0; x < game_map.width; ++x) {
                    Resource *res = &game_map.resources[y][x];
                    if (res->amount > 0 &&
                        (res->type == CELL_GOLD || res->type == CELL_WOOD || res->type == CELL_FOOD)) {

                        int t_per = (res->type == CELL_GOLD) ? TICKS_PER_GOLD_UNIT :
                                    (res->type == CELL_WOOD) ? TICKS_PER_WOOD_UNIT :
                                                               TICKS_PER_FOOD_UNIT;

                        int d = manhattan(start_x, start_y, x, y);
                        int can_take = (res->amount > left) ? left : res->amount;
                        if (can_take == 0) continue;

                        double score = (selector == 0) ? -d :
                                       ((double)(can_take * t_per)) / (d + 1);

                        if (score > best_score) {
                            best_score = score;
                            best_x = x; best_y = y;
                            best_type = res->type;
                            take = can_take;
                        }
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

            if (target->amount == 0) {
                if(target->type == CELL_WOOD){

                    game_map.cells[best_y][best_x] = CELL_WOOD_EMPTY; 

                } else if (target->type == CELL_FOOD){
                    game_map.cells[best_y][best_x] = CELL_FOOD_EMPTY; 

                } else if (target->type == CELL_GOLD){
                    game_map.cells[best_y][best_x] = CELL_GOLD_EMPTY; 

                }
                 
            }

            collectionEffort += canreally * (
                (best_type == CELL_GOLD) ? TICKS_PER_GOLD_UNIT :
                (best_type == CELL_WOOD) ? TICKS_PER_WOOD_UNIT :
                                           TICKS_PER_FOOD_UNIT
            );
            got = canreally;
            left -= canreally;

            if (best_type == CELL_WOOD) *wood_collected += canreally;
            else if (best_type == CELL_GOLD) *gold_collected += canreally;
            else if (best_type == CELL_FOOD) *food_collected += canreally;
        }

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

    return collectionEffort;
}

// Simulation loop with logging per tick
void run_strategy_simulation(
    int strategy_id,
    int *total_collectionEffort,
    int *used_ticks,
    long long *total_distance,
    int mpi_rank
) {
    int tick = 0, finished = 0;
    int villager_x[NUMBER_OF_VILLAGERS], villager_y[NUMBER_OF_VILLAGERS];
    for (int i = 0; i < NUMBER_OF_VILLAGERS; ++i) {
        villager_x[i] = villagers[i].x;
        villager_y[i] = villagers[i].y;
    }

    int dist_sum[NUMBER_OF_VILLAGERS] = {0};
    int total_wood_local = 0;
    int total_gold_local = 0;
    int total_food_local = 0;
    int total_collectionEffort_local = 0;

    // int drop_x, drop_y;
    // nearest_dropoff(game_map.width / 2, game_map.height / 2, &drop_x, &drop_y);
    // for (int i = 0; i < NUMBER_OF_VILLAGERS; ++i) {
    //     villager_x[i] = drop_x;
    //     villager_y[i] = drop_y;
    // }

    *total_collectionEffort = 0;
    *used_ticks = 0;
    *total_distance = 0;
    double t_start = MPI_Wtime();

    while (!finished) {
        int round_collectionEffort = 0;
        VillagerAction tick_actions[NUMBER_OF_VILLAGERS][VILLAGER_CAPACITY] = {{{0}}};
        int action_counts[NUMBER_OF_VILLAGERS] = {0};

        #pragma omp parallel for reduction(+:round_collectionEffort, total_wood_local, total_gold_local, total_food_local)
        for (int tid = 0; tid < NUMBER_OF_VILLAGERS; ++tid) {
            int wood = 0, gold = 0, food = 0;

            int t = villager_collect_knapsack(
                tid, villager_x, villager_y, &dist_sum[tid], strategy_id,
                tick_actions, action_counts,
                &wood, &gold, &food
            );

            round_collectionEffort += t;
            total_wood_local += wood;
            total_gold_local += gold;
            total_food_local += food;
        }

        total_collectionEffort_local += round_collectionEffort;

        save_tick_state(FOLDER, strategy_id, tick, villager_x, villager_y, tick_actions, action_counts);

        save_map_txt_with_villagers("output/ticks", strategy_id, tick, villager_x, villager_y);
        save_tick_json_state("output/simulation", strategy_id, tick, villager_x, villager_y,
                     total_wood_local, total_gold_local, total_food_local, 
                     total_distance, total_collectionEffort_local,
                     tick_actions, action_counts);



        tick++;

        if (total_wood_local >= GOAL_WOOD &&
            total_gold_local >= GOAL_GOLD &&
            total_food_local >= GOAL_FOOD) {
            finished = 1;
        }
    }

    double t_end = MPI_Wtime();
    *used_ticks = tick;
    for (int i = 0; i < NUMBER_OF_VILLAGERS; ++i)
        *total_distance += dist_sum[i];
    *total_collectionEffort = total_collectionEffort_local;

    printf("[Rank %d] Finished: Ticks=%d, Time=%.3fs, collectionEffort=%d, TotalDist=%lld\n",
    mpi_rank, *used_ticks, t_end - t_start, *total_collectionEffort, *total_distance);
    printf("[Rank %d] Resources Collected → Wood: %d | Gold: %d | Food: %d\n",
        mpi_rank, total_wood_local, total_gold_local, total_food_local);
}
