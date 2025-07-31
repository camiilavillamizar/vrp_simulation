//Visualization: export .ppm
#ifndef RENDERER_H
#define RENDERER_H

#include "game_rules.h"

//Generates a .ppm image from the map and saves it to the given filename
void render_to_ppm(const char *filename, int **map, int width, int height);

#endif

