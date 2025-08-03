#ifndef GAME_RULES_H
#define GAME_RULES_H

// ===== TO MAP CELL VALUES =====
#define CELL_EMPTY         0   //Nothing
#define CELL_GOLD          1   //Gold resource
#define CELL_FOOD          2   //Food resource
#define CELL_WOOD          3   //Wood resource
#define CELL_DROP_OFF      4   //Drop-off point
#define CELL_TOWN_CENTER   5   //Town center (starting building)
#define CELL_ENEMY_BUILD   6   //Enemy building (not used for now)
#define CELL_FRIENDLY_BLD  7   //Friendly building

#define CELL_VILLAGER     -1   //Villager (may be rendered separately)

// ===== VILLAGER LOGIC =====
#define MAX_VILLAGERS         25
#define INITIAL_VILLAGERS      3

// ===== SIMULATION SETTINGS =====
#define MAP_WIDTH               1000
#define MAP_HEIGHT              1000
#define TICK_SLEEP_US          10000  //10ms between ticks for visual pacing (optional)

// ===== RESOURCE NODES CAPACITY =====
#define TREE_CAPACITY       1000    //Wood per tree
#define MINE_CAPACITY       1000    //Gold per mine
#define FOOD_NODE_CAPACITY  750     //Food per bush/animal

// ===== GATHERING SETTINGS =====
#define VILLAGER_CAPACITY   25      //Max a villager can carry per trip

//Ticks it takes to gather one unit of each resource
#define GATHER_TICK_PER_WOOD   2    //25 wood = 50 ticks
#define GATHER_TICK_PER_GOLD   2
#define GATHER_TICK_PER_FOOD   3    //Food is slower (like from hunting)

#endif