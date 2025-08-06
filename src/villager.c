//Villager's logic 
#include <stdio.h>
#include <stdlib.h>
#include "villager.h"
#include "map.h" 

extern int total_gold;
extern int total_wood;
extern int total_food;

int offsets[][2] = {
        {1, 1}, {1, -1}, {-1, 1}, {-1, -1},
        {0, 1}, {1, 0}, {0, -1}, {-1, 0},
        {2, 0}, {0, 2}, {-2, 0}, {0, -2},
        {2, 1}, {1, 2}, {-2, 1}, {-1, 2},
        {2, -1}, {1, -2}, {-2, -1}, {-1, -2}
         
    };

// Global villager array
Villager villagers[MAX_VILLAGERS];
int villager_count = 0;

int create_villager(int x, int y) {
    if (villager_count >= MAX_VILLAGERS) {
        printf("⚠️ Max number of villagers reached.\n");
        return 0;
    }

    Villager v;
    v.id = villager_count;
    v.x = x;
    v.y = y;
    v.carrying_type = CELL_EMPTY;
    v.carrying_amount = 0;
    v.task = 0; // idle
    v.target_x = -1;
    v.target_y = -1;
    v.ticks_remaining = 0;

    villagers[villager_count] = v;
    villager_count++;

    return 1;
}

void initialize_villagers(int town_x, int town_y) {
    
    int num_offsets = sizeof(offsets) / sizeof(offsets[0]);
    int created = 0;

    for (int i = 0; i < num_offsets && created < NUMBER_OF_VILLAGERS; i++) {
        int x = town_x + offsets[i][0];
        int y = town_y + offsets[i][1];
        create_villager(x, y);
        created++;
    }

    // Si no hubo suficientes offsets, puedes añadir un fallback aquí
    if (created < NUMBER_OF_VILLAGERS) {
        printf("⚠️ Not enough unique spawn positions for %d villagers\n", NUMBER_OF_VILLAGERS);
    }
}


void save_villagers_to_file(const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        perror("Failed to save villagers");
        return;
    }

    fprintf(f, "%d\n", villager_count);
    for (int i = 0; i < villager_count; i++) {
        Villager v = villagers[i];
        fprintf(f, "%d %d %d %d %d %d %d %d %d\n",
                v.id, v.x, v.y, v.carrying_type, v.carrying_amount,
                v.task, v.target_x, v.target_y, v.ticks_remaining);
    }

    fclose(f);
}

void load_villagers_from_file(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        perror("Failed to load villagers");
        return;
    }

    fscanf(f, "%d", &villager_count);
    for (int i = 0; i < villager_count; i++) {
        Villager v;
        fscanf(f, "%d %d %d %d %d %d %d %d %d",
               &v.id, &v.x, &v.y, &v.carrying_type, &v.carrying_amount,
               &v.task, &v.target_x, &v.target_y, &v.ticks_remaining);
        villagers[i] = v;
        set_cell(v.x, v.y, CELL_VILLAGER);  //Show in the map 
    }

    fclose(f);
}

