/**
 * @file mpi_helpers.h
 * @brief Common MPI helper functions and utilities.
 *
 * This header file contains helper functions for common MPI operations
 * that can be reused across different DMR examples.
 *
 * @author Marco De Rosso
 * @date 06/08/2025
 * @version 1.0
 */

#ifndef MPI_HELPERS_H
#define MPI_HELPERS_H

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Initialize MPI and get rank and size information.
 *
 * @param argc Pointer to argc from main
 * @param argv Pointer to argv from main
 * @param rank Pointer to store current rank
 * @param size Pointer to store total number of ranks
 */
void mpi_init_and_get_info(int *argc, char ***argv, int *rank, int *size);

/**
 * @brief Print a formatted message with rank information.
 *
 * @param rank Current MPI rank
 * @param format Printf-style format string
 * @param ... Variable arguments for the format string
 */
void print_rank_message(int rank, const char *format, ...);

/**
 * @brief Perform a synchronized barrier with optional debug output.
 *
 * @param comm MPI communicator
 * @param debug_message Optional debug message to print (can be NULL)
 */
void synchronized_barrier(MPI_Comm comm, const char *debug_message);

/**
 * @brief Check if current rank is root (rank 0).
 *
 * @param rank Current MPI rank
 * @return 1 if rank is 0, 0 otherwise
 */
int is_root_rank(int rank);

/**
 * @brief Safe MPI finalization with error checking.
 */
void safe_mpi_finalize(void);

#endif /* MPI_HELPERS_H */
