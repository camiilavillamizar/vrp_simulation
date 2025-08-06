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

// Creates a new villager at the specified (x, y) location
int create_villager(int x, int y);

// Initializes the starting villagers near the town center
void initialize_villagers(int town_x, int town_y);

// Saves the state of all villagers to a file
void save_villagers_to_file(const char *filename);

// Loads the state of villagers from a file
void load_villagers_from_file(const char *filename);

#endif
