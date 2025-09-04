#include "timing.h"

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

// Structure to store timing results for CSV output
typedef struct {
    int initial_procs;
    int expand_procs;
    int shrink_procs;
    int cycle;
    double expand_time;
    double shrink_time;
    double computation_time;
    double total_time;
    double overhead_percent;
} timing_result_t;

// Function to write CSV header
void write_csv_header(FILE *file, const char* test_type) {
    if (strcmp(test_type, "varying_initial") == 0) {
        fprintf(file, "initial_procs,cycle,expand_time,shrink_time,computation_time,total_time,overhead_percent\n");
    } else if (strcmp(test_type, "varying_expand_shrink") == 0) {
        fprintf(file, "initial_procs,expand_procs,shrink_procs,cycle,expand_time,shrink_time,computation_time,total_time,overhead_percent\n");
    } else if (strcmp(test_type, "shrink_only") == 0) {
        fprintf(file, "initial_procs,shrink_to_procs,shrink_time,computation_before,computation_after,total_time\n");
    }
}

// Function to write timing result to CSV
void write_timing_result_csv(FILE *file, const timing_result_t *result, const char* test_type) {
    if (strcmp(test_type, "varying_initial") == 0) {
        fprintf(file, "%d,%d,%.6f,%.6f,%.6f,%.6f,%.2f\n",
                result->initial_procs, result->cycle, result->expand_time,
                result->shrink_time, result->computation_time, result->total_time,
                result->overhead_percent);
    } else if (strcmp(test_type, "varying_expand_shrink") == 0) {
        fprintf(file, "%d,%d,%d,%d,%.6f,%.6f,%.6f,%.6f,%.2f\n",
                result->initial_procs, result->expand_procs, result->shrink_procs,
                result->cycle, result->expand_time, result->shrink_time,
                result->computation_time, result->total_time, result->overhead_percent);
    }
}

// Function to write shrink-only result to CSV
void write_shrink_only_csv(FILE *file, int initial_procs, int shrink_to_procs, 
                          double shrink_time, double comp_before, double comp_after, double total_time) {
    fprintf(file, "%d,%d,%.6f,%.6f,%.6f,%.6f\n",
            initial_procs, shrink_to_procs, shrink_time, comp_before, comp_after, total_time);
}

