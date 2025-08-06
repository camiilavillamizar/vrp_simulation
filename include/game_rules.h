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
#define MAP_WIDTH               20
#define MAP_HEIGHT              20
#define TICK_SLEEP_US          10000  //10ms between ticks for visual pacing (optional)

#define PERCENT_WOOD   0.30  // 30% of the map will be wood
#define PERCENT_GOLD   0.10  // 10% gold
#define PERCENT_FOOD   0.15  // 15% food

#define PERCENT_DROP_OFF 0.02


// ===== RESOURCE NODES CAPACITY =====
#define TREE_CAPACITY       1000    //Wood per tree
#define MINE_CAPACITY       1000    //Gold per mine
#define FOOD_NODE_CAPACITY  750     //Food per bush/animal

// ===== GATHERING SETTINGS =====
#define VILLAGER_CAPACITY   25      //Max a villager can carry per trip

//Ticks it takes to gather one unit of each resource
#define TICKS_PER_WOOD_UNIT 2
#define TICKS_PER_GOLD_UNIT 3
#define TICKS_PER_FOOD_UNIT 1

#define GOAL_AMOUNT 10000

#define GOAL_WOOD 100
#define GOAL_GOLD 100
#define GOAL_FOOD 100

#define NUMBER_OF_VILLAGERS 3


#endif