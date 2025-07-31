//Structure, creation and functions of the map
#ifndef MAP_H
#define MAP_H

#include "game_rules.h"  

//Struct to manage the main game map (matrix of cells)
extern int **game_map;

//Allocates memory and creates the map with random resources and initial layout
void generate_random_map();

//Frees the memory of the game_map
void free_map();

//Safely sets a value on the map (ignores if out of bounds)
void set_cell(int x, int y, int value);

//Safely gets a cell value (returns -99 if out of bounds)
int get_cell(int x, int y);

//Prints a small portion of the map 
void print_map_viewport(int center_x, int center_y, int view_width, int view_height);

#endif