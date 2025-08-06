/**
 * @file timing_functions.c
 * @brief Implementation of timing performance measurement functions for DMR benchmark.
 *
 * This file contains the implementation of functions used to measure various
 * timing performance metrics in a distributed MPI environment with DMR capabilities.
 * It includes functions for computation timing, communication benchmarks,
 * checkpoint/restart performance, and statistical analysis.
 *
 * @author Marco De Rosso
 * @date August 6, 2025
 * @version 1.0
 */

#include "timing.h"

/**
 * @brief Get high-precision timestamp
 * @return Current time in seconds with microsecond precision
 */
double get_timestamp(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

/**
 * @brief Initialize timing measurement system
 */
timing_data_t* init_timing_system(int rank, int size)
{
    timing_data_t *data = malloc(sizeof(timing_data_t));
    if (data == NULL) {
        return NULL;
    }
    
    // Initialize all timing data
    data->iteration = 0;
    data->computation_time = 0.0;
    data->communication_time = 0.0;
    data->checkpoint_time = 0.0;
    data->restart_time = 0.0;
    data->reconfiguration_time = 0.0;
    data->total_time = 0.0;
    data->num_reconfigurations = 0;
    data->num_checkpoints = 0;
    data->num_restarts = 0;
    
    return data;
}

/**
 * @brief Perform computational work and measure timing
 */
void perform_computation_work(timing_data_t *data, int iteration)
{
    if (data == NULL) {
        return;
    }
    
    double start_time = get_timestamp();
    
    // Simulate computational work with varying intensity
    double result = 0.0;
    int workload = WORKLOAD_SIZE + (iteration % 1000); // Vary workload slightly
    
    for (int i = 0; i < workload; i++) {
        // Perform some floating-point operations
        result += sin(i * 0.001) * cos(i * 0.001);
        result += sqrt(i + 1.0);
        
        // Add some branching to make it more realistic
        if (i % 7 == 0) {
            result *= 1.001;
        }
    }
    
    // Prevent optimization from removing the computation
    volatile double dummy = result;
    (void)dummy;
    
    double end_time = get_timestamp();
    data->computation_time += (end_time - start_time);
}

/**
 * @brief Perform communication test and measure timing
 */
void perform_communication_test(int rank, int size, timing_data_t *data)
{
    if (data == NULL) {
        return;
    }
    
    double start_time = get_timestamp();
    
    // Allocate message buffer
    char *send_buffer = malloc(MESSAGE_SIZE);
    char *recv_buffer = malloc(MESSAGE_SIZE);
    
    if (send_buffer == NULL || recv_buffer == NULL) {
        free(send_buffer);
        free(recv_buffer);
        return;
    }
    
    // Initialize send buffer with test data
    for (int i = 0; i < MESSAGE_SIZE; i++) {
        send_buffer[i] = (char)(rank + i) % 256;
    }
    
    // Perform various communication patterns
    
    // 1. All-to-all communication
    MPI_Alltoall(send_buffer, MESSAGE_SIZE / size, MPI_CHAR,
                 recv_buffer, MESSAGE_SIZE / size, MPI_CHAR,
                 MPI_COMM_WORLD);
    
    // 2. Broadcast from rank 0
    MPI_Bcast(send_buffer, MESSAGE_SIZE, MPI_CHAR, 0, MPI_COMM_WORLD);
    
    // 3. Reduction operation
    double local_sum = (double)rank;
    double global_sum;
    MPI_Allreduce(&local_sum, &global_sum, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
    
    // 4. Point-to-point communication (ring pattern)
    int next_rank = (rank + 1) % size;
    int prev_rank = (rank - 1 + size) % size;
    
    MPI_Request requests[2];
    MPI_Status statuses[2];
    
    MPI_Isend(send_buffer, MESSAGE_SIZE / 4, MPI_CHAR, next_rank, 0, 
              MPI_COMM_WORLD, &requests[0]);
    MPI_Irecv(recv_buffer, MESSAGE_SIZE / 4, MPI_CHAR, prev_rank, 0,
              MPI_COMM_WORLD, &requests[1]);
    
    MPI_Waitall(2, requests, statuses);
    
    double end_time = get_timestamp();
    data->communication_time += (end_time - start_time);
    
    free(send_buffer);
    free(recv_buffer);
}

/**
 * @brief Checkpoint timing data
 */
void checkpoint_timing(int rank, int size, timing_data_t *data, const char *filepath)
{
    if (data == NULL) {
        return;
    }
    
    double start_time = get_timestamp();
    
    // Create checkpoint filename with rank
    char checkpoint_file[512];
    snprintf(checkpoint_file, sizeof(checkpoint_file), "%s.%03d", filepath, rank);
    
    // Save timing data to checkpoint file
    FILE *fp = fopen(checkpoint_file, "wb");
    if (fp != NULL) {
        fwrite(data, sizeof(timing_data_t), 1, fp);
        fclose(fp);
    }
    
    double end_time = get_timestamp();
    double checkpoint_duration = end_time - start_time;
    data->checkpoint_time += checkpoint_duration;
    data->num_checkpoints++;
}

/**
 * @brief Restart timing data from checkpoint
 */
void restart_timing(int rank, int size, timing_data_t **data, const char *filepath)
{
    if (data == NULL || *data == NULL) {
        return;
    }
    
    double start_time = get_timestamp();
    
    // Create checkpoint filename with rank
    char checkpoint_file[512];
    snprintf(checkpoint_file, sizeof(checkpoint_file), "%s.%03d", filepath, rank);
    
    // Load timing data from checkpoint file
    FILE *fp = fopen(checkpoint_file, "rb");
    if (fp != NULL) {
        fread(*data, sizeof(timing_data_t), 1, fp);
        fclose(fp);
        
        double end_time = get_timestamp();
        double restart_duration = end_time - start_time;
        (*data)->restart_time += restart_duration;
        (*data)->num_restarts++;
        
        if (rank == 0) {
            printf("Restarted from checkpoint at iteration %d\n", (*data)->iteration);
        }
    }
}

/**
 * @brief Finalize timing system and cleanup
 */
void finalize_timing(int rank, timing_data_t *data)
{
    // This function is called on DMR exit
    if (data != NULL) {
        if (rank == 0) {
            printf("Finalizing timing system...\n");
        }
        // Data will be freed in main function
    }
}

/**
 * @brief Print detailed timing results
 */
void print_timing_results(int rank, timing_data_t *data, double overall_runtime)
{
    if (data == NULL || rank != 0) {
        return; // Only rank 0 prints results
    }
    
    printf("\n");
    printf("========================================\n");
    printf("    DMR TIMING PERFORMANCE RESULTS     \n");
    printf("========================================\n");
    printf("\n");
    printf("Overall Statistics:\n");
    printf("  Total iterations completed: %d\n", data->iteration);
    printf("  Overall runtime: %.6f seconds\n", overall_runtime);
    printf("\n");
    
    printf("Timing Breakdown:\n");
    printf("  Computation time: %.6f seconds (%.2f%%)\n", 
           data->computation_time,
           overall_runtime > 0 ? (data->computation_time / overall_runtime) * 100.0 : 0.0);
    printf("  Communication time: %.6f seconds (%.2f%%)\n", 
           data->communication_time,
           overall_runtime > 0 ? (data->communication_time / overall_runtime) * 100.0 : 0.0);
    printf("  Checkpoint time: %.6f seconds (%.2f%%)\n", 
           data->checkpoint_time,
           overall_runtime > 0 ? (data->checkpoint_time / overall_runtime) * 100.0 : 0.0);
    printf("  Restart time: %.6f seconds (%.2f%%)\n", 
           data->restart_time,
           overall_runtime > 0 ? (data->restart_time / overall_runtime) * 100.0 : 0.0);
    printf("  Reconfiguration time: %.6f seconds (%.2f%%)\n", 
           data->reconfiguration_time,
           overall_runtime > 0 ? (data->reconfiguration_time / overall_runtime) * 100.0 : 0.0);
    printf("\n");
    
    printf("Event Counts:\n");
    printf("  Checkpoints taken: %d\n", data->num_checkpoints);
    printf("  Restarts performed: %d\n", data->num_restarts);
    printf("  Reconfigurations: %d\n", data->num_reconfigurations);
    printf("\n");
    
    if (data->iteration > 0) {
        printf("Average Timings per Iteration:\n");
        printf("  Computation: %.6f seconds\n", data->computation_time / data->iteration);
        printf("  Communication: %.6f seconds\n", data->communication_time / data->iteration);
        printf("  Total per iteration: %.6f seconds\n", 
               (data->computation_time + data->communication_time) / data->iteration);
        printf("\n");
    }
    
    printf("========================================\n");
}

/**
 * @brief Save timing results to file
 */
void save_timing_results(int rank, int size, timing_data_t *data, double overall_runtime)
{
    if (data == NULL || rank != 0) {
        return; // Only rank 0 saves results
    }
    
    // Create results filename with timestamp
    char filename[256];
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    strftime(filename, sizeof(filename), 
            "/home/mderosso/dmr/DMR-test/results/timing_%Y%m%d_%H%M%S.out", 
            timeinfo);
    
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        printf("Warning: Could not save results to file\n");
        return;
    }
    
    // Write header
    fprintf(fp, "# DMR Timing Performance Results\n");
    fprintf(fp, "# Generated on: %s", ctime(&rawtime));
    fprintf(fp, "# Number of processes: %d\n", size);
    fprintf(fp, "# Total iterations: %d\n", data->iteration);
    fprintf(fp, "# Overall runtime: %.6f\n", overall_runtime);
    fprintf(fp, "#\n");
    fprintf(fp, "# Format: category,time_seconds,percentage,count\n");
    fprintf(fp, "#\n");
    
    // Write timing data
    fprintf(fp, "computation,%.6f,%.2f,NA\n",
            data->computation_time,
            overall_runtime > 0 ? (data->computation_time / overall_runtime) * 100.0 : 0.0);
            
    fprintf(fp, "communication,%.6f,%.2f,NA\n",
            data->communication_time,
            overall_runtime > 0 ? (data->communication_time / overall_runtime) * 100.0 : 0.0);
            
    fprintf(fp, "checkpoint,%.6f,%.2f,%d\n",
            data->checkpoint_time,
            overall_runtime > 0 ? (data->checkpoint_time / overall_runtime) * 100.0 : 0.0,
            data->num_checkpoints);
            
    fprintf(fp, "restart,%.6f,%.2f,%d\n",
            data->restart_time,
            overall_runtime > 0 ? (data->restart_time / overall_runtime) * 100.0 : 0.0,
            data->num_restarts);
            
    fprintf(fp, "reconfiguration,%.6f,%.2f,%d\n",
            data->reconfiguration_time,
            overall_runtime > 0 ? (data->reconfiguration_time / overall_runtime) * 100.0 : 0.0,
            data->num_reconfigurations);
    
    fclose(fp);
    printf("Results saved to: %s\n", filename);
}
