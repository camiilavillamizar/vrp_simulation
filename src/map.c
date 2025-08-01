#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "map.h"
#include "game_rules.h"
#include "villager.h"

// Global game map instance
Map game_map;

/**
 * Free all memory allocated for the map
 */
void free_map() {
    // if (game_map.cells == NULL) return;

    // Free each row
    // for (int i = 0; i < game_map.height; i++) {
    //     free(game_map.cells[i]);
    // }
    // Free the row pointers
    // free(game_map.cells);
}

/**
 * Generate a new random map with resources and structures
 */
void generate_random_map() {
    // Initialize random seed
    srand(time(NULL));
    
    // Initialize map structure
    // init_map();

    // Fill entire map with empty cells
    for (int y = 0; y < game_map.height; y++) {
        for (int x = 0; x < game_map.width; x++) {
            game_map.cells[y][x] = CELL_EMPTY;
        }
    }

    // === 1. Place Town Center in top-left corner ===
    int tc_x = 2;
    int tc_y = 2;
    game_map.cells[tc_y][tc_x] = CELL_TOWN_CENTER;

    // === 2. Place initial villagers around town center ===
    initialize_villagers(tc_x, tc_y);

    // === 3. Place friendly buildings near town center ===
    game_map.cells[tc_y - 1][tc_x] = CELL_FRIENDLY_BLD;  // Above TC
    game_map.cells[tc_y][tc_x - 1] = CELL_FRIENDLY_BLD;  // Left of TC
    game_map.cells[tc_y - 1][tc_x - 1] = CELL_FRIENDLY_BLD;
    game_map.cells[tc_y + 2][tc_x] = CELL_FRIENDLY_BLD;

    // === 4. Place enemy building in opposite corner ===
    game_map.cells[game_map.height - 3][game_map.width - 3] = CELL_ENEMY_BUILD;

    // === 5. Place resources randomly ===
    const int num_trees = 1000;
    const int num_mines = 500;
    const int num_food_sources = 600;

    // Place trees (wood resources)
    for (int i = 0; i < num_trees; i++) {
        int x = rand() % game_map.width;
        int y = rand() % game_map.height;
        if (game_map.cells[y][x] == CELL_EMPTY) {
            game_map.cells[y][x] = CELL_WOOD;
        }
    }

    // Place mines (gold resources)
    for (int i = 0; i < num_mines; i++) {
        int x = rand() % game_map.width;
        int y = rand() % game_map.height;
        if (game_map.cells[y][x] == CELL_EMPTY) {
            game_map.cells[y][x] = CELL_GOLD;
        }
    }

    // Place food sources (berry bushes)
    for (int i = 0; i < num_food_sources; i++) {
        int x = rand() % game_map.width;
        int y = rand() % game_map.height;
        if (game_map.cells[y][x] == CELL_EMPTY) {
            game_map.cells[y][x] = CELL_FOOD;
        }
    }

    // Save generated map to file
    save_map_to_file("output/map/initial_map.txt");
}

/**
 * Save map data to a text file
 * 
 * @param filename Path to output file
 */
void save_map_to_file(const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        perror("Failed to open map file for writing");
        return;
    }

    // Write map dimensions
    fprintf(f, "%d %d\n", game_map.height, game_map.width);

    // Write cell values
    for (int y = 0; y < game_map.height; y++) {
        for (int x = 0; x < game_map.width; x++) {
            fprintf(f, "%d ", game_map.cells[y][x]);
        }
        fprintf(f, "\n");
    }

    // Save villagers information
    fprintf(f, "VILLAGERS %d\n", villager_count);
    for (int i = 0; i < villager_count; i++) {
        fprintf(f, "%d %d %d\n", 
                villagers[i].x, 
                villagers[i].y,
                // villagers[i].age,
                villagers[i].carrying_type);
    }

    fclose(f);
    printf("Map saved to %s\n", filename);
}

/**
 * Load map data from a text file
 * 
 * @param filename Path to input file
 */
void load_map_from_file(const char *filename)
{
    FILE *f = fopen(filename, "r");
    if (!f) {
        perror("Failed to open map file for reading");
        return;
    }

    int height, width;
    if (fscanf(f, "%d %d", &height, &width) != 2) {
        fprintf(stderr, "Invalid map file format\n");
        fclose(f);
        return;
    }

    // Verify map dimensions
    if (height != MAP_HEIGHT || width != MAP_WIDTH) {
        fprintf(stderr, "Map size mismatch: Expected %dx%d, got %dx%d\n",
                MAP_HEIGHT, MAP_WIDTH, height, width);
        fclose(f);
        return;
    }

    // Initialize map structure
    // init_map();

    // Read cell values
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (fscanf(f, "%d", &game_map.cells[y][x]) != 1) {
                fprintf(stderr, "Error reading map data\n");
                fclose(f);
                free_map();
                return;
            }
        }
    }

    // Read villagers information
    char buffer[256];
    if (fgets(buffer, sizeof(buffer), f)) { // Skip newline
        if (fgets(buffer, sizeof(buffer), f)) {
            if (strncmp(buffer, "VILLAGERS", 9) == 0) {
                int num_villagers = 3;
                // sscanf(buffer, "VILLAGERS %d", &num_villagers);
                
                for (int i = 0; i < num_villagers; i++) {
                    int x, y, carrying_type;
                    if (fscanf(f, "%d %d %d", &x, &y, &carrying_type) == 4) {
                        create_villager(x, y);
                        villagers[i].carrying_type = carrying_type;
                    }
                }
            }
        }
    }

    fclose(f);
    printf("Map loaded from %s\n", filename);
}

/**
 * Get cell value at specified coordinates
 * 
 * @param x X coordinate (column)
 * @param y Y coordinate (row)
 * @return Cell value or CELL_INVALID if out of bounds
 */
int get_cell(int x, int y) {
    if (x < 0 || x >= game_map.width || y < 0 || y >= game_map.height) {
        return -1;
    }
    return game_map.cells[y][x];
}

/**
 * Set cell value at specified coordinates
 * 
 * @param x X coordinate (column)
 * @param y Y coordinate (row)
 * @param value New cell value
 */
void set_cell(int x, int y, int value) {
    if (x < 0 || x >= game_map.width || y < 0 || y >= game_map.height) {
        return;
    }
    game_map.cells[y][x] = value;
}

/**
 * Print a rectangular viewport of the map
 * 
 * @param center_x Center X coordinate of viewport
 * @param center_y Center Y coordinate of viewport
 * @param view_width Width of viewport to display
 * @param view_height Height of viewport to display
 */
void print_map_viewport(int center_x, int center_y, int view_width, int view_height) {
    int start_x = center_x - view_width / 2;
    int start_y = center_y - view_height / 2;

    printf("\nMap Viewport (Center: %d, %d):\n", center_x, center_y);
    for (int y = 0; y < view_height; y++) {
        for (int x = 0; x < view_width; x++) {
            int map_x = start_x + x;
            int map_y = start_y + y;

            // Handle out-of-bound coordinates
            if (map_x < 0 || map_x >= game_map.width || 
                map_y < 0 || map_y >= game_map.height) {
                printf("# ");
                continue;
            }

            // Check for villagers at this position
            int has_villager = 0;
            for (int i = 0; i < villager_count; i++) {
                if (villagers[i].x == map_x && villagers[i].y == map_y) {
                    printf("V ");
                    has_villager = 1;
                    break;
                }
            }
            
            // Print cell symbol if no villager present
            if (!has_villager) {
                printf("%s ", get_cell_symbol(game_map.cells[map_y][map_x]));
            }
        }
        printf("\n");
    }
}