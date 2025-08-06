#ifndef SIMULATION_H
#define SIMULATION_H

//These globals are used to track total resources (can be external if defined elsewhere)
extern int total_gold;
extern int total_wood;
extern int total_food;

typedef struct {
    int strategy_id;                // ID of the strategy (0, 1, 2...)
    int total_collection_effort;   // Total effort spent collecting resources
    int used_ticks;                // Number of ticks used by the strategy
    long long total_distance;      // Total distance traveled by all villagers
    double elapsed_time;           // Execution time in seconds (measured with MPI_Wtime)
} StrategyResult;

/**
 * Initializes the simulation environment, including MPI, map loading, and broadcasting data.
 * Runs the appropriate strategy depending on the MPI rank and gathers results in rank 0.
 * 
 * This function is implemented in simulation.c and serves as the entry point of the program.
 */
int main(int argc, char** argv);

#endif // SIMULATION_H
