#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "map.h"
#include "villager.h"

// Global static map (as declared extern in map.h)
Map game_map;

void free_map() {
    // No-op for static array. No dynamic allocation to free.
}

void set_cell(int x, int y, int value) {
    if (x >= 0 && x < game_map.width && y >= 0 && y < game_map.height)
        game_map.cells[y][x] = value;
}

int get_cell(int x, int y) {
    if (x >= 0 && x < game_map.width && y >= 0 && y < game_map.height)
        return game_map.cells[y][x];
    return -99;
}

void generate_random_map() {
    int total_cells = MAP_WIDTH * MAP_HEIGHT;
    int wood_cells = total_cells * PERCENT_WOOD;
    int gold_cells = total_cells * PERCENT_GOLD;
    int food_cells = total_cells * PERCENT_FOOD;

    srand(time(NULL));
    game_map.width = MAP_WIDTH;
    game_map.height = MAP_HEIGHT;

    // Fill empty
    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++) {
            game_map.cells[y][x] = CELL_EMPTY;
            game_map.resources[y][x].type = -1;
            game_map.resources[y][x].amount = 0;
        }

    // Place Town Center and villagers
    int tc_x = 2, tc_y = 2;
    game_map.cells[tc_y][tc_x] = CELL_TOWN_CENTER;
    initialize_villagers(tc_x, tc_y);

    // Place WOOD
    int placed_wood = 0;
    while (placed_wood < wood_cells) {
        int x = rand() % MAP_WIDTH, y = rand() % MAP_HEIGHT;
        if (game_map.cells[y][x] == CELL_EMPTY) {
            game_map.cells[y][x] = CELL_WOOD;
            game_map.resources[y][x].type = CELL_WOOD;
            game_map.resources[y][x].amount = TREE_CAPACITY;
            placed_wood++;
        }
    }

    // Place GOLD
    int placed_gold = 0;
    while (placed_gold < gold_cells) {
        int x = rand() % MAP_WIDTH, y = rand() % MAP_HEIGHT;
        if (game_map.cells[y][x] == CELL_EMPTY) {
            game_map.cells[y][x] = CELL_GOLD;
            game_map.resources[y][x].type = CELL_GOLD;
            game_map.resources[y][x].amount = MINE_CAPACITY;
            placed_gold++;
        }
    }

    // Place FOOD
    int placed_food = 0;
    while (placed_food < food_cells) {
        int x = rand() % MAP_WIDTH, y = rand() % MAP_HEIGHT;
        if (game_map.cells[y][x] == CELL_EMPTY) {
            game_map.cells[y][x] = CELL_FOOD;
            game_map.resources[y][x].type = CELL_FOOD;
            game_map.resources[y][x].amount = FOOD_NODE_CAPACITY;
            placed_food++;
        }
    }


    save_map_to_file("output/map/initial_map.txt");

}

void save_map_to_file(const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) { perror("Failed to open file for writing"); return; }
    fprintf(f, "%d %d\n", game_map.height, game_map.width);
    for (int y = 0; y < game_map.height; y++) {
        for (int x = 0; x < game_map.width; x++)
            fprintf(f, "%d ", game_map.cells[y][x]);
        fprintf(f, "\n");
    }
    fprintf(f, "VILLAGERS %d\n", villager_count);
    for (int i = 0; i < villager_count; i++)
        fprintf(f, "%d %d %d\n", villagers[i].x, villagers[i].y, villagers[i].carrying_type);
    fclose(f);
}

void load_map_from_file(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        perror("Failed to open file for reading");
        return;
    }

    int height, width;
    // Read map dimensions and validate against expected size
    if (fscanf(f, "%d %d", &height, &width) != 2) {
        fprintf(stderr, "Invalid map file format\n");
        fclose(f);
        return;
    }

    if (height != MAP_HEIGHT || width != MAP_WIDTH) {
        fprintf(stderr, "Map size mismatch: Expected %dx%d, got %dx%d\n",
                MAP_HEIGHT, MAP_WIDTH, height, width);
        fclose(f);
        return;
    }

    game_map.height = height;
    game_map.width = width;

    // Load each cell value and initialize resource metadata accordingly
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int cell_type;
            fscanf(f, "%d", &cell_type);
            game_map.cells[y][x] = cell_type;

            // Initialize resource properties if this is a resource cell
            if (cell_type == CELL_WOOD) {
                game_map.resources[y][x].type = CELL_WOOD;
                game_map.resources[y][x].amount = TREE_CAPACITY;
            } else if (cell_type == CELL_GOLD) {
                game_map.resources[y][x].type = CELL_GOLD;
                game_map.resources[y][x].amount = MINE_CAPACITY;
            } else if (cell_type == CELL_FOOD) {
                game_map.resources[y][x].type = CELL_FOOD;
                game_map.resources[y][x].amount = FOOD_NODE_CAPACITY;
            } else {
                // Non-resource cell
                game_map.resources[y][x].type = -1;
                game_map.resources[y][x].amount = 0;
            }
        }
    }

    // Read villager metadata after the map
    char buffer[256];
    int vcount = 0;
    while (fgets(buffer, sizeof(buffer), f)) {
        if (strncmp(buffer, "VILLAGERS", 9) == 0) {
            sscanf(buffer, "VILLAGERS %d", &vcount);
            for (int i = 0; i < vcount; i++) {
                int x, y, ctype;
                fscanf(f, "%d %d %d", &x, &y, &ctype);
                create_villager(x, y);
                villagers[i].carrying_type = ctype;
            }
        }
    }

    fclose(f);
}


void print_map_viewport(int center_x, int center_y, int view_width, int view_height) {
    int start_x = center_x - view_width / 2;
    int start_y = center_y - view_height / 2;
    printf("\nMap Viewport (Center: %d, %d):\n", center_x, center_y);
    for (int y = 0; y < view_height; y++) {
        for (int x = 0; x < view_width; x++) {
            int mx = start_x + x, my = start_y + y;
            if (mx < 0 || mx >= game_map.width || my < 0 || my >= game_map.height) {
                printf("# ");
                continue;
            }
            printf("%d ", game_map.cells[my][mx]);
        }
        printf("\n");
    }
}