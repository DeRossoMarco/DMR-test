#include "timing.h"

// Global variables to store timing data for multiple timing sessions
#define MAX_TIMERS 50  // Increased from 10 to accommodate comprehensive tests
static double start_times[MAX_TIMERS];
static double end_times[MAX_TIMERS];
static int active_timers[MAX_TIMERS];
static int next_timer_id = 0;

// Initialize timing system
void init_timing_system()
{
    for (int i = 0; i < MAX_TIMERS; i++) {
        start_times[i] = 0.0;
        end_times[i] = 0.0;
        active_timers[i] = 0;
    }
    next_timer_id = 0;
}

// Reset timing system (alias for init_timing_system for clarity)
void reset_timing_system()
{
    init_timing_system();
}

// Start a new timer and return its ID
int start_new_timer(MPI_Comm comm)
{
    if (next_timer_id >= MAX_TIMERS) {
        fprintf(stderr, "Error: Maximum number of timers (%d) exceeded\n", MAX_TIMERS);
        return -1;
    }
    
    int timer_id = next_timer_id++;
    
    // Synchronize all processes before starting the timer
    MPI_Barrier(comm);
    
    // Record the start time using MPI_Wtime for high precision
    start_times[timer_id] = MPI_Wtime();
    active_timers[timer_id] = 1;
    
    return timer_id;
}

// Stop a specific timer
int stop_timer(int timer_id, MPI_Comm comm)
{
    if (timer_id < 0 || timer_id >= MAX_TIMERS || !active_timers[timer_id]) {
        fprintf(stderr, "Error: Invalid timer ID %d\n", timer_id);
        return -1;
    }
    
    // Record the end time
    end_times[timer_id] = MPI_Wtime();
    
    // Synchronize all processes after stopping the timer
    MPI_Barrier(comm);
    
    active_timers[timer_id] = 0;
    return 0;
}

// Get elapsed time for a specific timer
double get_elapsed_time(int timer_id)
{
    if (timer_id < 0 || timer_id >= MAX_TIMERS) {
        fprintf(stderr, "Error: Invalid timer ID %d\n", timer_id);
        return -1.0;
    }
    
    if (active_timers[timer_id]) {
        // Timer is still running, return current elapsed time
        return MPI_Wtime() - start_times[timer_id];
    } else {
        // Timer has stopped, return final elapsed time
        return end_times[timer_id] - start_times[timer_id];
    }
}

// Report detailed timing statistics for a specific timer
void report_timer_stats(int timer_id, MPI_Comm comm, const char* timer_name)
{
    if (timer_id < 0 || timer_id >= MAX_TIMERS) {
        fprintf(stderr, "Error: Invalid timer ID %d\n", timer_id);
        return;
    }
    
    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    
    // Calculate local execution time
    double local_time = get_elapsed_time(timer_id);
    
    if (local_time < 0) {
        fprintf(stderr, "Error: Could not get elapsed time for timer %d\n", timer_id);
        return;
    }
    
    // Gather timing statistics across all processes
    double min_time, max_time, avg_time, total_time;
    double variance_sum = 0.0, variance = 0.0, std_dev = 0.0;
    
    // Reduce operations to get min, max, and sum of execution times
    MPI_Reduce(&local_time, &min_time, 1, MPI_DOUBLE, MPI_MIN, 0, comm);
    MPI_Reduce(&local_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, comm);
    MPI_Reduce(&local_time, &total_time, 1, MPI_DOUBLE, MPI_SUM, 0, comm);
    
    // Calculate average
    avg_time = total_time / size;
    
    // Calculate variance and standard deviation
    double diff = local_time - avg_time;
    double diff_squared = diff * diff;
    MPI_Reduce(&diff_squared, &variance_sum, 1, MPI_DOUBLE, MPI_SUM, 0, comm);
    variance = variance_sum / size;
    std_dev = sqrt(variance);
    
    // Only rank 0 prints the detailed results
    if (rank == 0) {
        printf("\n=== Timing Results for %s ===\n", timer_name ? timer_name : "Timer");
        printf("Timer ID: %d\n", timer_id);
        printf("Number of processes: %d\n", size);
        printf("Minimum time: %.6f seconds\n", min_time);
        printf("Maximum time: %.6f seconds\n", max_time);
        printf("Average time: %.6f seconds\n", avg_time);
        printf("Standard deviation: %.6f seconds\n", std_dev);
        printf("Total cumulative time: %.6f seconds\n", total_time);
        printf("Load imbalance: %.2f%%\n", ((max_time - min_time) / avg_time) * 100.0);
        printf("===============================\n\n");
    }
}

// Report timing for all completed timers
void report_all_timers(MPI_Comm comm)
{
    int rank;
    MPI_Comm_rank(comm, &rank);
    
    if (rank == 0) {
        printf("\n=== Summary of All Timers ===\n");
    }
    
    for (int i = 0; i < next_timer_id; i++) {
        char timer_name[32];
        snprintf(timer_name, sizeof(timer_name), "Timer %d", i);
        report_timer_stats(i, comm, timer_name);
    }
}

// Utility function to benchmark a code section
double benchmark_section(MPI_Comm comm, void (*function_to_benchmark)(void), const char* section_name)
{
    int timer_id = start_new_timer(comm);
    if (timer_id < 0) {
        return -1.0;
    }
    
    // Execute the function to benchmark
    if (function_to_benchmark) {
        function_to_benchmark();
    }
    
    stop_timer(timer_id, comm);
    
    // Report results if a name is provided
    if (section_name) {
        report_timer_stats(timer_id, comm, section_name);
    }
    
    return get_elapsed_time(timer_id);
}