// Test 1: Fixed expansion/shrinking with varying initial processes
void test_varying_initial_processes(int argc, char *argv[]) {
    int rank, size;
    MPI_Comm_rank(DMR_WORLD_COMM, &rank);
    MPI_Comm_size(DMR_WORLD_COMM, &size);
    
    if (rank == 0) {
        printf("\n=== TEST 1: Varying Initial Processes ===\n");
        printf("Fixed expansion: +2 processes, Fixed shrinking: -1 process\n");
        printf("Number of cycles: 3\n");
        printf("Current initial processes: %d\n\n", size);
    }
    
    FILE *csv_file = NULL;
    if (rank == 0) {
        csv_file = fopen("results/test1_varying_initial.csv", "w");
        if (csv_file) {
            write_csv_header(csv_file, "varying_initial");
        }
    }
    
    // Set fixed expansion/shrink parameters
    if (rank == 0) {
        dmr_set_procs_next_expand(size + 2);
        dmr_set_procs_next_shrink(size - 1);
    }
    
    // Baseline measurement (do this once outside the loop)
    if (rank == 0) {
        printf("Measuring baseline performance...\n");
    }
    int baseline_timer = start_new_timer(DMR_WORLD_COMM);
    distributed_computation_work(rank, size);
    MPI_Barrier(DMR_WORLD_COMM);
    stop_timer(baseline_timer, DMR_WORLD_COMM);
    double baseline_time = get_elapsed_time(baseline_timer);
    
    // Run 3 cycles of expand/shrink
    for (int cycle = 0; cycle < 3; cycle++) {
        if (rank == 0) {
            printf("Cycle %d with %d initial processes\n", cycle + 1, size);
        }
        
        // Reset timers for each cycle to avoid overflow
        reset_timing_system();
        
        timing_result_t result = {0};
        result.initial_procs = size;
        result.cycle = cycle + 1;
        
        // Start total timer
        int total_timer = start_new_timer(DMR_WORLD_COMM);
        
        // Expand operation
        int expand_timer = start_new_timer(DMR_WORLD_COMM);
        DMR_AUTO(dmr_check(SHOULD_EXPAND), (void)NULL, (void)NULL, (void)NULL);
        stop_timer(expand_timer, DMR_WORLD_COMM);
        result.expand_time = get_elapsed_time(expand_timer);
        
        // Update size after expansion
        MPI_Comm_rank(DMR_WORLD_COMM, &rank);
        MPI_Comm_size(DMR_WORLD_COMM, &size);
        
        // Computation with expanded processes
        int comp_timer = start_new_timer(DMR_WORLD_COMM);
        distributed_computation_work(rank, size);
        MPI_Barrier(DMR_WORLD_COMM);
        stop_timer(comp_timer, DMR_WORLD_COMM);
        result.computation_time = get_elapsed_time(comp_timer);
        
        // Shrink operation
        int shrink_timer = start_new_timer(DMR_WORLD_COMM);
        DMR_AUTO(dmr_check(SHOULD_SHRINK), (void)NULL, (void)NULL, (void)NULL);
        stop_timer(shrink_timer, DMR_WORLD_COMM);
        result.shrink_time = get_elapsed_time(shrink_timer);
        
        // Update size after shrinking
        MPI_Comm_rank(DMR_WORLD_COMM, &rank);
        MPI_Comm_size(DMR_WORLD_COMM, &size);
        
        stop_timer(total_timer, DMR_WORLD_COMM);
        result.total_time = get_elapsed_time(total_timer);
        result.overhead_percent = ((result.total_time - baseline_time) / baseline_time) * 100.0;
        
        if (rank == 0 && csv_file) {
            write_timing_result_csv(csv_file, &result, "varying_initial");
            fflush(csv_file);
        }
    }
    
    if (rank == 0 && csv_file) {
        fclose(csv_file);
        printf("Results written to: results/test1_varying_initial.csv\n");
    }
}

