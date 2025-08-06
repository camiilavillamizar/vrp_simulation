#ifndef STATUS_REPORT_H
#define STATUS_REPORT_H

#include "villager.h"  // For VillagerAction
#include "game_rules.h"  // For NUMBER_OF_VILLAGERS

/**
 * Saves the current map state to a .txt file for visualization.
 * The map includes villagers and cell values.
 *
 * @param folder Output base folder (e.g., "output/ticks")
 * @param strategy_id Strategy number (0, 1, 2, ...)
 * @param tick Current simulation tick
 * @param villager_x Array of villagers' x positions
 * @param villager_y Array of villagers' y positions
 */
void save_map_txt_with_villagers(
    const char* folder, int strategy_id, int tick,
    int villager_x[], int villager_y[]
);

/**
 * Saves the state of a simulation tick into a structured JSON file.
 * Includes resources, villagers' actions, paths, and statistics.
 *
 * @param folder Output base folder (e.g., "output/simulation")
 * @param strategy_id Strategy number
 * @param tick Tick number
 * @param villager_x X positions of villagers
 * @param villager_y Y positions of villagers
 * @param total_wood Amount of wood collected
 * @param total_gold Amount of gold collected
 * @param total_food Amount of food collected
 * @param total_distance_ptr Pointer to cumulative distance
 * @param collectionEffort Total collection effort so far
 * @param tick_actions Actions taken by villagers at each tick
 * @param action_counts Number of actions per villager
 */
void save_tick_json_state(
    const char* folder, int strategy_id, int tick,
    int villager_x[], int villager_y[],
    int total_wood, int total_gold, int total_food,
    long long* total_distance_ptr,
    int collectionEffort,
    VillagerAction tick_actions[][VILLAGER_CAPACITY],
    int action_counts[]
);

/**
 * Recursively deletes a folder and all its contents.
 *
 * @param path Folder path to delete
 */
void delete_folder_recursive(const char *path);

/**
 * Clears and recreates all simulation output folders.
 * Specifically targets "output/ticks" and "output/simulation".
 */
void clear_output_folders();

#endif // STATUS_REPORT_H
