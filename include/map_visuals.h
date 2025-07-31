#ifndef MAP_VISUALS_H
#define MAP_VISUALS_H

#include "game_rules.h"

/// Returns the emoji/ASCII symbol to display a map cell in terminal
char *get_cell_symbol(int cell_value);

/// Returns the RGB values for a map cell (used in .ppm rendering)
void get_cell_color(int cell_value, int *r, int *g, int *b);

#endif