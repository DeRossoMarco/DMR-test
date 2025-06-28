/**
 * @file test.h
 * @brief Header file for MPI-based checkpointing and restarting functionality.
 *
 * This header file contains function declarations for a distributed counter simulation
 * using MPI with Dynamic Memory Recovery (DMR) capabilities. It defines the interface
 * for distributed counter management, checkpoint/restart operations, and process
 * reconfiguration functions.
 *
 * @author Marco De Rosso
 * @date 26/05/2025
 * @version 1.0
 */

#ifndef TEST_H
#define TEST_H

#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include "dmr.h"

/** @brief Base directory path for checkpoint files */
#define FILEPATH "/home/mderosso/dmr/DMR-test/checkpoints/"
/** @brief Base filename for counter checkpoint files */
#define FILENAME "counters"
/** @brief Total number of counters distributed across all MPI ranks */
#define NUM_COUNTERS 50
/** @brief Maximum value each counter can reach before stopping */
#define MAX_COUNTER_VALUE 20
/** @brief Simulated computation time in seconds per counter increment */
#define COMPUTE_TIME 2

/**
 * @brief Computes offset for this rank in the global counter array.
 *
 * This function calculates the starting position (offset) for the current MPI rank
 * in the global array of counters. It handles load balancing by distributing
 * counters as evenly as possible across all ranks.
 *
 * @param rank Current MPI rank (0-based)
 * @param size Total number of MPI ranks
 * @param num_counters Total number of counters to distribute
 * @return Offset value for this rank in the global counters array
 *
 * @note Returns 0 for invalid input parameters as a safe default
 * @note Formula accounts for remainder distribution among lower-ranked processes
 */
int offset(int rank, int size, int num_counters);

/**
 * @brief Calculates the number of counters assigned to this rank.
 *
 * This function determines how many counters the current MPI rank should manage.
 * It ensures balanced distribution of counters across all ranks, with any remainder
 * counters distributed to the lower-numbered ranks.
 *
 * @param rank Current MPI rank (0-based)
 * @param size Total number of MPI ranks
 * @param num_counters Total number of counters to distribute
 * @return Number of counters assigned to this rank
 *
 * @note Returns 0 for invalid input parameters as a safe default
 * @note Ranks with index < (num_counters % size) get one extra counter
 */
int dimension(int rank, int size, int num_counters);

/**
 * @brief Initializes the global counters file if it doesn't exist.
 *
 * This function creates and initializes the global counters file with zero values.
 * Only rank 0 (root process) performs this operation to avoid race conditions.
 * The file is created only on the first run (reconfig_count == 0).
 *
 * @param reconfig_count Number of reconfigurations that have occurred
 * @param rank Current MPI rank
 * @param filepath Full path to the counters file
 *
 * @note This function is called before any checkpoint/restart operations
 * @note Only executed by rank 0 to ensure single-threaded file creation
 * @note Creates file with NUM_COUNTERS lines, each containing "0"
 */
void init_data(int reconfig_count, int rank, char *filepath);

/**
 * @brief Allocates and initializes local counters array for this rank.
 *
 * This function allocates memory for the local counters array and initializes
 * all counters to zero. It includes comprehensive error checking for memory
 * allocation failures and input validation.
 *
 * @param rank Current MPI rank (used for error reporting)
 * @param num_counters Number of counters to allocate for this rank
 * @return Pointer to allocated and initialized counters array
 *
 * @note The returned pointer must be freed using free() when no longer needed
 * @note Program will abort on memory allocation failure or invalid parameters
 * @note All counters are initialized to 0
 */
int *init_counters(int rank, int num_counters);

/**
 * @brief Checks if any local counter is still below the maximum value.
 *
 * This function determines whether the computation should continue by checking
 * if any of the local counters has not yet reached MAX_COUNTER_VALUE.
 * Includes input validation for safety.
 *
 * @param counters Pointer to the local counters array
 * @param num_counters Number of counters in the array
 * @return 1 if any counter is below the maximum value (continue computation),
 *         0 if all counters have reached maximum or on invalid input (stop computation)
 *
 * @note Returns 0 (stop) for NULL pointer or invalid parameters as safe default
 * @note Used to control the main computation loop termination
 */
int check_counters(int *counters, int num_counters);

/**
 * @brief Loads counter values from checkpoint file after a reconfiguration.
 *
 * This function restores the state of local counters from the global checkpoint file
 * after a system reconfiguration or restart. It calculates the appropriate file offset
 * for this rank and reads the corresponding counter values. Includes comprehensive
 * error checking and data validation.
 *
 * @param rank Current MPI rank
 * @param size Total number of MPI ranks
 * @param counters Pointer to the local counters array to populate
 * @param num_counters Number of counters this rank should manage
 * @param filepath Path to the global checkpoint file
 *
 * @note Counter values are validated and reset to 0 if outside valid range
 * @note Program will abort on file I/O errors or invalid parameters
 * @note Uses offset() function to determine correct file position for this rank
 */
void restart(int rank, int size, int *counters, int *num_counters, char *filepath);

/**
 * @brief Saves local counters and creates aggregated checkpoint file.
 *
 * This function implements a two-phase checkpointing strategy:
 * 1. Each rank saves its local counters to a rank-specific file
 * 2. Rank 0 aggregates all rank-specific files into a global checkpoint file
 *
 * The process is synchronized using MPI barriers to ensure data consistency.
 * This approach allows for fault-tolerant state preservation during reconfigurations.
 *
 * @param rank Current MPI rank
 * @param size Total number of MPI ranks
 * @param counters Pointer to the local counters array
 * @param num_counters Number of counters in the local array
 * @param filepath Base path for the checkpoint files
 *
 * @note Uses MPI barriers for synchronization between phases
 * @note Rank-specific files are temporary and aggregated by rank 0
 * @note Creates files with .XXX suffix for individual ranks, then consolidates
 */
void checkpoint(int rank, int size, int *counters, int num_counters, char *filepath);

/**
 * @brief Simulates computational work with a time delay.
 *
 * This function represents actual computational work in the simulation by
 * introducing a configurable sleep delay. In a real application, this would
 * be replaced with actual computational algorithms.
 *
 * @note Sleep duration is controlled by COMPUTE_TIME constant
 * @note Helps simulate realistic timing for checkpoint/restart testing
 * @note Called once per counter increment to simulate work load
 */
void compute();

/**
 * @brief Safely deallocates memory for counters array before process termination.
 *
 * This function performs cleanup operations before a process exits, including
 * freeing dynamically allocated memory for the counters array. Includes
 * null pointer checking to prevent segmentation faults.
 *
 * @param rank Current MPI rank (used for logging)
 * @param counters Pointer to the counters array to be freed
 *
 * @note Safe to call with NULL pointer - will log warning but not crash
 * @note Should be called before process termination to prevent memory leaks
 * @note Part of the cleanup process in DMR_AUTO finalization
 */
void finalize(int rank, int *counters);

#endif /* TEST_H */
