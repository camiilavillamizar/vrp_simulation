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

#define CELL_TEMPLE        8   //Temple (Available in Archaic Age, to advance to classical age)
#define CELL_ARMORY        9   //Armory (Available in classical age, to advance to heroic age)
#define CELL_FORTRESS     10   //Fortress (Available in heroic age, to advance to mythic age)
#define CELL_TITAN_GATE   11   //Titan Gate (Available in mythic age, to win the game)

#define CELL_VILLAGER     -1   //Villager (may be rendered separately)

// ===== CONSTRUCTION TIMES (in ticks) =====
#define TICKS_TEMPLE        60
#define TICKS_ARMORY       120
#define TICKS_FORTRESS     240
#define TICKS_TITAN_GATE  1000

// ===== AGE ADVANCEMENT COSTS (per age) =====
#define AGE_CLASSICAL   1  //Age 2
#define AGE_HEROIC      2  //Age 3
#define AGE_MYTHIC      3  //Age 4

#define COST_CLASSICAL  300   //Cost to advance from Age 1 → 2
#define COST_HEROIC     500   //Cost to advance from Age 2 → 3
#define COST_MYTHIC    1000   //Cost to advance from Age 3 → 4

// ===== VILLAGER LOGIC =====
#define MAX_VILLAGERS         25
#define INITIAL_VILLAGERS      3

// ===== SIMULATION SETTINGS =====
#define INITIAL_RESOURCE_AMOUNT  500
#define MAX_AGE                    4

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