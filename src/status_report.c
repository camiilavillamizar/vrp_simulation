#include "status_report.h"
#include "map.h" // game_map
#include "game_rules.h" // NUMBER_OF_VILLAGERS
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <json-c/json.h>  

#include "vrp.h"



void save_map_txt_with_villagers(
    const char* folder, int strategy_id, int tick,
    int villager_x[], int villager_y[]
) {
    char path[256];
    snprintf(path, sizeof(path), "%s/tick_strategy%d_%03d.txt", folder, strategy_id, tick);
    FILE* f = fopen(path, "w");
    if (!f) return;

    for (int y = 0; y < game_map.height; ++y) {
        for (int x = 0; x < game_map.width; ++x) {
            int printed = 0;
            for (int i = 0; i < NUMBER_OF_VILLAGERS; ++i) {
                if (villager_x[i] == x && villager_y[i] == y) {
                    fprintf(f, "V ");
                    printed = 1;
                    break;
                }
            }
            if (!printed) {
                fprintf(f, "%d ", game_map.cells[y][x]);
            }
        }
        fprintf(f, "\n");
    }

    fclose(f);
}

void save_tick_json_state(
    const char* folder, int strategy_id, int tick,
    int villager_x[], int villager_y[],
    int total_wood, int total_gold, int total_food,
    long long* total_distance_ptr,
    int collectionEffort,
    VillagerAction tick_actions[][VILLAGER_CAPACITY],
    int action_counts[]
) {
    char path[256];
    snprintf(path, sizeof(path), "%s/tick_strategy%d_%03d.json", folder, strategy_id, tick);
    FILE* f = fopen(path, "w");
    if (!f) return;

    json_object *root = json_object_new_object();

    // Tick number
    json_object_object_add(root, "tick", json_object_new_int(tick));

    // Resources
    json_object *res_obj = json_object_new_object();
    json_object_object_add(res_obj, "wood", json_object_new_int(total_wood));
    json_object_object_add(res_obj, "gold", json_object_new_int(total_gold));
    json_object_object_add(res_obj, "food", json_object_new_int(total_food));
    json_object_object_add(root, "resources", res_obj);

    // Villagers and their actions
    json_object *villagers_array = json_object_new_array();
    for (int i = 0; i < NUMBER_OF_VILLAGERS; ++i) {
        json_object *v = json_object_new_object();
        json_object_object_add(v, "id", json_object_new_int(i));
        json_object_object_add(v, "x", json_object_new_int(villager_x[i]));
        json_object_object_add(v, "y", json_object_new_int(villager_y[i]));

        json_object *actions_array = json_object_new_array();
        for (int j = 0; j < action_counts[i]; ++j) {
            VillagerAction act = tick_actions[i][j];

            const char* resource_name =
                act.resource_type == CELL_WOOD ? "wood" :
                act.resource_type == CELL_GOLD ? "gold" :
                act.resource_type == CELL_FOOD ? "food" :
                "unknown";

            char desc[128];
            snprintf(desc, sizeof(desc),
                     "collected %d %s at (%d,%d)",
                     act.amount, resource_name, act.x, act.y);

            json_object_array_add(actions_array, json_object_new_string(desc));
        }

        json_object_object_add(v, "actions", actions_array);
        json_object_array_add(villagers_array, v);
    }
    json_object_object_add(root, "villagers", villagers_array);

    // Statistics
    json_object *stats = json_object_new_object();
    json_object_object_add(stats, "totalDistance", json_object_new_int64(*total_distance_ptr));
    json_object_object_add(stats, "collectionEffort", json_object_new_int(collectionEffort));
    json_object_object_add(root, "statistics", stats);

    // Save JSON to file
    fprintf(f, "%s", json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY));
    fclose(f);
    json_object_put(root);
}


void clear_output_folders() {
    const char* folders[] = {"output/ticks", "output/simulation"};
    const int folder_count = 2;

    for (int i = 0; i < folder_count; ++i) {
        const char* folder = folders[i];

        // Create the folder if it doesn't exist
        struct stat st = {0};
        if (stat(folder, &st) == -1) {
            mkdir(folder, 0700);
            continue;
        }

        // Open directory
        DIR* dir = opendir(folder);
        if (!dir) continue;

        struct dirent* entry;
        char filepath[512];
        while ((entry = readdir(dir)) != NULL) {
            // Skip "." and ".."
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            snprintf(filepath, sizeof(filepath), "%s/%s", folder, entry->d_name);

            unlink(filepath); // Remove file

            // if (unlink(filepath) != 0) {
            //     perror("Error deleting file");
            // }
        }

        closedir(dir);
    }
}