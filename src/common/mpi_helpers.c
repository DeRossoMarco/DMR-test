/**
 * @file mpi_helpers.c
 * @brief Implementation of common MPI helper functions.
 *
 * This file contains the implementation of helper functions for common
 * MPI operations that can be reused across different DMR examples.
 *
 * @author Marco De Rosso
 * @date 06/08/2025
 * @version 1.0
 */

#include "mpi_helpers.h"
#include <stdarg.h>

void mpi_init_and_get_info(int *argc, char ***argv, int *rank, int *size)
{
    // Initialize MPI environment
    MPI_Init(argc, argv);
    
    // Get rank and size information
    MPI_Comm_rank(MPI_COMM_WORLD, rank);
    MPI_Comm_size(MPI_COMM_WORLD, size);
}

void print_rank_message(int rank, const char *format, ...)
{
    if (!format) return;
    
    va_list args;
    va_start(args, format);
    
    printf("Rank %d: ", rank);
    vprintf(format, args);
    printf("\n");
    fflush(stdout);
    
    va_end(args);
}

void synchronized_barrier(MPI_Comm comm, const char *debug_message)
{
    if (debug_message)
    {
        int rank;
        MPI_Comm_rank(comm, &rank);
        print_rank_message(rank, "Synchronizing: %s", debug_message);
    }
    
    MPI_Barrier(comm);
}

int is_root_rank(int rank)
{
    return (rank == 0);
}

void safe_mpi_finalize(void)
{
    int finalized;
    MPI_Finalized(&finalized);
    
    if (!finalized)
    {
        MPI_Finalize();
    }
}
