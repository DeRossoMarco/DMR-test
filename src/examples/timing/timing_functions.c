#include "timing.h"

// Global variables to store timing data for multiple timing sessions
#define MAX_TIMERS 3
static double start_times[MAX_TIMERS];
static double end_times[MAX_TIMERS];
static int active_timers[MAX_TIMERS];
static double accumulated_times[MAX_TIMERS];  // Accumulate elapsed time across sessions

// Initialize timing system
void init_timing_system()
{
    for (int i = 0; i < MAX_TIMERS; i++) {
        start_times[i] = 0.0;
        end_times[i] = 0.0;
        active_timers[i] = 0;
        accumulated_times[i] = 0.0;
    }
}

// Start a new timer
void start_timer(int timer_id, MPI_Comm comm)
{
    if (timer_id < 0 || timer_id >= MAX_TIMERS) {
        return;
    }
    
    // If timer is already active, don't restart it
    if (active_timers[timer_id]) {
        return;
    }
    
    // Synchronize all processes before starting the timer
    MPI_Barrier(comm);
    
    // Record the start time using MPI_Wtime for high precision
    start_times[timer_id] = MPI_Wtime();
    active_timers[timer_id] = 1;
}

// Stop a specific timer
void stop_timer(int timer_id, MPI_Comm comm)
{
    if (timer_id < 0 || timer_id >= MAX_TIMERS || !active_timers[timer_id]) {
        return;
    }

    // Synchronize all processes after stopping the timer
    MPI_Barrier(comm);

    // Record the end time
    end_times[timer_id] = MPI_Wtime();
    
    // Accumulate elapsed time from this timing session
    accumulated_times[timer_id] += end_times[timer_id] - start_times[timer_id];
    
    active_timers[timer_id] = 0;
}

// Get elapsed time for a specific timer (including accumulated time from previous sessions)
double get_elapsed_time(int timer_id)
{
    if (timer_id < 0 || timer_id >= MAX_TIMERS) {
        fprintf(stderr, "Error: Invalid timer ID %d\n", timer_id);
        return -1.0;
    }
    
    if (active_timers[timer_id]) {
        // Timer is still running, return accumulated time plus current session elapsed time
        return accumulated_times[timer_id] + (MPI_Wtime() - start_times[timer_id]);
    } else {
        // Timer has stopped, return total accumulated time
        return accumulated_times[timer_id];
    }
}

// Get only the accumulated time from previous sessions (excludes current running session)
double get_accumulated_time(int timer_id)
{
    if (timer_id < 0 || timer_id >= MAX_TIMERS) {
        fprintf(stderr, "Error: Invalid timer ID %d\n", timer_id);
        return -1.0;
    }
    
    return accumulated_times[timer_id];
}

// Internal helper: gather elapsed time without printing errors (assumes valid id)
static inline double _raw_elapsed(int timer_id) {
    if (active_timers[timer_id]) {
        return accumulated_times[timer_id] + (MPI_Wtime() - start_times[timer_id]);
    } else {
        return accumulated_times[timer_id];
    }
}

int compute_timer_stats(int timer_id, MPI_Comm comm, timer_stats_t *stats)
{
    if (!stats) return -1;
    if (timer_id < 0 || timer_id >= MAX_TIMERS) return -2;
    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    double local = _raw_elapsed(timer_id);
    double minv = 0.0, maxv = 0.0, sumv = 0.0;
    MPI_Reduce(&local, &minv, 1, MPI_DOUBLE, MPI_MIN, 0, comm);
    MPI_Reduce(&local, &maxv, 1, MPI_DOUBLE, MPI_MAX, 0, comm);
    MPI_Reduce(&local, &sumv, 1, MPI_DOUBLE, MPI_SUM, 0, comm);
    double stddev = 0.0;
    // Compute average locally on each rank from the sum
    double avg_local = sumv / (double)size;
    double diff = local - avg_local;
    double sq = diff * diff;
    double sumsq = 0.0;
    MPI_Reduce(&sq, &sumsq, 1, MPI_DOUBLE, MPI_SUM, 0, comm);
    if (rank == 0) {
        stddev = sqrt(sumsq / (double)size);
        stats->min = minv;
        stats->max = maxv;
        stats->avg = avg_local;
        stats->stddev = stddev;
        stats->sum = sumv;
        stats->count = size;
    }
    return 0;
}

void report_timer_stats(int timer_id, MPI_Comm comm, const char *label)
{
    int rank;
    MPI_Comm_rank(comm, &rank);
    timer_stats_t s;
    if (compute_timer_stats(timer_id, comm, &s) == 0 && rank == 0) {
        printf("[TIMER %d] %-12s min=%g max=%g avg=%g stddev=%g sum=%g n=%d\n",
               timer_id, label ? label : "(unnamed)", s.min, s.max, s.avg, s.stddev, s.sum, s.count);
    }
}

