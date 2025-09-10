#include "timing.h"
#include "dmr.h"

// File-scope helper to append CSV timing results (path provided via CLI)
static void append_result_line(int rank, const char *csv_path,
                               int initial_size, int exp_amt, int shr_amt,
                               double expand_t, double shrink_t, double total_t) {
    if (rank != 0 || !csv_path) return;
    FILE *af = fopen(csv_path, "a");
    if (af) {
        fprintf(af, "%d,%d,%d,%.9f,%.9f,%.9f\n", initial_size, exp_amt, shr_amt, expand_t, shrink_t, total_t);
        fclose(af);
    } else {
        fprintf(stderr, "Rank 0: Failed to append to %s.\n", csv_path);
    }
}

// Simple checkpoint: store current iteration (and optionally size) to a global file
static void timing_checkpoint(int rank, int current_iter, const char *ckpt_file) {
    if (rank != 0) return;
    FILE *cfw = fopen(ckpt_file, "w");
    if (!cfw) {
        fprintf(stderr, "Rank 0: Failed to write checkpoint %s\n", ckpt_file);
        return;
    }
    // Store only iteration for robustness. (Initial size kept separate.)
    fprintf(cfw, "%d\n", current_iter);
    fclose(cfw);

    save_timers_binary("checkpoints/timing_checkpoint_timers.bin", DMR_WORLD_COMM);
}

// Restart callback used with DMR_AUTO after reconfiguration
static void timing_restart(int *start_iter, const char *ckpt_file, int *expand_amount, int expand_iter, int *shrink_amount, int shrink_iter) {
    int rank;
    MPI_Comm_rank(DMR_WORLD_COMM, &rank);
    // Each rank independently opens and reads the checkpoint file (no broadcast)
    FILE *cf = fopen(ckpt_file, "r");
    if (cf) {
        int stored_iter = 0;
        if (fscanf(cf, "%d", &stored_iter) == 1) {
            *start_iter = stored_iter;
        }
        fclose(cf);
    } else {
        // Silent failure is acceptable (fresh start); could log on one rank if desired
    }

    // Load timers (each rank reads its own copy now)
    load_timers_binary("checkpoints/timing_checkpoint_timers.bin", DMR_WORLD_COMM);
}

// Distributed computation that works across multiple processes
void distributed_computation_work(int rank, int size)
{
    // Define total work to be distributed among all processes
    const int total_work = 256;  // Total iterations to distribute
    
    // Calculate work chunk size per process
    int work_per_process = total_work / size;
    int remainder = total_work % size;
    
    // Distribute work based on rank and size
    int work_start = rank * work_per_process;
    int work_end = work_start + work_per_process;
    
    // Distribute remainder work to the first few processes
    if (rank < remainder) {
        work_start += rank;
        work_end += rank + 1;
    } else {
        work_start += remainder;
        work_end += remainder;
    }
    
    for (int i = work_start; i < work_end; i++) {
        sleep(2); // Simulate work
    }
}

