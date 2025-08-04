//Villager's logic 
#include <stdio.h>
#include <stdlib.h>
#include "villager.h"
#include "map.h" 

extern int total_gold;
extern int total_wood;
extern int total_food;

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
        set_cell(v.x, v.y, CELL_VILLAGER);  //Show in the map 
    }

    fclose(f);
}

void update_villager(Villager* v) {
    int x = v->x;
    int y = v->y;

    // Deliver resources
    if (v->carrying_amount > 0 &&
        (game_map.cells[y][x] == CELL_DROP_OFF || game_map.cells[y][x] == CELL_TOWN_CENTER)) {

        if (v->carrying_type == CELL_GOLD) total_gold += v->carrying_amount;
        if (v->carrying_type == CELL_WOOD) total_wood += v->carrying_amount;
        if (v->carrying_type == CELL_FOOD) total_food += v->carrying_amount;

        v->carrying_amount = 0;
        v->carrying_type = CELL_EMPTY;
        return;
    }

    // Gather resources
    if (v->carrying_amount == 0 &&
        (game_map.cells[y][x] == CELL_GOLD ||
         game_map.cells[y][x] == CELL_WOOD ||
         game_map.cells[y][x] == CELL_FOOD)) {

        Resource* res = &game_map.resources[y][x];
        if (res->amount > 0) {
            int collected = (res->amount >= VILLAGER_CAPACITY) ? VILLAGER_CAPACITY : res->amount;
            v->carrying_amount = collected;
            v->carrying_type = res->type;
            res->amount -= collected;
        }
        return;
    }

    // Carrying something → move to drop-off
    if (v->carrying_amount > 0) {
        int best_x = -1, best_y = -1, best_dist = 9999;
        for (int yy = 0; yy < game_map.height; yy++) {
            for (int xx = 0; xx < game_map.width; xx++) {
                if (game_map.cells[yy][xx] == CELL_DROP_OFF || game_map.cells[yy][xx] == CELL_TOWN_CENTER) {
                    int dist = abs(v->x - xx) + abs(v->y - yy);
                    if (dist < best_dist) {
                        best_dist = dist;
                        best_x = xx;
                        best_y = yy;
                    }
                }
            }
        }

        if (v->x < best_x) v->x++;
        else if (v->x > best_x) v->x--;
        else if (v->y < best_y) v->y++;
        else if (v->y > best_y) v->y--;

        return;
    }

    // Not carrying → search for nearest resource
    int best_x = -1, best_y = -1, best_dist = 9999;
    for (int yy = 0; yy < game_map.height; yy++) {
        for (int xx = 0; xx < game_map.width; xx++) {
            int type = game_map.cells[yy][xx];
            if (type == CELL_GOLD || type == CELL_WOOD || type == CELL_FOOD) {
                Resource* res = &game_map.resources[yy][xx];
                if (res->amount > 0) {
                    int dist = abs(v->x - xx) + abs(v->y - yy);
                    if (dist < best_dist) {
                        best_dist = dist;
                        best_x = xx;
                        best_y = yy;
                    }
                }
            }
        }
    }

    if (best_x != -1 && best_y != -1) {
        if (v->x < best_x) v->x++;
        else if (v->x > best_x) v->x--;
        else if (v->y < best_y) v->y++;
        else if (v->y > best_y) v->y--;
    }
}
