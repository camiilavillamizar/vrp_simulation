#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include "map.h"
#include "villager.h"
#include "simulation.h"

int total_gold = 0;
int total_wood = 0;
int total_food = 0;
int tick_count = 0;

typedef enum {
    STATE_IDLE,
    STATE_MOVING_TO_RESOURCE,
    STATE_GATHERING,
    STATE_RETURNING,
    STATE_DROPPING
} VillagerState;

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
        //generate_random_map();
        printf("[RANK %d] Map and villagers generated. Total: %d\n", rank, villager_count);
        
        // Uncomment this to load from a file instead of generating randomly
        load_map_from_file("output/map/initial_map.txt");
    }

    //Broadcast the map dimensions
    MPI_Bcast(&game_map.width, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&game_map.height, 1, MPI_INT, 0, MPI_COMM_WORLD);

    //Broadcast the map cell contents
    MPI_Bcast(&(game_map.cells[0][0]), MAP_HEIGHT * MAP_WIDTH, MPI_INT, 0, MPI_COMM_WORLD);

    //Broadcast resource data (amount and type)
    MPI_Bcast(&(game_map.resources[0][0]), MAP_HEIGHT * MAP_WIDTH * sizeof(Resource), MPI_BYTE, 0, MPI_COMM_WORLD);

    //Broadcast the villagers
    MPI_Bcast(&villager_count, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(villagers, villager_count * sizeof(Villager), MPI_BYTE, 0, MPI_COMM_WORLD);

    //Simulation runs on Rank 1
    if (rank == 1) {
        clean_tick_folder("output/ticks");
        printf("[RANK 1] Starting simulation...\n");

        int tick = 0;
        while (!is_game_over()) {
            printf("[Tick %d] Simulating...\n", tick);

            simulate_tick();

            for (int i = 0; i < villager_count; i++) {
                Villager *v = &villagers[i];
                printf("🧍 Villager %d → Position: (%d, %d), Carrying: %d of type %d\n",
                    i, v->x, v->y, v->carrying_amount, v->carrying_type);
            }

            save_tick_state(tick);

            //send updated state to process 0 (for visualization)
            MPI_Send(villagers, villager_count * sizeof(Villager), MPI_BYTE, 0, 0, MPI_COMM_WORLD);

            tick++;
        }

        printf("[Simulation] Finished in %d ticks. Resources gathered: %d gold, %d wood, %d food\n",
               tick, total_gold, total_wood, total_food);
    }

    // Process 0  waits to receive updated state
    if (rank == 0) {
        Villager buffer[MAX_VILLAGERS];
        MPI_Status status;
        while (1) {
            int flag = 0;
            MPI_Iprobe(1, 0, MPI_COMM_WORLD, &flag, &status);
            if (!flag) break;
            MPI_Recv(buffer, MAX_VILLAGERS * sizeof(Villager), MPI_BYTE, 1, 0, MPI_COMM_WORLD, &status);
        }
    }

    free_map();
    MPI_Finalize();
    return 0;
}




int is_game_over() {
    return total_gold >= GOAL_AMOUNT &&
           total_food >= GOAL_AMOUNT &&
           total_wood >= GOAL_AMOUNT;
}

void simulate_tick() {
    for (int i = 0; i < villager_count; i++) {
        update_villager(&villagers[i]);
    }

    printf("Total accumulated: Gold: %d, Wood: %d, Food: %d\n", total_gold, total_wood, total_food);
    tick_count++;
}

void save_tick_state(int tick) {
    // Create folder if not exists
    struct stat st = {0};
    if (stat("output/ticks", &st) == -1) {
        mkdir("output/ticks", 0700);
    }

    char filename[128];
    snprintf(filename, sizeof(filename), "output/ticks/tick_%03d.txt", tick);
    FILE *f = fopen(filename, "w");
    if (!f) {
        perror("Failed to write tick state");
        return;
    }

    // Save map dimensions
    fprintf(f, "%d %d\n", game_map.height, game_map.width);

    for (int y = 0; y < game_map.height; y++) {
        for (int x = 0; x < game_map.width; x++) {
            int has_villager = 0;
            for (int i = 0; i < villager_count; i++) {
                if (villagers[i].x == x && villagers[i].y == y) {
                    has_villager = 1;
                    break;
                }
            }
            if (has_villager) {
                fprintf(f, "99 "); 
            } else {
                fprintf(f, "%d ", game_map.cells[y][x]);
            }
        }
        fprintf(f, "\n");
    }

    fclose(f);
}

void clean_tick_folder(const char *folder) {
    DIR *dir = opendir(folder);
    if (!dir) return;

    struct dirent *entry;
    char path[512];  

    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        // Build full path: folder/filename
        snprintf(path, sizeof(path), "%s/%s", folder, entry->d_name);
        remove(path);
    }

    closedir(dir);
}
