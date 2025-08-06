#ifndef STATUS_REPORT_H
#define STATUS_REPORT_H

#include "vrp.h"

void save_map_txt_with_villagers(
    const char* folder,
    int strategy_id,
    int tick,
    int villager_x[],
    int villager_y[]
);

void save_tick_json_state(
    const char* folder, int strategy_id, int tick,
    int villager_x[], int villager_y[],
    int total_wood, int total_gold, int total_food,
    long long* total_distance_ptr,
    int collectionEffort,
    VillagerAction tick_actions[][VILLAGER_CAPACITY],
    int action_counts[]
);

void clear_output_folders();

#endif // STATUS_REPORT_H