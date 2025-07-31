//ticks engine and principal cicle
#ifndef SIMULATION_H
#define SIMULATION_H

#include "game_rules.h"

//Starts the simulation loop and controls the game ticks
void run_simulation();

//Returns true if the player has met the win condition (Titan Gate built)
bool has_won();

//Initializes global simulation state (resources, age, etc.)
void initialize_simulation();

#endif
