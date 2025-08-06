#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "map.h"
#include "villager.h"
#include "vrp.h"
#include "status_report.h"

int total_gold = 0;
int total_wood = 0;
int total_food = 0;

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
        clear_output_folders();

        //generate_random_map();
        
        // Uncomment this to load from a file instead of generating randomly
        load_map_from_file("output/map/initial_map.txt");

        printf("[RANK %d] Map and villagers generated. Total: %d\n", rank, villager_count);
        
    }
    
    // Broadcast the game map to all processes
    MPI_Bcast(&game_map, sizeof(Map), MPI_BYTE, 0, MPI_COMM_WORLD);

    //Broadcast the villagers
    MPI_Bcast(&villager_count, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(villagers, villager_count * sizeof(Villager), MPI_BYTE, 0, MPI_COMM_WORLD);

    if (rank >= 1 && rank <= 3) {
        int strategy_id = rank - 1;
        int total_collection_effort = 0, used_ticks = 0;
        long long total_distance = 0;

        double start_time = MPI_Wtime();
        run_strategy_simulation(strategy_id, &total_collection_effort, &used_ticks, &total_distance, rank);
        double end_time = MPI_Wtime();

        double elapsed_time = end_time - start_time;

        StrategyResult result = {strategy_id, total_collection_effort, used_ticks, total_distance, elapsed_time};
        MPI_Send(&result, sizeof(result), MPI_BYTE, 0, 0, MPI_COMM_WORLD);
    }

    if (rank == 0) {
        StrategyResult results[3];
        for (int i = 0; i < 3; ++i)
            MPI_Recv(&results[i], sizeof(StrategyResult), MPI_BYTE, i+1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        printf("\n=== All Strategies Finished ===\n");
        for (int i = 0; i < 3; ++i) {
            const char *name =
                (results[i].strategy_id == 0) ? "Greedy Nearest" :
                (results[i].strategy_id == 1) ? "Max Profit/Distance" :
                "Optimal Permutation";
            printf("[Strategy %d: %s]\n  Collection efforts: %d\n  Ticks: %d\n  Total Distance: %lld\n  Time: %.4f seconds\n\n",
                results[i].strategy_id, name,
                results[i].total_collection_effort,
                results[i].used_ticks,
                results[i].total_distance,
                results[i].elapsed_time
            );
        }
    }

    free_map();
    MPI_Finalize();
    return 0;
}