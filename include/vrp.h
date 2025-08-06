#ifndef VRP_H
#define VRP_H

#include "map.h"
#include "simulation.h"
#include "villager.h"

// Constants
#define FOLDER "output/logs"
#define MAX_DROPOFF 100
#define K_NEAREST 5


// Finds all drop-off locations in the map
void find_all_dropoffs(int *xs, int *ys, int *n);

// Finds the nearest drop-off from a given point
void nearest_dropoff(int from_x, int from_y, int *drop_x, int *drop_y);

// Saves all villager actions and resource states for the current tick
void save_tick_state(
    const char *folder, int strategy_id, int tick,
    int villager_x[], int villager_y[],
    VillagerAction tick_actions[NUMBER_OF_VILLAGERS][VILLAGER_CAPACITY],
    int action_counts[NUMBER_OF_VILLAGERS]
);

// Main resource collection logic for a single villager
int villager_collect_knapsack(
    int tid, int *vx, int *vy, int *dist_sum, int selector,
    VillagerAction tick_actions[NUMBER_OF_VILLAGERS][VILLAGER_CAPACITY],
    int action_counts[NUMBER_OF_VILLAGERS],
    int* total_wood_collected, int* total_gold_collected, int* total_food_collected,
    int claimed[MAP_HEIGHT][MAP_WIDTH],
    int tick
);

// Runs the simulation using the selected strategy and logs output
void run_strategy_simulation(
    int strategy_id,
    int *total_collectionEffort,
    int *used_ticks,
    long long *total_distance,
    int mpi_rank
);

#endif