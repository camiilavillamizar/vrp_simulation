//Villager's logic 
#ifndef VILLAGER_H
#define VILLAGER_H

#include "game_rules.h"

//Represents a single villager in the simulation
typedef struct {
    int id;              // Unique identifier
    int x, y;            // Current position on the map
    int carrying_type;   // Type of resource being carried (CELL_WOOD, CELL_GOLD, etc.)
    int carrying_amount; // Amount being carried (max VILLAGER_CAPACITY)
    int task;            // Task type (0 = idle, 1 = gathering, 2 = delivering, 3 = building, ...)
    int target_x;        // Target cell x
    int target_y;        // Target cell y
    int ticks_remaining; // Time left to complete current action
} Villager;

//Array of all active villagers
extern Villager villagers[MAX_VILLAGERS];

//Current number of villagers
extern int villager_count;

//Initializes the starting villagers around the town center
void initialize_villagers(int town_x, int town_y);

//Moves each villager one tick forward in their task
void update_villagers();

//Creates a new villager if there is space (returns 1 if successful, 0 if full)
int create_villager(int x, int y);

//Returns 1 if there is any villager at position (x, y), 0 otherwise
int is_villager_at(int x, int y);

//Renders all villagers into the map (for visual output only)
void render_villagers_on_map(int **map);

#endif
