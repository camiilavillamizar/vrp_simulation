//Structure, creation and functions of the map
#ifndef MAP_H
#define MAP_H

#include "game_rules.h"  
#include "map_visuals.h"

typedef struct {
  int cells[MAP_HEIGHT][MAP_WIDTH];
  int width;
  int height;
} Map;


//Struct to manage the main game map (matrix of cells)
extern Map game_map;

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

//Saves the current map to a text file (e.g., for debugging or reloading)
void save_map_to_file(const char *filename);

//Loads a map from a file (previously saved with save_map_to_file)
//The game_map will be replaced with the one from the file
void load_map_from_file(const char *filename);


#endif