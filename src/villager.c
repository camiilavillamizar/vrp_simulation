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