// Test 2: Fixed initial processes with varying expansion/shrinking
void test_varying_expand_shrink(int argc, char *argv[]) {
    int rank, size;
    MPI_Comm_rank(DMR_WORLD_COMM, &rank);
    MPI_Comm_size(DMR_WORLD_COMM, &size);
    
    if (rank == 0) {
        printf("\n=== TEST 2: Varying Expansion/Shrinking ===\n");
        printf("Fixed initial processes: %d\n", size);
        printf("Testing different expansion and shrinking amounts\n\n");
    }
    
    FILE *csv_file = NULL;
    if (rank == 0) {
        csv_file = fopen("results/test2_varying_expand_shrink.csv", "w");
        if (csv_file) {
            write_csv_header(csv_file, "varying_expand_shrink");
        }
    }
    
    // Test different expansion/shrinking configurations
    int expand_amounts[] = {1, 2, 3, 4};
    int shrink_amounts[] = {1, 2, 3}; // Always less than initial
    int num_expand = sizeof(expand_amounts) / sizeof(expand_amounts[0]);
    int num_shrink = sizeof(shrink_amounts) / sizeof(shrink_amounts[0]);
    
    for (int e = 0; e < num_expand; e++) {
        for (int s = 0; s < num_shrink; s++) {
            int expand_to = size + expand_amounts[e];
            int shrink_to = size - shrink_amounts[s];
            
            if (shrink_to <= 0) continue; // Skip invalid configurations
            
            if (rank == 0) {
                printf("Testing: %d -> %d -> %d processes\n", size, expand_to, shrink_to);
                dmr_set_procs_next_expand(expand_to);
                dmr_set_procs_next_shrink(shrink_to);
            }
            
            // Run 2 cycles for each configuration
            for (int cycle = 0; cycle < 2; cycle++) {
                // Reset timers for each cycle to avoid overflow
                reset_timing_system();
                
                timing_result_t result = {0};
                result.initial_procs = size;
                result.expand_procs = expand_to;
                result.shrink_procs = shrink_to;
                result.cycle = cycle + 1;
                
                // Baseline measurement
                int baseline_timer = start_new_timer(DMR_WORLD_COMM);
                distributed_computation_work(rank, size);
                stop_timer(baseline_timer, DMR_WORLD_COMM);
                double baseline_time = get_elapsed_time(baseline_timer);
                
                // Start total timer
                int total_timer = start_new_timer(DMR_WORLD_COMM);
                
                // Expand operation
                int expand_timer = start_new_timer(DMR_WORLD_COMM);
                DMR_AUTO(dmr_check(SHOULD_EXPAND), (void)NULL, (void)NULL, (void)NULL);
                stop_timer(expand_timer, DMR_WORLD_COMM);
                result.expand_time = get_elapsed_time(expand_timer);
                
                // Update size after expansion
                MPI_Comm_rank(DMR_WORLD_COMM, &rank);
                MPI_Comm_size(DMR_WORLD_COMM, &size);
                
                // Computation with expanded processes
                int comp_timer = start_new_timer(DMR_WORLD_COMM);
                distributed_computation_work(rank, size);
                stop_timer(comp_timer, DMR_WORLD_COMM);
                result.computation_time = get_elapsed_time(comp_timer);
                
                // Shrink operation
                int shrink_timer = start_new_timer(DMR_WORLD_COMM);
                DMR_AUTO(dmr_check(SHOULD_SHRINK), (void)NULL, (void)NULL, (void)NULL);
                stop_timer(shrink_timer, DMR_WORLD_COMM);
                result.shrink_time = get_elapsed_time(shrink_timer);
                
                // Update size after shrinking
                MPI_Comm_rank(DMR_WORLD_COMM, &rank);
                MPI_Comm_size(DMR_WORLD_COMM, &size);
                
                stop_timer(total_timer, DMR_WORLD_COMM);
                result.total_time = get_elapsed_time(total_timer);
                result.overhead_percent = ((result.total_time - baseline_time) / baseline_time) * 100.0;
                
                if (rank == 0 && csv_file) {
                    write_timing_result_csv(csv_file, &result, "varying_expand_shrink");
                    fflush(csv_file);
                }
            }
        }
    }
    
    if (rank == 0 && csv_file) {
        fclose(csv_file);
        printf("Results written to: results/test2_varying_expand_shrink.csv\n");
    }
}

// Test 3: Multiple shrinking without expanding
void test_shrink_only(int argc, char *argv[]) {
    int rank, size;
    MPI_Comm_rank(DMR_WORLD_COMM, &rank);
    MPI_Comm_size(DMR_WORLD_COMM, &size);
    
    if (rank == 0) {
        printf("\n=== TEST 3: Shrink Only (No Expansion) ===\n");
        printf("Initial processes: %d\n", size);
        printf("Testing multiple shrinking operations\n\n");
    }
    
    FILE *csv_file = NULL;
    if (rank == 0) {
        csv_file = fopen("results/test3_shrink_only.csv", "w");
        if (csv_file) {
            write_csv_header(csv_file, "shrink_only");
        }
    }
    
    int original_size = size;
    
    // Test shrinking to different sizes
    while (size > 2) { // Keep at least 2 processes
        int target_size = size - 1;
        
        // Reset timers for each shrinking iteration
        reset_timing_system();
        
        if (rank == 0) {
            printf("Shrinking from %d to %d processes\n", size, target_size);
            dmr_set_procs_next_shrink(target_size);
        }
        
        // Computation before shrinking
        int comp_before_timer = start_new_timer(DMR_WORLD_COMM);
        distributed_computation_work(rank, size);
        MPI_Barrier(DMR_WORLD_COMM);
        stop_timer(comp_before_timer, DMR_WORLD_COMM);
        double comp_before_time = get_elapsed_time(comp_before_timer);
        
        // Total timer for this shrinking operation
        int total_timer = start_new_timer(DMR_WORLD_COMM);
        
        // Shrink operation
        int shrink_timer = start_new_timer(DMR_WORLD_COMM);
        DMR_AUTO(dmr_check(SHOULD_SHRINK), (void)NULL, (void)NULL, (void)NULL);
        stop_timer(shrink_timer, DMR_WORLD_COMM);
        double shrink_time = get_elapsed_time(shrink_timer);
        
        // Update size after shrinking
        MPI_Comm_rank(DMR_WORLD_COMM, &rank);
        MPI_Comm_size(DMR_WORLD_COMM, &size);
        
        // Computation after shrinking
        int comp_after_timer = start_new_timer(DMR_WORLD_COMM);
        distributed_computation_work(rank, size);
        MPI_Barrier(DMR_WORLD_COMM);
        stop_timer(comp_after_timer, DMR_WORLD_COMM);
        double comp_after_time = get_elapsed_time(comp_after_timer);
        
        stop_timer(total_timer, DMR_WORLD_COMM);
        double total_time = get_elapsed_time(total_timer);
        
        if (rank == 0 && csv_file) {
            write_shrink_only_csv(csv_file, original_size, size, shrink_time, 
                                comp_before_time, comp_after_time, total_time);
            fflush(csv_file);
        }
    }
    
    if (rank == 0 && csv_file) {
        fclose(csv_file);
        printf("Results written to: results/test3_shrink_only.csv\n");
    }
}