// Binary persistence of timing data
// File format (all little-endian native, not portable across arch with different endianness):
// struct Header {
//   char magic[8] = "TMRBIN\0"; // 7 chars + NUL
//   int  version = 2;           // updated version for accumulated timing
//   int  max_timers;            // sanity check (should equal MAX_TIMERS)
// };
// Then arrays of length max_timers:
//   double start_times[max_timers];
//   double end_times[max_timers];
//   int    active_timers[max_timers];
//   double accumulated_times[max_timers];

typedef struct {
    char magic[8];
    int version;
    int max_timers;
} TimerBinHeader;

int save_timers_binary(const char *filename, MPI_Comm comm)
{
    int rank = 0;
    if (comm != MPI_COMM_NULL) {
        MPI_Comm_rank(comm, &rank);
    }
    int rc = 0;
    
    // Before saving, accumulate any currently active timers
    double current_time = MPI_Wtime();
    double temp_accumulated[MAX_TIMERS];
    for (int i = 0; i < MAX_TIMERS; i++) {
        temp_accumulated[i] = accumulated_times[i];
        if (active_timers[i]) {
            temp_accumulated[i] += current_time - start_times[i];
        }
    }
    
    if (rank == 0) {
        FILE *f = fopen(filename, "wb");
        if (!f) {
            fprintf(stderr, "Could not open %s for writing timing data.\n", filename);
            rc = -1;
        } else {
            TimerBinHeader hdr;
            memset(&hdr, 0, sizeof(hdr));
            strncpy(hdr.magic, "TMRBIN\0", sizeof(hdr.magic)-1);
            hdr.version = 2;  // Updated version for accumulated timing
            hdr.max_timers = MAX_TIMERS;
            size_t wrote = 0;
            wrote = fwrite(&hdr, sizeof(hdr), 1, f);
            if (wrote != 1) rc = -2;
            if (rc == 0 && fwrite(start_times, sizeof(double), MAX_TIMERS, f) != (size_t)MAX_TIMERS) rc = -3;
            if (rc == 0 && fwrite(end_times, sizeof(double), MAX_TIMERS, f) != (size_t)MAX_TIMERS) rc = -4;
            if (rc == 0 && fwrite(active_timers, sizeof(int), MAX_TIMERS, f) != (size_t)MAX_TIMERS) rc = -5;
            if (rc == 0 && fwrite(temp_accumulated, sizeof(double), MAX_TIMERS, f) != (size_t)MAX_TIMERS) rc = -6;
            fclose(f);
        }
    }
    return rc;
}

int load_timers_binary(const char *filename, MPI_Comm comm)
{
    int rank = 0;
    if (comm != MPI_COMM_NULL) MPI_Comm_rank(comm, &rank);
    int rc = 0;
    TimerBinHeader hdr;

    FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "[rank %d] Could not open %s for reading timing data.\n", rank, filename);
        return -1;
    }

    if (fread(&hdr, sizeof(hdr), 1, f) != 1) {
        rc = -2;
    } else if (strncmp(hdr.magic, "TMRBIN", 6) != 0) {
        rc = -3; // bad header magic
    } else if (hdr.max_timers != MAX_TIMERS) {
        rc = -4; // mismatched build/runtime constant
    } else {
        if (fread(start_times, sizeof(double), MAX_TIMERS, f) != (size_t)MAX_TIMERS) rc = -5;
        if (rc == 0 && fread(end_times, sizeof(double), MAX_TIMERS, f) != (size_t)MAX_TIMERS) rc = -6;
        if (rc == 0 && fread(active_timers, sizeof(int), MAX_TIMERS, f) != (size_t)MAX_TIMERS) rc = -7;
        
        // Handle version differences for backward compatibility
        if (rc == 0) {
            if (hdr.version >= 2) {
                // Version 2: read accumulated times
                if (fread(accumulated_times, sizeof(double), MAX_TIMERS, f) != (size_t)MAX_TIMERS) rc = -8;
            } else if (hdr.version == 1) {
                // Version 1: initialize accumulated times to zero (no accumulation support)
                for (int i = 0; i < MAX_TIMERS; i++) {
                    accumulated_times[i] = 0.0;
                }
            } else {
                rc = -9; // unsupported version
            }
        }
        
        // After loading, restart any active timers from current time
        double current_time = MPI_Wtime();
        for (int i = 0; i < MAX_TIMERS; i++) {
            if (active_timers[i]) {
                start_times[i] = current_time;
                end_times[i] = 0.0;  // Clear end time for active timer
            }
        }
    }
    fclose(f);

    return rc;
}

