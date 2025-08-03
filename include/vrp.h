#ifndef VRP_H
#define VRP_H

#include "game_rules.h"
#include "map.h"
#include "villager.h"
#include <mpi.h>
#define MAX_PATH_LEN 4096

typedef struct {
  int x[MAX_PATH_LEN];
  int y[MAX_PATH_LEN];
  int length;
} Path;

#define STRATEGY_GREEDY_PATH           0
#define STRATEGY_MAX_PROFIT            1
#define STRATEGY_STAGE_CONCENTRATION   2
#define STRATEGY_REGION_SCHEDULING     3

typedef struct {
  int total_collected;
  int total_ticks;
} StrategyResult;

typedef struct {
  int x, y;
} Position;

typedef struct {
  Position target;
  int type; // CELL_WOOD, CELL_GOLD, CELL_FOOD
} Task;

int find_path(int map[MAP_HEIGHT][MAP_WIDTH], int sx, int sy, int tx, int ty, Path *path);
// Strategy function declarations
StrategyResult assign_task_optimal_permutation(Map *map, Villager *villagers, int num_villagers, Path *villager_paths);
StrategyResult assign_task_greedy_nearest(Map *map, Villager *villagers, int num_villagers, Path *villager_paths);
StrategyResult assign_task_simulated_annealing(Map *map, Villager *villagers, int num_villagers, Path *villager_paths);
// StrategyResult assign_task_region_based(Map *map, Villager *villagers, int num_villagers, Path *villager_paths);

#endif