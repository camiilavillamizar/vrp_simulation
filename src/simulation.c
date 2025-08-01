//ticks engine and principal cicle
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include "map.h"
#include "renderer.h"
#include "villager.h"
#include "vrp.h"

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Initialize game state
    load_map_from_file("output/map/initial_map.txt");
    place_villagers_on_map();
    render_to_ppm("output/map/initial_map.ppm", &game_map);
    
    // Broadcast map to workers
    if (rank == 0) {
        for (int i = 1; i < size; ++i) {
            MPI_Send(&game_map.width, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(&game_map.height, 1, MPI_INT, i, 0, MPI_COMM_WORLD);

            for (int y = 0; y < game_map.height; y++) {
                MPI_Send(game_map.cells[y], game_map.width, MPI_INT, i, 0, MPI_COMM_WORLD);
            }
        }
    } else {
        MPI_Recv(&game_map.width, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&game_map.height, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        for (int y = 0; y < game_map.height; y++) {
            MPI_Recv(game_map.cells[y], game_map.width, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    }

    // Execute strategy based on rank
    StrategyResult result;
    int num_villagers = 3;
    switch (rank) {
        case 1: 
            result = assign_task_greedy(&game_map, villagers, num_villagers); 
            break;
        case 2: 
            result = assign_task_max_profit(&game_map, villagers, num_villagers); 
            break;
        case 3: 
            result = assign_task_stage_based(&game_map, villagers, num_villagers); 
            break;
        case 4: 
            result = assign_task_region_based(&game_map, villagers, num_villagers); 
            break;
        default: 
            result.total_collected = 0;
            result.total_ticks = 0;
    }

    printf("[Worker %d] Strategy complete: resources=%d, ticks=%d\n", 
           rank, result.total_collected, result.total_ticks);

    free_map();
    MPI_Finalize();
    return 0;
}
