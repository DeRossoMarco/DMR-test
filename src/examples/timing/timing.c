/**
 * @file timing.c
 * @brief Main program for distributed timing performance benchmark with MPI and DMR capabilities.
 *
 * This file contains the main function for a distributed timing performance benchmark using MPI
 * (Message Passing Interface) with Dynamic Memory Recovery (DMR) capabilities.
 * The program measures various performance metrics including computation time,
 * communication overhead, checkpoint/restart latency, and reconfiguration costs.
 *
 * @author Marco De Rosso
 * @date August 6, 2025
 * @version 1.0
 */

#include "timing.h"

/**
 * @brief Main function implementing distributed timing performance benchmark with DMR support.
 *
 * This function initializes the MPI environment, sets up timing measurement infrastructure,
 * and runs the main benchmark loop with checkpoint/restart capabilities.
 * The program supports dynamic process reconfiguration through the DMR library.
 *
 * @param argc Number of command line arguments
 * @param argv Array of command line argument strings
 * @return EXIT_SUCCESS on successful completion, EXIT_FAILURE on error
 */
int main(int argc, char *argv[])
{
    // Initialize MPI environment
    MPI_Init(&argc, &argv);

    // Get MPI rank and size information
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
        printf("Starting DMR Timing Performance Benchmark\n");
        printf("Number of MPI processes: %d\n", size);
        printf("Maximum iterations: %d\n", MAX_ITERATIONS);
        printf("Checkpoint interval: %d\n", CHECKPOINT_INTERVAL);
        printf("Message size: %d bytes\n", MESSAGE_SIZE);
        printf("Workload size: %d\n", WORKLOAD_SIZE);
        printf("----------------------------------------\n");
    }

    // Construct full filepath for checkpoint files
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s%s", FILEPATH, FILENAME);

    // Initialize timing data structure
    timing_data_t *timing_data = init_timing_system(rank, size);
    if (timing_data == NULL) {
        fprintf(stderr, "Rank %d: Error initializing timing system\n", rank);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    // Initialize DMR with restart callback
    DMR_AUTO(dmr_init(argc, argv), (void)NULL, 
             restart_timing(rank, size, &timing_data, filepath), (void)NULL);

    MPI_Comm comm = dmr_get_world_comm();

    // Set expansion parameters for rank 0 (coordinator)
    if (rank == 0) {
        dmr_set_procs_next_expand(size + 2);
        dmr_set_procs_next_shrink(size - 1);
    }

    // Synchronize all processes before starting main computation
    MPI_Barrier(comm);

    double program_start_time = get_timestamp();

    // Main benchmark loop
    while (timing_data->iteration < MAX_ITERATIONS) {
        double iteration_start_time = get_timestamp();

        // Perform computational work
        perform_computation_work(timing_data, timing_data->iteration);

        // Perform communication test
        perform_communication_test(rank, size, timing_data);

        // Determine dynamic reconfiguration suggestion
        DMRSuggestion suggestion = SHOULD_STAY;

        // Suggest expansion early in execution for testing
        if (timing_data->iteration == 200) {
            suggestion = SHOULD_EXPAND;
        }
        // Suggest shrinking later in execution
        else if (timing_data->iteration == 600) {
            suggestion = SHOULD_SHRINK;
        }

        // Update iteration timing
        double iteration_end_time = get_timestamp();
        double iteration_time = iteration_end_time - iteration_start_time;
        timing_data->total_time += iteration_time;

        timing_data->iteration++;

        // Progress reporting
        if (rank == 0 && timing_data->iteration % 100 == 0) {
            printf("Completed iteration %d/%d (%.1f%%)\n", 
                   timing_data->iteration, MAX_ITERATIONS,
                   (double)timing_data->iteration / MAX_ITERATIONS * 100.0);
        }

        // Check for reconfiguration and perform checkpoint with cleanup on exit
        DMR_AUTO(dmr_check(suggestion), 
                checkpoint_timing(rank, size, timing_data, filepath), 
                restart_timing(rank, size, &timing_data, filepath), 
                finalize_timing(rank, timing_data));
    }

    // Calculate overall runtime
    double program_end_time = get_timestamp();
    double overall_runtime = program_end_time - program_start_time;

    if (rank == 0) {
        printf("Benchmark completed. Collecting statistics...\n");
        print_timing_results(rank, timing_data, overall_runtime);
        save_timing_results(rank, size, timing_data, overall_runtime);
    }

    // Cleanup
    DMR_AUTO(dmr_finalize(), (void)NULL, (void)NULL, (void)NULL);
    free(timing_data);
    MPI_Finalize();

    if (rank == 0) {
        printf("DMR Timing Performance Benchmark completed successfully.\n");
    }

    return EXIT_SUCCESS;
}
