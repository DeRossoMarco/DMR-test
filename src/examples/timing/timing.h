/**
 * @file timing.h
 * @brief Header file for MPI-based distributed timing performance benchmark with DMR capabilities.
 *
 * This header file contains function declarations for a distributed timing performance
 * benchmark using MPI with Dynamic Memory Recovery (DMR) capabilities. It defines the 
 * interface for measuring various timing metrics including computation time, 
 * communication overhead, checkpoint/restart latency, and reconfiguration costs.
 *
 * @author Marco De Rosso
 * @date August 6, 2025
 * @version 1.0
 */

#ifndef TIMING_H
#define TIMING_H

#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "dmr.h"

/** @brief Base directory path for checkpoint files */
#define FILEPATH "/home/mderosso/dmr/DMR-test/checkpoints/"
/** @brief Base filename for timing checkpoint files */
#define FILENAME "timing"

/** @brief Maximum number of timing iterations to perform */
#define MAX_ITERATIONS 1000
/** @brief Checkpoint interval (iterations) */
#define CHECKPOINT_INTERVAL 100
/** @brief Communication test message size in bytes */
#define MESSAGE_SIZE 1024
/** @brief Computation workload size per iteration */
#define WORKLOAD_SIZE 10000

/**
 * @brief Structure to hold timing measurement data
 */
typedef struct {
    int iteration;                    /**< Current iteration number */
    double computation_time;          /**< Time spent in computation */
    double communication_time;        /**< Time spent in communication */
    double checkpoint_time;           /**< Time spent in checkpointing */
    double restart_time;              /**< Time spent in restart operations */
    double reconfiguration_time;      /**< Time spent in reconfiguration */
    double total_time;                /**< Total elapsed time */
    int num_reconfigurations;         /**< Number of reconfigurations performed */
    int num_checkpoints;              /**< Number of checkpoints taken */
    int num_restarts;                 /**< Number of restarts performed */
} timing_data_t;

/* Function declarations */

/**
 * @brief Initialize timing measurement system
 * @param rank MPI rank of the process
 * @param size Total number of MPI processes
 * @return Pointer to initialized timing data structure, NULL on error
 */
timing_data_t* init_timing_system(int rank, int size);

/**
 * @brief Perform computational work and measure timing
 * @param data Pointer to timing data structure
 * @param iteration Current iteration number
 */
void perform_computation_work(timing_data_t *data, int iteration);

/**
 * @brief Perform communication test and measure timing
 * @param rank MPI rank of the process
 * @param size Total number of MPI processes
 * @param data Pointer to timing data structure
 */
void perform_communication_test(int rank, int size, timing_data_t *data);

/**
 * @brief Checkpoint timing data
 * @param rank MPI rank of the process
 * @param size Total number of MPI processes
 * @param data Pointer to timing data structure
 * @param filepath Checkpoint file path
 */
void checkpoint_timing(int rank, int size, timing_data_t *data, const char *filepath);

/**
 * @brief Restart timing data from checkpoint
 * @param rank MPI rank of the process
 * @param size Total number of MPI processes
 * @param data Pointer to timing data structure pointer
 * @param filepath Checkpoint file path
 */
void restart_timing(int rank, int size, timing_data_t **data, const char *filepath);

/**
 * @brief Finalize timing system and cleanup
 * @param rank MPI rank of the process
 * @param data Pointer to timing data structure
 */
void finalize_timing(int rank, timing_data_t *data);

/**
 * @brief Print detailed timing results
 * @param rank MPI rank of the process
 * @param data Local timing data structure
 * @param overall_runtime Overall program runtime
 */
void print_timing_results(int rank, timing_data_t *data, double overall_runtime);

/**
 * @brief Save timing results to file
 * @param rank MPI rank of the process
 * @param size Total number of MPI processes
 * @param data Local timing data structure
 * @param overall_runtime Overall program runtime
 */
void save_timing_results(int rank, int size, timing_data_t *data, double overall_runtime);

/**
 * @brief Get high-precision timestamp
 * @return Current time in seconds with microsecond precision
 */
double get_timestamp(void);

#endif /* TIMING_H */
