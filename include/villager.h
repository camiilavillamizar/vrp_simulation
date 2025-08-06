#ifndef VILLAGER_H
#define VILLAGER_H

#include "game_rules.h"

// Structure that represents a single villager in the game
typedef struct {
    int id;                 // Unique identifier for the villager
    int x, y;               // Current position on the map
    int carrying_type;      // Type of resource being carried (GOLD, WOOD, FOOD, or EMPTY)
    int carrying_amount;    // How much of the resource the villager is carrying
    int task;               // Current task status (e.g., idle, moving, gathering)
    int target_x, target_y; // Target position for current task
    int ticks_remaining;    // Ticks left to complete current task (for future expansions)
    int resource_type;      //The type of resource the villager is currently gathering
    int target_found;
} Villager;

// Log for each villager's action in a tick
typedef struct {
  int villager_id;      // Villager ID
  int action_idx;       // The order of resource collection in this tick (0,1,...)
  int resource_type;    // CELL_GOLD, CELL_WOOD, CELL_FOOD
  int x, y;             // Resource coordinate
  int amount;           // Amount collected this action
} VillagerAction;

enum TaskStatus {
    TASK_IDLE = 0,
    TASK_MOVING = 1,
    TASK_GATHERING = 2,
    TASK_DELIVERING = 3
};


// Global array holding all villagers in the game
extern Villager villagers[MAX_VILLAGERS];

// Number of villagers currently active in the game
extern int villager_count;

/**
 * Creates a villager at the given position.
 * Returns 1 if created successfully, 0 if limit reached.
 */
int create_villager(int x, int y);

/**
 * Initializes villagers around the town center at (town_x, town_y).
 */
void initialize_villagers(int town_x, int town_y);

/**
 * Saves all current villagers to a file.
 */
void save_villagers_to_file(const char *filename);

/**
 * Loads villagers from a file and places them back into the map.
 */
void load_villagers_from_file(const char *filename);

#endif
