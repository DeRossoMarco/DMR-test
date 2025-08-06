/**
 * @file dmr_utils.h
 * @brief Common DMR utilities and helper functions for MPI applications.
 *
 * This header file contains shared utilities and helper functions for
 * Dynamic Memory Recovery (DMR) applications with MPI support.
 *
 * @author Marco De Rosso
 * @date 06/08/2025
 * @version 1.0
 */

#ifndef DMR_UTILS_H
#define DMR_UTILS_H

#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Calculates load distribution parameters for MPI ranks.
 *
 * This structure holds the distribution parameters for evenly distributing
 * work items across MPI ranks with remainder handling.
 */
typedef struct {
    int offset;     /**< Starting index for this rank */
    int count;      /**< Number of items for this rank */
} LoadDistribution;

/**
 * @brief Calculates load distribution for a given rank.
 *
 * This function computes how to distribute total_items across size ranks,
 * ensuring balanced distribution with remainder items given to lower ranks.
 *
 * @param rank Current MPI rank (0-based)
 * @param size Total number of MPI ranks
 * @param total_items Total number of items to distribute
 * @return LoadDistribution structure with offset and count
 */
LoadDistribution calculate_load_distribution(int rank, int size, int total_items);

/**
 * @brief Validates MPI rank and size parameters.
 *
 * @param rank Current MPI rank
 * @param size Total number of MPI ranks
 * @return 1 if valid, 0 if invalid
 */
int validate_mpi_params(int rank, int size);

/**
 * @brief Creates a formatted checkpoint filepath.
 *
 * @param base_path Base directory path
 * @param filename Base filename
 * @param rank MPI rank (optional, use -1 for global files)
 * @param buffer Output buffer for the formatted path
 * @param buffer_size Size of the output buffer
 * @return 1 on success, 0 on failure
 */
int format_checkpoint_path(const char *base_path, const char *filename, 
                          int rank, char *buffer, size_t buffer_size);

/**
 * @brief Safe file operations with error handling.
 *
 * @param filepath Path to the file
 * @param mode File open mode ("r", "w", etc.)
 * @param comm MPI communicator for error handling
 * @return FILE pointer on success, aborts on failure
 */
FILE *safe_fopen(const char *filepath, const char *mode, MPI_Comm comm);

#endif /* DMR_UTILS_H */
