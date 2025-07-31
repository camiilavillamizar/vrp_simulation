// Structure, creation and functions of the map
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "map.h"
#include "game_rules.h"
#include "villager.h"

//Global pointer to the game map
int **game_map;

void generate_random_map() {

    //Seed randomness
    srand(time(NULL));

    //Allocate memory for game_map
    game_map = malloc(MAP_HEIGHT * sizeof(int *));
    for (int i = 0; i < MAP_HEIGHT; i++) {
        game_map[i] = malloc(MAP_WIDTH * sizeof(int));
    }

    //Fill map with empty cells
    for (int i = 0; i < MAP_HEIGHT; i++) {
        for (int j = 0; j < MAP_WIDTH; j++) {
            game_map[i][j] = CELL_EMPTY;
        }
    }

    //=== 1. Place Town Center in top-left corner ===
    int tc_x = 2;
    int tc_y = 2;
    game_map[tc_y][tc_x] = CELL_TOWN_CENTER;

    //=== 2. Place 3 initial villagers around the town center ===
    initialize_villagers(tc_x, tc_y);

    // === 3. Place 4 friendly buildings near the town center ===
    game_map[tc_y - 1][tc_x] = CELL_FRIENDLY_BLD;  //above
    game_map[tc_y][tc_x - 1] = CELL_FRIENDLY_BLD;  //left
    game_map[tc_y - 1][tc_x - 1] = CELL_FRIENDLY_BLD;
    game_map[tc_y + 2][tc_x] = CELL_FRIENDLY_BLD;

    // === 4. Place enemy building in opposite corner ===
    game_map[MAP_HEIGHT - 3][MAP_WIDTH - 3] = CELL_ENEMY_BUILD;

    // === 5. Place resources randomly ===
    int num_trees = 1000;
    int num_mines = 500;
    int num_food_sources = 600;

    //Trees
    for (int i = 0; i < num_trees; i++) {
        int x = rand() % MAP_WIDTH;
        int y = rand() % MAP_HEIGHT;
        if (game_map[y][x] == CELL_EMPTY) {
            game_map[y][x] = CELL_WOOD;
        }
    }

    //Mines
    for (int i = 0; i < num_mines; i++) {
        int x = rand() % MAP_WIDTH;
        int y = rand() % MAP_HEIGHT;
        if (game_map[y][x] == CELL_EMPTY) {
            game_map[y][x] = CELL_GOLD;
        }
    }

    //Food
    for (int i = 0; i < num_food_sources; i++) {
        int x = rand() % MAP_WIDTH;
        int y = rand() % MAP_HEIGHT;
        if (game_map[y][x] == CELL_EMPTY) {
            game_map[y][x] = CELL_FOOD;
        }
    }

    save_map_to_file("output/map/initial_map.txt");
}

void free_map() {
    if (game_map == NULL) return;

    for (int i = 0; i < MAP_HEIGHT; i++) {
        free(game_map[i]);
    }
    free(game_map);
    game_map = NULL;
}

void save_map_to_file(const char *filename) {

    FILE *f = fopen(filename, "w");
    if (!f) {
        perror("Could not open file to save map");
        return;
    }

    //First line: size
    fprintf(f, "%d %d\n", MAP_HEIGHT, MAP_WIDTH);

    //Each line: one row of the map
    for (int i = 0; i < MAP_HEIGHT; i++) {
        for (int j = 0; j < MAP_WIDTH; j++) {
            fprintf(f, "%d ", game_map[i][j]);
        }
        fprintf(f, "\n");
    }

    // Save villagers count and their positions
    fprintf(f, "VILLAGERS %d\n", villager_count);
    for (int i = 0; i < villager_count; i++) {
        fprintf(f, "%d %d\n", villagers[i].x, villagers[i].y);
    }

    save_villagers_to_file("output/map/villagers.txt");

    fclose(f);
    printf("Map saved to %s\n", filename);
}

void load_map_from_file(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        perror("Could not open map file for loading");
        return;
    }

    int height, width;
    if (fscanf(f, "%d %d", &height, &width) != 2) {
        fprintf(stderr, "Invalid map format\n");
        fclose(f);
        return;
    }

    //Safety check
    if (height != MAP_HEIGHT || width != MAP_WIDTH) {
        fprintf(stderr, "Map size mismatch: expected %dx%d, got %dx%d\n",
                MAP_HEIGHT, MAP_WIDTH, height, width);
        fclose(f);
        return;
    }

    //Allocate new map
    game_map = malloc(height * sizeof(int *));
    for (int i = 0; i < height; i++) {
        game_map[i] = malloc(width * sizeof(int));
        for (int j = 0; j < width; j++) {
            fscanf(f, "%d", &game_map[i][j]);
        }
    }

    char line[256];
    if (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "VILLAGERS", 9) == 0) {
            int num;
            sscanf(line, "VILLAGERS %d", &num);
            for (int i = 0; i < num; i++) {
                int x, y;
                if (fscanf(f, "%d %d", &x, &y) == 2) {
                    create_villager(x, y);
                }
            }
        }
    }

    load_villagers_from_file("output/map/villagers.txt");
    fclose(f);
    printf("Map loaded from %s\n", filename);
}

//Safely retrieves the value of a cell (returns -99 if out of bounds)
int get_cell(int x, int y) {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) {
        return -99;
    }
    return game_map[y][x]; // Remember: rows (y) first, then columns (x)
}

//Safely sets a value on the map (ignores if out of bounds)
void set_cell(int x, int y, int value) {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) {
        return;
    }
    game_map[y][x] = value;
}

void print_map_viewport(int center_x, int center_y, int view_width, int view_height) {
    int start_x = center_x - view_width / 2;
    int start_y = center_y - view_height / 2;

    printf("\nMap Viewport centered at (%d, %d):\n", center_x, center_y);
    for (int y = 0; y < view_height; y++) {
        for (int x = 0; x < view_width; x++) {
            int map_x = start_x + x;
            int map_y = start_y + y;

            int value = get_cell(map_x, map_y);
            printf("%s ", get_cell_symbol(value));
        }
        printf("\n");
    }
}