#ifndef SIMULATION_H
#define SIMULATION_H

#include "game_rules.h"

// Initializes the global simulation state (resources, age, counters, etc.)
void initialize_simulation(void);

// Runs one tick of the simulation loop (villager actions, movement, etc.)
void simulate_tick(void);

// Saves the current map state to a tick file for visualization
void save_tick_state(int tick);

// Returns true if the player has met the win condition (e.g., 1000 of each resource)
int is_game_over(void);
void clean_tick_folder(const char *folder);

#endif // SIMULATION_H
