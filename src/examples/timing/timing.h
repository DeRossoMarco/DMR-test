#ifndef TIMING_H
#define TIMING_H

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#include "dmr.h"

// Advanced timing functions for comprehensive performance analysis
// Timer ID conventions (fixed mapping):
//   0 -> total runtime
//   1 -> expansion phase cumulative time
//   2 -> shrink phase cumulative time
void init_timing_system(void);
void start_timer(int timer_id, MPI_Comm comm);
void stop_timer(int timer_id, MPI_Comm comm);
double get_elapsed_time(int timer_id);
int save_timers_binary(const char *filename, MPI_Comm comm);
int load_timers_binary(const char *filename, MPI_Comm comm);

// Collective statistics for a timer across all ranks in a communicator
// The statistics are computed using MPI_Reduce on rank 0 only.
// count: number of ranks participating (size of comm)
typedef struct {
	double min;
	double max;
	double avg;
	double stddev; // population standard deviation
	double sum;    // sum of elapsed times over ranks
	int    count;  // number of ranks
} timer_stats_t;

// Compute stats for a given timer id. Returns 0 on success, non-zero on error.
// Only rank 0 receives filled stats; other ranks have unspecified contents.
int compute_timer_stats(int timer_id, MPI_Comm comm, timer_stats_t *stats);
// Convenience: rank 0 prints a formatted line for the timer.
void report_timer_stats(int timer_id, MPI_Comm comm, const char *label);

// Computation function
void distributed_computation_work(int rank, int size);

#endif // TIMING_H

