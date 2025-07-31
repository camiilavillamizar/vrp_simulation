//Villager's logic 
#include <stdio.h>
#include "villager.h"
#include "map.h" 

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

void place_villagers_on_map() {
    for (int i = 0; i < villager_count; i++) {
        set_cell(villagers[i].x, villagers[i].y, CELL_VILLAGER);
    }
}
void initialize_villagers(int town_x, int town_y) {
    create_villager(town_x + 1, town_y + 1);
    create_villager(town_x + 1, town_y - 1);
    create_villager(town_x - 1, town_y + 1);
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
        set_cell(v.x, v.y, CELL_VILLAGER);  // Mostrar en el mapa
    }

    fclose(f);
}

