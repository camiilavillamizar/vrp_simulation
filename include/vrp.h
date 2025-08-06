#ifndef VRP_H
#define VRP_H

#include "map.h"

typedef struct {
  int strategy_id;
  int total_collection_effort;
  int used_ticks;
  long long total_distance;
} StrategyResult;

// Log for each villager's action in a tick
typedef struct {
  int villager_id;      // Villager ID
  int action_idx;       // The order of resource collection in this tick (0,1,...)
  int resource_type;    // CELL_GOLD, CELL_WOOD, CELL_FOOD
  int x, y;             // Resource coordinate
  int amount;           // Amount collected this action
} VillagerAction;

void run_strategy_simulation(
  int strategy_id,
  int *total_collection_effort,
  int *used_ticks,
  long long *total_distance,
  int mpi_rank
);

void save_tick_state(
    const char *folder, int strategy_id, int tick,
    int villager_x[], int villager_y[],
    VillagerAction tick_actions[NUMBER_OF_VILLAGERS][VILLAGER_CAPACITY],
    int action_counts[NUMBER_OF_VILLAGERS]
);

#endif