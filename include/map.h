// map.h — Structure and functions for managing the main game map
#ifndef MAP_H
#define MAP_H

#include "game_rules.h"    //Contains constants like MAP_WIDTH, MAP_HEIGHT, resource capacities
#include "map_visuals.h"   //For rendering or visualization
#include "resource.h"      //Eesource struct (type and amount) definition

// Map struct represents the game world grid and associated resources
typedef struct {
    int cells[MAP_HEIGHT][MAP_WIDTH];            // Main grid: each cell contains an element type (e.g., EMPTY, WOOD, GOLD)
    Resource resources[MAP_HEIGHT][MAP_WIDTH];   // Parallel grid storing resource info per cell (if applicable)
    int width;                                   // Width of the map (typically MAP_WIDTH)
    int height;                                  // Height of the map (typically MAP_HEIGHT)
} Map;

// Global instance of the map used by all modules
extern Map game_map;

// === MAP MANAGEMENT FUNCTIONS ===

// Initializes the map with random resource distribution and places the Town Center and initial villagers
void generate_random_map();

// Deallocates map resources (currently a no-op since static arrays are used)
void free_map();

// Sets a cell's type if within map bounds (e.g., setting a cell to CELL_WOOD)
void set_cell(int x, int y, int value);

// Returns the type of a cell; returns -99 if coordinates are out of bounds
int get_cell(int x, int y);

// === I/O OPERATIONS FOR THE MAP ===

// Saves the current state of the map and villagers to a text file
// Useful for debugging or replay functionality
void save_map_to_file(const char *filename);

// Loads the map and villagers from a file created by save_map_to_file()
// Overwrites the current game_map contents
void load_map_from_file(const char *filename);

// Prints a small section of the map around a center coordinate (for debugging purposes)
void print_map_viewport(int center_x, int center_y, int view_width, int view_height);

#endif // MAP_H