// Helper: parse simple key=value plan file (lines: key=value, comments starting with #)
static void parse_plan_file(const char *path, int rank,
                            int *expand_amount, int *shrink_amount,
                            int *expand_iter, int *shrink_iter,
                            int *total_iters, int *initial_np) {
    if (!path) return;
    FILE *pf = fopen(path, "r");
    if (!pf) {
        if (rank == 0) fprintf(stderr, "Could not open plan file %s\n", path);
        return;
    }
    char line[256];
    while (fgets(line, sizeof(line), pf)) {
        char *p = line;
        while (*p==' ' || *p=='\t') ++p;
        if (*p=='#' || *p=='\n' || *p=='\0') continue;
        char key[64];
        char value[64];
        if (sscanf(p, "%63[^=]=%63s", key, value) == 2) {
            if (strcmp(key, "expand") == 0)      *expand_amount = atoi(value);
            else if (strcmp(key, "shrink") == 0) *shrink_amount = atoi(value);
            else if (strcmp(key, "expand_iter") == 0) *expand_iter = atoi(value);
            else if (strcmp(key, "shrink_iter") == 0) *shrink_iter = atoi(value);
            else if (strcmp(key, "iters") == 0) *total_iters = atoi(value);
            else if (strcmp(key, "initial_np") == 0) *initial_np = atoi(value);
        }
    }
    
    fclose(pf);
}

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int expand_amount = 0;      // effective expansion amount (may be zeroed after restart)
    int shrink_amount = 0;      // effective shrink amount (may be zeroed after restart)
    int total_iters = 10;       // total compute iterations (may be overridden by plan)
    int expand_iter = 2;        // iteration index at which to trigger expansion (0-based)
    int shrink_iter = 7;        // iteration index at which to trigger shrink (0-based)

    const char *csv_path = NULL;
    const char *ckpt_path = NULL;
    int initial_np = -1;        // may be overridden by plan; if still -1 after init, set to size
    const char *plan_path = NULL; // optional plan file overriding expand/shrink params

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--csv") == 0 && i + 1 < argc) csv_path = argv[++i];
        else if (strcmp(argv[i], "--ckpt") == 0 && i + 1 < argc) ckpt_path = argv[++i];
        else if (strcmp(argv[i], "--plan") == 0 && i + 1 < argc) plan_path = argv[++i];
        else if (strcmp(argv[i], "--help") == 0) {
            if (rank == 0) {
                printf("Usage: ./timing --csv <file> --ckpt <file> [--plan file]\n");
                printf("  Iteration count, initial_np, and any expansion/shrink behavior now come solely from the optional plan file.\n");
                printf("  Plan file lines: key=value (expand, shrink, expand_iter, shrink_iter, iters, initial_np). Comments with #.\n");
                printf("  CSV file must exist with header (created by script). Program appends one line.\n");
                printf("  Checkpoint file path is required; program writes iteration index each loop.\n");
                printf("  --plan file sets expand/shrink behavior (if omitted, no reconfiguration occurs unless defaults changed in source).\n");
            }
            MPI_Finalize();
            return 0;
        }
    }

    // If a plan file provided, parse after CLI so file can override (script-driven)
    parse_plan_file(plan_path, rank, &expand_amount, &shrink_amount,
                    &expand_iter, &shrink_iter, &total_iters, &initial_np);

    // If initial_np was not set by plan file, use current size
    if (initial_np == -1) {
        initial_np = size;
    }

    if (csv_path == NULL || ckpt_path == NULL) {
        if (rank == 0) fprintf(stderr, "Error: --csv and --ckpt paths are required.\n");
        MPI_Finalize();
        return 1;
    }

    if (rank == 0) {
        printf("DMR Timing Single Run (pre-init): req_expand=%d (iter=%d) req_shrink=%d (iter=%d) total_iters=%d\n",
               expand_amount, expand_iter, shrink_amount, shrink_iter, total_iters);
        if (plan_path) {
            printf("Loaded plan file: %s\n", plan_path);
        }
    }

    init_timing_system();

    int start_iter = 0;           // will be set by timing_restart if resuming
    DMR_AUTO(dmr_init(argc, argv), (void)NULL,
             timing_restart(&start_iter, ckpt_path, &expand_amount, expand_iter, &shrink_amount, shrink_iter),
             (void)NULL);

    // Set desired dynamic reconfiguration targets (only rank 0) with guards
    if (rank == 0) {
        if (expand_amount > 0) dmr_set_procs_next_expand(expand_amount);
        if (shrink_amount > 0) dmr_set_procs_next_shrink(shrink_amount);
    }

    if (start_iter > 0 && rank == 0) {
        printf("Resumed from checkpoint via DMR restart callback: start_iter=%d (total=%d)\n", start_iter, total_iters);
    }
    
    // Always start timer 0 if it's not already running (handles both fresh start and restart)
    if (dmr_get_reconfig_count() == 0 || start_iter > 0) {
        start_timer(0, DMR_WORLD_COMM);
    }

    MPI_Barrier(DMR_WORLD_COMM);

    for (int iter = start_iter; iter < total_iters; ++iter) {
        printf("Rank %d starting iteration %d\n", rank, iter);
        // After potential reconfiguration completes, stop phase timers if they were started this iteration
        if (dmr_get_reconfig_count() == 1) {
                stop_timer(1, DMR_WORLD_COMM);
            }
        if (dmr_get_reconfig_count() == 2 || iter == (shrink_iter + 1)) {
                stop_timer(2, DMR_WORLD_COMM);
            }

        distributed_computation_work(rank, size);

        DMRSuggestion suggestion = SHOULD_STAY;

        if (iter == expand_iter && expand_amount > 0) {
            suggestion = SHOULD_EXPAND;
            start_timer(1, DMR_WORLD_COMM);
        }
        if (iter == shrink_iter && shrink_amount > 0) {
            suggestion = SHOULD_SHRINK;
            start_timer(2, DMR_WORLD_COMM);
        }

        MPI_Barrier(DMR_WORLD_COMM);
        DMR_AUTO(dmr_check(suggestion), timing_checkpoint(rank, iter + 1, ckpt_path), timing_restart(&start_iter, ckpt_path, &expand_amount, expand_iter, &shrink_amount, shrink_iter), (void)NULL);
    }

    stop_timer(0, DMR_WORLD_COMM);
    double total_time = get_elapsed_time(0);
    double expand_time = get_elapsed_time(1);
    double shrink_time = get_elapsed_time(2);
    report_timer_stats(0, DMR_WORLD_COMM, "total");
    if (expand_amount > 0) report_timer_stats(1, DMR_WORLD_COMM, "expand");
    if (shrink_amount > 0) report_timer_stats(2, DMR_WORLD_COMM, "shrink");
    append_result_line(rank, csv_path, initial_np, expand_amount, shrink_amount, expand_time, shrink_time, total_time);
    dmr_finalize();
    MPI_Finalize();
    return EXIT_SUCCESS;
}
