/**
 * @file test_functions.c
 * @brief Implementation of MPI-based checkpointing and restarting functions.
 *
 * This file contains the implementation of functions for distributed counter
 * management, checkpoint/restart operations, and process reconfiguration
 * in an MPI environment with DMR capabilities.
 *
 * @author Marco De Rosso
 * @date 26/05/2025
 * @version 1.0
 */

#include "test.h"

int offset(int rank, int size, int num_counters)
{
    // Input validation - ensure all parameters are within valid ranges
    if (size <= 0 || rank < 0 || rank >= size || num_counters <= 0)
    {
        return 0;  // Return safe default for invalid inputs
    }
    
    // Calculate the offset using integer division and modulo for load balancing
    // Formula accounts for remainder distribution among lower-ranked processes
    return rank * floor((double)num_counters / size) + (rank < num_counters % size ? rank : num_counters % size);
}

int dimension(int rank, int size, int num_counters)
{
    // Input validation - ensure all parameters are within valid ranges
    if (size <= 0 || rank < 0 || rank >= size || num_counters <= 0)
    {
        return 0;  // Return safe default for invalid inputs
    }
    
    // Calculate base number of counters per rank plus one extra for remainder distribution
    return floor((double)num_counters / size) + (rank < num_counters % size ? 1 : 0);
}

void init_data(int reconfig_count, int rank, char *filepath)
{
    // Initialize counters file only on first run and only by root process
    if (reconfig_count == 0 && rank == 0)
    {
        FILE *f = fopen(filepath, "w");
        if (!f)
        {
            fprintf(stderr, "Could not open file %s for writing\n", filepath);
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }

        // Initialize all counters to zero in the global file
        for (int i = 0; i < NUM_COUNTERS; i++)
        {
            fprintf(f, "%d\n", 0); // Write each counter value on a separate line
        }
        fclose(f);
    }
}

int *init_counters(int rank, int num_counters)
{
    // Input validation - ensure positive number of counters
    if (num_counters <= 0)
    {
        fprintf(stderr, "Invalid number of counters (%d) on rank %d\n", num_counters, rank);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
    
    // Allocate memory for the counters array
    int *counters = malloc(num_counters * sizeof(int));
    if (!counters)
    {
        fprintf(stderr, "Memory allocation failed on rank %d\n", rank);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
    
    // Initialize all counters to zero
    for (int i = 0; i < num_counters; i++)
    {
        counters[i] = 0;
    }

    printf("Rank %d initialized %d counters.\n", rank, num_counters);

    return counters;
}

int check_counters(int *counters, int num_counters)
{
    // Input validation - ensure valid pointer and positive count
    if (!counters || num_counters <= 0)
    {
        return 0;  // Return safe default (stop execution) for invalid inputs
    }
    
    // Check if any counter is still below the maximum value
    for (int i = 0; i < num_counters; i++)
    {
        if (counters[i] < MAX_COUNTER_VALUE)
        {
            return 1; // Continue computation if any local counter is below max value
        }
    }
    return 0; // Stop computation - all local counters have reached maximum value
}

void restart(int rank, int size, int *counters, int *num_counters, char *filepath)
{
    printf("Rank %d is restarting. Loading counters from file...\n", rank);

    int new_rank, new_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &new_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &new_size);

    if (new_rank != rank || new_size != size)
    {
        free(counters);
        *num_counters = dimension(new_rank, new_size, NUM_COUNTERS);
        counters = init_counters(rank, *num_counters);
    }
    
    // Input validation - ensure all required parameters are valid
    if (!counters || !filepath || *num_counters <= 0)
    {
        fprintf(stderr, "Invalid parameters for restart on rank %d\n", rank);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
    
    // Open the global checkpoint file for reading
    FILE *f = fopen(filepath, "r");
    if (!f)
    {
        fprintf(stderr, "Could not open file %s on rank %d\n", filepath, rank);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    char line[256];
    int lines_to_skip = offset(rank, size, *num_counters);

    // Skip lines belonging to previous ranks in the file
    for (int i = 0; i < lines_to_skip; i++)
    {
        if (!fgets(line, sizeof(line), f))
        {
            fprintf(stderr, "Error skipping lines on rank %d\n", rank);
            fclose(f);
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }
    }

    // Read and validate local counter values
    for (int i = 0; i < *num_counters; i++)
    {
        if (!fgets(line, sizeof(line), f))
        {
            fprintf(stderr, "Failed to read counter line on rank %d\n", rank);
            fclose(f);
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }
        counters[i] = atoi(line);
        
        // Validate counter values are within acceptable range
        if (counters[i] < 0 || counters[i] > MAX_COUNTER_VALUE)
        {
            fprintf(stderr, "Warning: Invalid counter value %d on rank %d, resetting to 0\n", counters[i], rank);
            counters[i] = 0;
        }
    }
    fclose(f);
}

void checkpoint(int rank, int size, int *counters, int num_counters, char *filepath)
{
    printf("Rank %d checkpointed. Saving data...\n", rank);

    // Phase 1: Each rank saves its local counters to a rank-specific file
    char rank_filepath[512];  // Increased buffer size for safety
    snprintf(rank_filepath, sizeof(rank_filepath), "%s.%03d", filepath, rank);

    FILE *f = fopen(rank_filepath, "w");
    if (!f)
    {
        fprintf(stderr, "Could not open file %s on rank %d\n", rank_filepath, rank);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    // Write local counters to rank-specific file (one per line)
    for (int i = 0; i < num_counters; i++)
    {
        fprintf(f, "%d\n", counters[i]);  // Direct output - more efficient than sprintf+fputs
    }

    fclose(f);

    // Synchronization barrier: ensure all ranks complete Phase 1 before Phase 2
    MPI_Barrier(MPI_COMM_WORLD);

    // Phase 2: Rank 0 aggregates all rank-specific files into global checkpoint
    if (rank == 0)
    {
        f = fopen(filepath, "w");
        if (!f)
        {
            fprintf(stderr, "Could not open file %s for writing on rank %d\n", filepath, rank);
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }

        // Aggregate counter data from all ranks in order
        for (int r = 0; r < size; r++)
        {
            char other_filepath[512];  // Increased buffer size for consistency
            snprintf(other_filepath, sizeof(other_filepath), "%s.%03d", filepath, r);
            FILE *other_f = fopen(other_filepath, "r");
            if (!other_f)
            {
                fprintf(stderr, "Could not open file %s for reading on rank %d\n", other_filepath, rank);
                fclose(f);
                MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
            }

            // Copy all lines from rank-specific file to global file
            char line[256];
            while (fgets(line, sizeof(line), other_f))
            {
                fputs(line, f);
            }
            fclose(other_f);
        }
        fclose(f);
    }

    // Final synchronization: ensure global checkpoint is complete before proceeding
    MPI_Barrier(MPI_COMM_WORLD);
}

void finalize(int rank, int *counters)
{
    printf("Rank %d is about to exit. Freeing memory...\n", rank);

    // Safe memory deallocation with null pointer check
    if (counters)
    {
        free(counters);
    }
    else
    {
        fprintf(stderr, "Warning: Attempted to free NULL pointer on rank %d\n", rank);
    }
}

void compute()
{
    // Simulate computational work by sleeping for the defined time period
    sleep(COMPUTE_TIME);
}
