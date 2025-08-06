/**
 * @file dmr_utils.c
 * @brief Implementation of common DMR utilities and helper functions.
 *
 * This file contains the implementation of shared utilities for
 * Dynamic Memory Recovery (DMR) applications with MPI support.
 *
 * @author Marco De Rosso
 * @date 06/08/2025
 * @version 1.0
 */

#include "dmr_utils.h"
#include <math.h>

LoadDistribution calculate_load_distribution(int rank, int size, int total_items)
{
    LoadDistribution dist = {0, 0};
    
    // Input validation
    if (!validate_mpi_params(rank, size) || total_items <= 0)
    {
        return dist;  // Return safe defaults
    }
    
    // Calculate base items per rank and remainder
    int base_items = total_items / size;
    int remainder = total_items % size;
    
    // Calculate offset (starting position for this rank)
    dist.offset = rank * base_items + (rank < remainder ? rank : remainder);
    
    // Calculate count (number of items for this rank)
    dist.count = base_items + (rank < remainder ? 1 : 0);
    
    return dist;
}

int validate_mpi_params(int rank, int size)
{
    return (size > 0 && rank >= 0 && rank < size);
}

int format_checkpoint_path(const char *base_path, const char *filename, 
                          int rank, char *buffer, size_t buffer_size)
{
    if (!base_path || !filename || !buffer || buffer_size == 0)
    {
        return 0;
    }
    
    int result;
    if (rank >= 0)
    {
        // Create rank-specific filepath
        result = snprintf(buffer, buffer_size, "%s%s.%03d", base_path, filename, rank);
    }
    else
    {
        // Create global filepath
        result = snprintf(buffer, buffer_size, "%s%s", base_path, filename);
    }
    
    return (result > 0 && result < buffer_size) ? 1 : 0;
}

FILE *safe_fopen(const char *filepath, const char *mode, MPI_Comm comm)
{
    if (!filepath || !mode)
    {
        int rank;
        MPI_Comm_rank(comm, &rank);
        fprintf(stderr, "Invalid parameters for file open on rank %d\n", rank);
        MPI_Abort(comm, EXIT_FAILURE);
    }
    
    FILE *file = fopen(filepath, mode);
    if (!file)
    {
        int rank;
        MPI_Comm_rank(comm, &rank);
        fprintf(stderr, "Could not open file %s with mode %s on rank %d\n", 
                filepath, mode, rank);
        MPI_Abort(comm, EXIT_FAILURE);
    }
    
    return file;
}
