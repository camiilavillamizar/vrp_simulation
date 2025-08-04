#include "map_visuals.h"
#include <string.h>

char *get_cell_symbol(int cell_value) {
    switch (cell_value) {
        case CELL_EMPTY:         return ".";
        case CELL_WOOD:          return "🌲";
        case CELL_GOLD:          return "💰";
        case CELL_FOOD:          return "🍖";
        case CELL_DROP_OFF:      return "📦";
        case CELL_TOWN_CENTER:   return "🏠";
        case CELL_ENEMY_BUILD:   return "🏴";
        case CELL_FRIENDLY_BLD:  return "🏛️";
        case CELL_VILLAGER:      return "☺️";
        default:                 return "?";
    }
}

void get_cell_color(int cell_value, int *r, int *g, int *b) {
    switch (cell_value) {
        case CELL_EMPTY:         *r = 245; *g = 245; *b = 245; break; //light gray
        case CELL_WOOD:          *r = 101; *g = 67;  *b = 33;  break; //brown
        case CELL_GOLD:          *r = 255; *g = 215; *b = 0;   break; //gold
        case CELL_FOOD:          *r = 200; *g = 0;   *b = 0;   break; //red
        case CELL_DROP_OFF:      *r = 0;   *g = 0;   *b = 255; break; //blue
        case CELL_TOWN_CENTER:   *r = 0;   *g = 100; *b = 0;   break; //dark green
        case CELL_ENEMY_BUILD:   *r = 255; *g = 0;   *b = 0;   break; //red
        case CELL_FRIENDLY_BLD:  *r = 0;   *g = 255; *b = 255; break; //cyan
        default:                 *r = 100; *g = 100; *b = 100; break; //unknown
    }
}

