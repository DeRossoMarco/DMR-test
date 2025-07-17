/**
 * @file test.c
 * @brief Main program for MPI-based checkpointing, restarting, and finalizing processes.
 *
 * This file contains the main function for a distributed counter simulation using MPI
 * (Message Passing Interface) with Dynamic Memory Recovery (DMR) capabilities.
 * The program simulates a distributed computation where each MPI rank manages
 * a subset of counters, incrementing them until they reach MAX_COUNTER_VALUE.
 * The system supports dynamic reconfiguration through the DMR library.
 *
 * @author Marco De Rosso
 * @date 26/05/2025
 * @version 1.0
 */

#include "test.h"

/**
 * @brief Main function implementing distributed counter simulation with DMR support.
 *
 * This function initializes the MPI environment, sets up distributed counters,
 * and runs the main computation loop with checkpoint/restart capabilities.
 * The program supports dynamic process reconfiguration through the DMR library.
 *
 * @param argc Number of command line arguments
 * @param argv Array of command line argument strings
 * @return EXIT_SUCCESS on successful completion, EXIT_FAILURE on error
 *
 * @details The function performs the following operations:
 * 1. Initialize MPI environment and get rank/size information
 * 2. Calculate local counter distribution for this rank
 * 3. Initialize DMR with restart capabilities
 * 4. Run main computation loop incrementing counters
 * 5. Handle dynamic reconfiguration suggestions (expand/shrink)
 * 6. Checkpoint state periodically for fault tolerance
 * 7. Clean up resources and finalize MPI
 */
int main(int argc, char *argv[])
{
    // Initialize MPI environment
    MPI_Init(&argc, &argv);

    // Get MPI rank and size information
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Construct full filepath for checkpoint files
    char filepath[512];  // Increased buffer size for safety
    snprintf(filepath, sizeof(filepath), "%s%s", FILEPATH, FILENAME);

    // Calculate number of counters for this rank
    int num_counters_local = dimension(rank, size, NUM_COUNTERS);
    if (num_counters_local <= 0)
    {
        fprintf(stderr, "Invalid number of local counters (%d) on rank %d\n", num_counters_local, rank);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
    
    // Allocate and initialize local counters array
    int *counters = init_counters(rank, num_counters_local);

    // Initialize DMR with the provided arguments and restart callback
    DMR_AUTO(dmr_init(argc, argv), (void)NULL, restart(rank, size, &counters, &num_counters_local, filepath), (void)NULL);

    MPI_Comm comm = dmr_get_world_comm();

    // Set expansion parameters for rank 0 (coordinator)
    if (rank == 0)
    {
        dmr_set_procs_next_expand(2);
        dmr_set_procs_next_shrink(2);
    }

    // Synchronize all processes before starting main computation
    MPI_Barrier(comm);

    // Main computation loop - continue until all local counters reach maximum value
    while (check_counters(counters, num_counters_local))
    {
        // Increment each local counter and perform computation
        for (int i = 0; i < num_counters_local; i++)
        {
            if (counters[i] < MAX_COUNTER_VALUE)
            {
                // Increment the counter if it hasn't reached maximum value
                counters[i]++;
            }
            // Simulate computational work
            compute();
        }

        // Print current state of all local counters for debugging and monitoring
        printf("Rank %d counters: ", rank);
        for (int i = 0; i < num_counters_local; i++)
        {
            printf("%d ", counters[i]);
        }
        printf("\n");

        // Determine dynamic reconfiguration suggestion based on counter values
        DMRSuggestion suggestion = SHOULD_STAY;

        // Expand when counters reach certain thresholds (more processes needed)
        if (counters[0] == 3)
        {
            suggestion = SHOULD_EXPAND;
        }
        // Shrink when computation is nearly complete (fewer processes needed)
        else if (counters[0] == 8)
        {
            suggestion = SHOULD_SHRINK;
        }

        // Synchronize all processes before checkpoint/reconfiguration
        // MPI_Barrier(comm);

        // Check for reconfiguration and perform checkpoint with cleanup on exit
        DMR_AUTO(dmr_check(suggestion), checkpoint(rank, size, counters, num_counters_local, filepath), restart(rank, size, &counters, &num_counters_local, filepath), finalize(rank, counters));
    }

    // Finalize DMR system
    DMR_AUTO(dmr_finalize(), (void)NULL, (void)NULL, (void)NULL);

    // Finalize MPI environment
    MPI_Finalize();

    return EXIT_SUCCESS;
}