#ifndef RESOURCE_H
#define RESOURCE_H

//This struct represents a resource on the map.
//Each resource has a type (e.g., gold, wood, food)
//and an amount representing how much of it is left.
typedef struct {
    int type;   // CELL_GOLD, CELL_WOOD, CELL_FOOD
    int amount; // Remaining amount (e.g., starts at 1000)
} Resource;

#endif 