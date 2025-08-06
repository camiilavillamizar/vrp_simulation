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
    int* total_wood_collected, int* total_gold_collected, int* total_food_collected,
    int claimed[MAP_HEIGHT][MAP_WIDTH],
    int tick
) {
    int left = VILLAGER_CAPACITY;
    int collectionEffort = 0;
    int start_x = vx[tid], start_y = vy[tid];

    while (left > 0) {
        int needed_wood, needed_gold, needed_food;
        #pragma omp critical
        {
            needed_wood = (*total_wood_collected < GOAL_WOOD);
            needed_gold = (*total_gold_collected < GOAL_GOLD);
            needed_food = (*total_food_collected < GOAL_FOOD);
        }

        printf("[Tick %d][Villager %d] Needs → Wood: %d, Gold: %d, Food: %d\n",
               tick, tid, needed_wood, needed_gold, needed_food);

        if (!needed_wood && !needed_gold && !needed_food) {
            printf("[Tick %d][Villager %d] All goals met, skipping turn.\n", tick, tid);
            break;
        }

        int best_x = -1, best_y = -1, best_type = -1, take = 0;
        double best_score = -1e9;

        for (int y = 0; y < game_map.height; ++y) {
            for (int x = 0; x < game_map.width; ++x) {
                Resource *res = &game_map.resources[y][x];
                if (res->amount > 0 && !claimed[y][x]) {
                    if ((res->type == CELL_WOOD && needed_wood) ||
                        (res->type == CELL_GOLD && needed_gold) ||
                        (res->type == CELL_FOOD && needed_food)) {

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
                            best_x = x;
                            best_y = y;
                            best_type = res->type;
                            take = can_take;
                        }
                    }
                }
            }
        }

        if (best_x == -1) {
            printf("[Tick %d][Villager %d] No suitable resource found.\n", tick, tid);
            break;
        }

        if (take == 0) {
            printf("[Tick %d][Villager %d] Found resource but can't take any.\n", tick, tid);
            break;
        }

        claimed[best_y][best_x] = 1;

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
                if (target->type == CELL_WOOD)
                    game_map.cells[best_y][best_x] = CELL_WOOD_EMPTY;
                else if (target->type == CELL_FOOD)
                    game_map.cells[best_y][best_x] = CELL_FOOD_EMPTY;
                else if (target->type == CELL_GOLD)
                    game_map.cells[best_y][best_x] = CELL_GOLD_EMPTY;
            }

            collectionEffort += canreally * (
                (best_type == CELL_GOLD) ? TICKS_PER_GOLD_UNIT :
                (best_type == CELL_WOOD) ? TICKS_PER_WOOD_UNIT :
                                           TICKS_PER_FOOD_UNIT
            );
            got = canreally;
            left -= canreally;

            if (best_type == CELL_WOOD) *total_wood_collected += canreally;
            else if (best_type == CELL_GOLD) *total_gold_collected += canreally;
            else if (best_type == CELL_FOOD) *total_food_collected += canreally;
        }

        printf("[Tick %d][Villager %d] Collected %d of %s at (%d, %d)\n",
               tick, tid, got,
               (best_type == CELL_WOOD) ? "wood" :
               (best_type == CELL_GOLD) ? "gold" : "food",
               best_x, best_y);

        int act_idx = action_counts[tid]++;
        tick_actions[tid][act_idx].villager_id = tid;
        tick_actions[tid][act_idx].action_idx = act_idx;
        tick_actions[tid][act_idx].resource_type = best_type;
        tick_actions[tid][act_idx].x = best_x;
        tick_actions[tid][act_idx].y = best_y;
        tick_actions[tid][act_idx].amount = got;

        *dist_sum += go_to + back;
        vx[tid] = drop_x;
        vy[tid] = drop_y;
        start_x = drop_x;
        start_y = drop_y;
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

    printf("[Rank %d] Starting simulation with strategy %d...\n", mpi_rank, strategy_id);
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

    *total_collectionEffort = 0;
    *used_ticks = 0;
    *total_distance = 0;
    double t_start = MPI_Wtime();

    while (!finished) {
        printf("[Tick %d] START → Wood: %d | Gold: %d | Food: %d\n",
            tick, total_wood_local, total_gold_local, total_food_local);

        int round_collectionEffort = 0;
        VillagerAction tick_actions[NUMBER_OF_VILLAGERS][VILLAGER_CAPACITY] = {{{0}}};
        int action_counts[NUMBER_OF_VILLAGERS] = {0};
        int claimed[MAP_HEIGHT][MAP_WIDTH] = {{0}};

        #pragma omp parallel for reduction(+:round_collectionEffort)
        for (int tid = 0; tid < NUMBER_OF_VILLAGERS; ++tid) {
            int t = villager_collect_knapsack(
                tid, villager_x, villager_y, &dist_sum[tid], strategy_id,
                tick_actions, action_counts,
                &total_wood_local, &total_gold_local, &total_food_local,
                claimed,
                tick
            );
            round_collectionEffort += t;
        }

        total_collectionEffort_local += round_collectionEffort;

        save_tick_state(FOLDER, strategy_id, tick, villager_x, villager_y, tick_actions, action_counts);
        save_map_txt_with_villagers("output/ticks", strategy_id, tick, villager_x, villager_y);
        save_tick_json_state("output/simulation", strategy_id, tick, villager_x, villager_y,
                     total_wood_local, total_gold_local, total_food_local,
                     total_distance, total_collectionEffort_local,
                     tick_actions, action_counts);

        printf("[Tick %d] END → Wood: %d | Gold: %d | Food: %d\n",
            tick, total_wood_local, total_gold_local, total_food_local);

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
