#ifndef TIMING_H
#define TIMING_H

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <string.h>
#include <math.h>

#include "dmr.h"

// Advanced timing functions for comprehensive performance analysis
void init_timing_system(void);
void reset_timing_system(void);
int start_new_timer(MPI_Comm comm);
int stop_timer(int timer_id, MPI_Comm comm);
double get_elapsed_time(int timer_id);
void report_timer_stats(int timer_id, MPI_Comm comm, const char* timer_name);
void report_all_timers(MPI_Comm comm);
double benchmark_section(MPI_Comm comm, void (*function_to_benchmark)(void), const char* section_name);

// Computation functions for expand/shrink timing analysis
void computation_heavy(void);
void computation_light(void);
void computation_medium(void);
void distributed_computation_work(int rank, int size);

#endif // TIMING_H