int main(int argc, char *argv[]) {
    // Initialize MPI environment
    MPI_Init(&argc, &argv);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    if (rank == 0) {
        printf("DMR Comprehensive Timing Tests\n");
        printf("==============================\n");
        printf("Starting with %d processes\n", size);
        printf("All results will be saved to CSV files in the results/ directory\n\n");
    }
    
    // Initialize DMR system
    DMR_AUTO(dmr_init(argc, argv), (void)NULL, (void)NULL, (void)NULL);
    
    // Update rank and size after DMR initialization
    MPI_Comm_rank(DMR_WORLD_COMM, &rank);
    MPI_Comm_size(DMR_WORLD_COMM, &size);
    
    // Initialize timing system
    init_timing_system();
    
    // Determine which test to run based on command line argument
    int test_number = 1; // Default to test 1
    if (argc > 1) {
        test_number = atoi(argv[1]);
    }
    
    switch (test_number) {
        case 1:
            test_varying_initial_processes(argc, argv);
            if (rank == 0) {
                printf("\n=== Test 1 Completed ===\n");
                printf("CSV file ready for plotting:\n");
                printf("- results/test1_varying_initial.csv\n");
            }
            break;
        case 2:
            test_varying_expand_shrink(argc, argv);
            if (rank == 0) {
                printf("\n=== Test 2 Completed ===\n");
                printf("CSV file ready for plotting:\n");
                printf("- results/test2_varying_expand_shrink.csv\n");
            }
            break;
        case 3:
            test_shrink_only(argc, argv);
            if (rank == 0) {
                printf("\n=== Test 3 Completed ===\n");
                printf("CSV file ready for plotting:\n");
                printf("- results/test3_shrink_only.csv\n");
            }
            break;
        default:
            if (rank == 0) {
                printf("Running all tests...\n");
            }
            test_varying_initial_processes(argc, argv);
            init_timing_system(); // Reset timers
            test_varying_expand_shrink(argc, argv);
            init_timing_system(); // Reset timers
            test_shrink_only(argc, argv);
            if (rank == 0) {
                printf("\n=== All Tests Completed ===\n");
                printf("CSV files are ready for plotting:\n");
                printf("- results/test1_varying_initial.csv\n");
                printf("- results/test2_varying_expand_shrink.csv\n");
                printf("- results/test3_shrink_only.csv\n");
            }
            break;
    }
    
    // Finalize DMR system
    DMR_AUTO(dmr_finalize(), (void)NULL, (void)NULL, (void)NULL);
    
    // Finalize MPI environment
    MPI_Finalize();
    
    return EXIT_SUCCESS;
}
