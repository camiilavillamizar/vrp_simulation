#ifndef VRP_H
#define VRP_H

#include "game_rules.h"
#include "map.h"
#include "villager.h"
#include <mpi.h>

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

// Strategy function declarations
StrategyResult assign_task_greedy(Map *map, Villager *villagers, int num_villagers);
StrategyResult assign_task_max_profit(Map *map, Villager *villagers, int num_villagers);
StrategyResult assign_task_stage_based(Map *map, Villager *villagers, int num_villagers);
StrategyResult assign_task_region_based(Map *map, Villager *villagers, int num_villagers);

#endif