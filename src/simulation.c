#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "map.h"
#include "renderer.h"
#include "villager.h"
#include "vrp.h"

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
        load_map_from_file("output/map/initial_map.txt");
        place_villagers_on_map();
        render_to_ppm("output/map/initial_map.ppm", &game_map);
    }

    if (rank != 0) {
        printf("Pre-bcast: width=%d height=%d vcount=%d\n", game_map.width, game_map.height, villager_count);
    }

    MPI_Bcast(&game_map.width, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&game_map.height, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&(game_map.cells[0][0]), MAP_HEIGHT * MAP_WIDTH, MPI_INT, 0, MPI_COMM_WORLD);

    MPI_Bcast(&villager_count, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(villagers, villager_count * sizeof(Villager), MPI_BYTE, 0, MPI_COMM_WORLD);

    printf("[RANK %d] Post-bcast: width=%d height=%d vcount=%d\n", rank, game_map.width, game_map.height, villager_count);

    Path *villager_paths = (Path*)malloc(sizeof(Path) * villager_count);

    StrategyResult result = {0, 0};
    switch (rank) {
        case 1:
            result = assign_task_greedy(&game_map, villagers, villager_count, villager_paths);
            break;
        case 2:
            result = assign_task_max_profit(&game_map, villagers, villager_count, villager_paths);
            break;
        case 3:
            result = assign_task_stage_based(&game_map, villagers, villager_count, villager_paths);
            break;
        case 4:
            result = assign_task_region_based(&game_map, villagers, villager_count, villager_paths);
            break;
        default:
            break;
    }

    if (rank > 0 && rank < 5) {
        printf("[Worker %d] Strategy complete: resources=%d, ticks=%d\n",
            rank, result.total_collected, result.total_ticks);
        for (int i = 0; i < villager_count; i++) {
            printf("[Worker %d] Villager %d path length: %d, path: ", rank, i, villager_paths[i].length);
            for (int j = 0; j < villager_paths[i].length; j++) {
                printf("(%d,%d)%s", villager_paths[i].x[j], villager_paths[i].y[j],
                       (j+1 == villager_paths[i].length) ? "" : "->");
            }
            printf("\n");
        }
    }

    free(villager_paths);
    free_map();

    MPI_Finalize();
    return 0;
}