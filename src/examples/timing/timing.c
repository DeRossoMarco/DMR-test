#include "timing.h"

// Example computation functions to benchmark
void computation_heavy()
{
    volatile double result = 0.0;
    for (int i = 0; i < 1000000; i++) {
        result += sin(i * 0.001) * cos(i * 0.001);
    }
}

void computation_light()
{
    volatile int result = 0;
    for (int i = 0; i < 100000; i++) {
        result += i * i;
    }
}

void computation_medium()
{
    volatile double result = 0.0;
    for (int i = 0; i < 500000; i++) {
        result += sqrt(i + 1);
    }
}

// Distributed computation that works across multiple processes
void distributed_computation_work(int rank, int size)
{
    // Distribute work based on rank
    int work_start = rank * 250000;
    int work_end = (rank + 1) * 250000;
    
    volatile double result = 0.0;
    for (int i = work_start; i < work_end; i++) {
        result += sin(i * 0.001) * cos(i * 0.001);
        if (i % 10000 == 0) {
            result += sqrt(i + 1);
        }
    }
}

int main(int argc, char *argv[])
{
    // Initialize MPI environment
    MPI_Init(&argc, &argv);

    // Get MPI rank and size information
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
        printf("DMR Timing Example - Expand/Shrink Performance Analysis\n");
        printf("Running on %d processes\n", size);
        printf("========================================================\n\n");
    }

    // Initialize DMR system
    DMR_AUTO(dmr_init(argc, argv), (void)NULL, (void)NULL, (void)NULL);
    
    // Update rank and size after DMR initialization
    MPI_Comm_rank(DMR_WORLD_COMM, &rank);
    MPI_Comm_size(DMR_WORLD_COMM, &size);

    // Set expansion/shrink parameters for rank 0 (coordinator)
    if (rank == 0) {
        dmr_set_procs_next_expand(size + 2);
        dmr_set_procs_next_shrink(size - 1);
    }

    // Initialize the advanced timing system
    init_timing_system();

    // Example 1: Baseline computation without expand/shrink
    if (rank == 0) {
        printf("Example 1: Baseline computation without expand/shrink\n");
    }
    
    int baseline_timer = start_new_timer(DMR_WORLD_COMM);
    
    // Perform 5 rounds of distributed computation
    for (int round = 0; round < 5; round++) {
        distributed_computation_work(rank, size);
        MPI_Barrier(DMR_WORLD_COMM);  // Synchronize between rounds
    }
    
    stop_timer(baseline_timer, DMR_WORLD_COMM);
    report_timer_stats(baseline_timer, DMR_WORLD_COMM, "Baseline Computation (No Expand/Shrink)");

    // Example 2: Computation with expansion and shrinking
    if (rank == 0) {
        printf("\nExample 2: Computation with expansion and shrinking\n");
    }

    int expand_shrink_timer = start_new_timer(DMR_WORLD_COMM);
    int expand_timer, shrink_timer, computation_between_timer;
    
    // Phase 1: Initial computation
    distributed_computation_work(rank, size);
    MPI_Barrier(DMR_WORLD_COMM);
    
    // Phase 2: Expand operation
    if (rank == 0) {
        printf("  Triggering expansion...\n");
    }
    
    expand_timer = start_new_timer(DMR_WORLD_COMM);
    DMR_AUTO(dmr_check(SHOULD_EXPAND), (void)NULL, (void)NULL, (void)NULL);
    stop_timer(expand_timer, DMR_WORLD_COMM);
    
    // Update communicator and process info after expansion
    MPI_Comm_rank(DMR_WORLD_COMM, &rank);
    MPI_Comm_size(DMR_WORLD_COMM, &size);
    
    // Phase 3: Computation with expanded processes
    computation_between_timer = start_new_timer(DMR_WORLD_COMM);
    for (int round = 0; round < 3; round++) {
        distributed_computation_work(rank, size);
        MPI_Barrier(DMR_WORLD_COMM);
    }
    stop_timer(computation_between_timer, DMR_WORLD_COMM);
    
    // Phase 4: Shrink operation  
    if (rank == 0) {
        printf("  Triggering shrinking...\n");
    }
    
    shrink_timer = start_new_timer(DMR_WORLD_COMM);
    DMR_AUTO(dmr_check(SHOULD_SHRINK), (void)NULL, (void)NULL, (void)NULL);
    stop_timer(shrink_timer, DMR_WORLD_COMM);
    
    // Update process info after shrinking
    MPI_Comm_rank(DMR_WORLD_COMM, &rank);
    MPI_Comm_size(DMR_WORLD_COMM, &size);
    
    // Phase 5: Final computation with reduced processes
    distributed_computation_work(rank, size);
    MPI_Barrier(DMR_WORLD_COMM);
    
    stop_timer(expand_shrink_timer, DMR_WORLD_COMM);

    // Report detailed timing results for expand/shrink operations
    report_timer_stats(expand_timer, DMR_WORLD_COMM, "Expansion Operation");
    report_timer_stats(computation_between_timer, DMR_WORLD_COMM, "Computation with Expanded Processes");
    report_timer_stats(shrink_timer, DMR_WORLD_COMM, "Shrinking Operation");
    report_timer_stats(expand_shrink_timer, DMR_WORLD_COMM, "Total with Expand/Shrink");

    // Example 3: Performance comparison
    if (rank == 0) {
        printf("\nExample 3: Performance comparison and analysis\n");
        printf("==============================================\n");
        
        double baseline_time = get_elapsed_time(baseline_timer);
        double expand_shrink_time = get_elapsed_time(expand_shrink_timer);
        double expand_time = get_elapsed_time(expand_timer);
        double shrink_time = get_elapsed_time(shrink_timer);
        double computation_between_time = get_elapsed_time(computation_between_timer);
        
        printf("Baseline computation time: %.6f seconds\n", baseline_time);
        printf("Computation with expand/shrink: %.6f seconds\n", expand_shrink_time);
        printf("Expand operation time: %.6f seconds\n", expand_time);
        printf("Shrink operation time: %.6f seconds\n", shrink_time);
        printf("Computation on expanded processes: %.6f seconds\n", computation_between_time);
        
        double overhead = expand_shrink_time - baseline_time;
        double reconfiguration_overhead = expand_time + shrink_time;
        
        printf("\nOverhead Analysis:\n");
        printf("Total overhead: %.6f seconds (%.2f%%)\n", 
               overhead, (overhead / baseline_time) * 100.0);
        printf("Reconfiguration overhead: %.6f seconds (%.2f%%)\n", 
               reconfiguration_overhead, (reconfiguration_overhead / baseline_time) * 100.0);
        
        if (overhead > 0) {
            printf("Expand/shrink adds %.2f%% overhead to computation\n", 
                   (overhead / baseline_time) * 100.0);
        } else {
            printf("Expand/shrink provides %.2f%% performance improvement\n", 
                   -(overhead / baseline_time) * 100.0);
        }
    }

    // Example 4: Multiple expand/shrink cycles timing
    if (rank == 0) {
        printf("\nExample 4: Multiple expand/shrink cycles\n");
    }

    int multi_cycle_timer = start_new_timer(DMR_WORLD_COMM);
    
    for (int cycle = 0; cycle < 3; cycle++) {
        if (rank == 0) {
            printf("  Cycle %d: Computing -> Expanding -> Computing -> Shrinking\n", cycle + 1);
        }
        
        // Computation
        distributed_computation_work(rank, size);
        
        // Expand
        DMR_AUTO(dmr_check(SHOULD_EXPAND), (void)NULL, (void)NULL, (void)NULL);
        MPI_Comm_rank(DMR_WORLD_COMM, &rank);
        MPI_Comm_size(DMR_WORLD_COMM, &size);
        
        // Computation with expanded processes
        distributed_computation_work(rank, size);
        
        // Shrink
        DMR_AUTO(dmr_check(SHOULD_SHRINK), (void)NULL, (void)NULL, (void)NULL);
        MPI_Comm_rank(DMR_WORLD_COMM, &rank);
        MPI_Comm_size(DMR_WORLD_COMM, &size);
    }
    
    stop_timer(multi_cycle_timer, DMR_WORLD_COMM);
    report_timer_stats(multi_cycle_timer, DMR_WORLD_COMM, "Multiple Expand/Shrink Cycles");

    // Example 5: Summary report of all timers
    if (rank == 0) {
        printf("\nExample 5: Summary of all timing measurements\n");
    }
    report_all_timers(DMR_WORLD_COMM);

    // Final performance analysis
    if (rank == 0) {
        printf("\n=== FINAL PERFORMANCE ANALYSIS ===\n");
        printf("Baseline (no expand/shrink): %.6f seconds\n", get_elapsed_time(baseline_timer));
        printf("Single expand/shrink cycle: %.6f seconds\n", get_elapsed_time(expand_shrink_timer));
        printf("Multiple cycles: %.6f seconds\n", get_elapsed_time(multi_cycle_timer));
        printf("====================================\n");
    }

    // Finalize DMR system
    DMR_AUTO(dmr_finalize(), (void)NULL, (void)NULL, (void)NULL);

    // Finalize MPI environment
    MPI_Finalize();
    
    return EXIT_SUCCESS;
}
