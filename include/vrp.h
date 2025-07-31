//VRP engine
#ifndef VRP_H
#define VRP_H

#include "game_rules.h"
#include "villager.h"

//Trigger the VRP engine to decide what each villager should do next
void execute_vrp();

//Returns true if something in the world changed and VRP should be executed
int should_execute_vrp();

//Notifies that a state-changing event happened (e.g., new villager, building complete)
void notify_world_changed();

#endif
