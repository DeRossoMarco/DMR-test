# DMR Timing Performance Benchmark

This example provides a comprehensive timing performance benchmark for the DMR (Dynamic Management of Resources) system. It measures various performance metrics in a distributed MPI environment with checkpointing and dynamic reconfiguration capabilities.

## Overview

The timing benchmark measures the following performance aspects:

1. **Computation Performance**: Time spent in computational work
2. **Communication Performance**: Time spent in MPI communication operations  
3. **Checkpoint Performance**: Time required for saving checkpoints
4. **Restart Performance**: Time required for loading checkpoints
5. **Reconfiguration Performance**: Time spent handling DMR reconfiguration suggestions

## Files

- `timing.h` - Header file with function declarations and data structures
- `timing.c` - Main program implementing the benchmark loop
- `timing_functions.c` - Implementation of timing measurement functions
- `run_timing.sbatch` - SLURM batch script for running the benchmark

## Configuration

The benchmark behavior can be configured by modifying constants in `timing.h`:

```c
#define MAX_ITERATIONS 1000        // Maximum number of iterations
#define CHECKPOINT_INTERVAL 100    // Checkpoint every N iterations  
#define MESSAGE_SIZE 1024          // Communication test message size
#define WORKLOAD_SIZE 10000        // Computational workload size
```

## Building

Build the # DMR Timing Examples

This directory contains examples demonstrating custom timing functionality for the DMR (Distributed Memory Replication) library.

## Overview

Since DMR doesn't have built-in timing functions, we've implemented custom timing utilities that provide:

- **High-precision timing** using `MPI_Wtime()`
- **Synchronization** across all MPI processes
- **Statistical analysis** of timing data (min, max, average, standard deviation)
- **Load imbalance detection**
- **Multiple timer support** for complex applications

## Files

- `timing.h` - Header file with timing function declarations
- `timing.c` - Basic timing example with simple start/stop/report functions
- `timing_functions.c` - Advanced timing utilities implementation
- `timing_advanced_example.c` - Comprehensive example showing all timing features

## Basic Usage

### Simple Timing (timing.c)

```c
#include "timing.h"

// Start timing
start_timing(DMR_WORLD_COMM);

// Your computation here
for (int i = 0; i < 1000000; i++) {
    volatile int temp = i * i;
}

// Stop timing and report results
stop_timing(DMR_WORLD_COMM);
report_timing(DMR_WORLD_COMM);
```

### Advanced Timing (timing_functions.c)

```c
#include "timing.h"

// Initialize timing system
init_timing_system();

// Start a new timer
int timer_id = start_new_timer(DMR_WORLD_COMM);

// Your computation here
computation_function();

// Stop the timer
stop_timer(timer_id, DMR_WORLD_COMM);

// Report detailed statistics
report_timer_stats(timer_id, DMR_WORLD_COMM, "My Computation");
```

## Building and Running

### Build the examples:

```bash
# Build basic timing example
make timing

# Build advanced timing example  
make timing-advanced

# Build all examples
make all
```

### Run the examples:

```bash
# Run basic timing example
mpirun -np 4 ./timing

# Run advanced timing example
mpirun -np 4 ./timing-advanced
```

## Timing Functions Reference

### Basic Functions

- `start_timing(MPI_Comm comm)` - Start timing for simple use cases
- `stop_timing(MPI_Comm comm)` - Stop timing
- `report_timing(MPI_Comm comm)` - Report timing statistics

### Advanced Functions

- `init_timing_system()` - Initialize the timing system
- `start_new_timer(MPI_Comm comm)` - Start a new timer, returns timer ID
- `stop_timer(int timer_id, MPI_Comm comm)` - Stop a specific timer
- `get_elapsed_time(int timer_id)` - Get elapsed time for a timer
- `report_timer_stats(int timer_id, MPI_Comm comm, const char* name)` - Report detailed statistics
- `report_all_timers(MPI_Comm comm)` - Report all timer results
- `benchmark_section(MPI_Comm comm, void (*func)(void), const char* name)` - Benchmark a function

## Features

### Statistical Analysis
The timing functions provide comprehensive statistics:
- Minimum execution time across all processes
- Maximum execution time across all processes  
- Average execution time
- Standard deviation
- Load imbalance percentage

### Multiple Timers
Support for up to 10 concurrent timers, allowing you to:
- Time different phases of your application
- Create nested timing measurements
- Compare performance of different algorithms

### Synchronization
All timing functions use `MPI_Barrier()` to ensure:
- Synchronized start times across all processes
- Accurate measurement of parallel sections
- Consistent timing results

## Example Output

```
=== Timing Results for Heavy Computation ===
Timer ID: 2
Number of processes: 4
Minimum time: 0.234567 seconds
Maximum time: 0.245123 seconds
Average time: 0.239845 seconds
Standard deviation: 0.004123 seconds
Total cumulative time: 0.959380 seconds
Load imbalance: 4.41%
===============================
```

## Performance Tips

1. **Use MPI_Wtime()** - Provides high-precision timing
2. **Minimize timing overhead** - Don't time very short operations
3. **Consider load balancing** - High standard deviation indicates imbalance
4. **Use barriers wisely** - They ensure synchronization but add overhead
5. **Time meaningful sections** - Focus on computationally expensive parts

## Integration with DMR

These timing utilities are designed to work seamlessly with DMR applications:
- Use `DMR_WORLD_COMM` for timing across all DMR processes
- Compatible with DMR's MPI-based architecture
- Can be used to measure DMR replication overhead
- Suitable for performance analysis of fault-tolerant applications using make:

```bash
make timing
```

Or build all examples:

```bash
make all
```

## Running

### Local Execution

Run directly with MPI:

```bash
mpirun -np 4 ./timing
```

### SLURM Execution

Submit to SLURM job scheduler:

```bash
sbatch run_timing.sbatch
```

### Monitoring

Monitor the benchmark execution in real-time:

```bash
./scripts/monitor_timing.sh
```

## Output

The benchmark produces several types of output:

### Console Output
- Real-time progress information
- Performance statistics summary
- Error messages and diagnostics

### Result Files
Results are automatically saved to timestamped files in the `results/` directory:
- Format: `timing_YYYYMMDD_HHMMSS.out`
- Contains detailed performance metrics in CSV format

### Checkpoint Files
Checkpoint data is saved to the `checkpoints/` directory:
- Format: `timing.XXX` (where XXX is the MPI rank)
- Used for restart functionality

## Performance Metrics

### Computation Metrics
- **Total computation time**: Cumulative time spent in computational work
- **Average computation time**: Mean time per computation cycle
- **Min/Max computation time**: Performance bounds

### Communication Metrics  
- **All-to-all communication**: Time for collective operations
- **Broadcast operations**: Time for one-to-all communication
- **Point-to-point communication**: Time for peer-to-peer messaging
- **Reduction operations**: Time for aggregation operations

### Checkpoint/Restart Metrics
- **Checkpoint save time**: Time to write checkpoint data
- **Checkpoint load time**: Time to read checkpoint data  
- **Checkpoint frequency**: Number of checkpoints taken
- **Restart frequency**: Number of restart operations

### Reconfiguration Metrics
- **Reconfiguration time**: Time to handle DMR suggestions
- **Expansion operations**: Time for adding processes
- **Shrinking operations**: Time for removing processes
- **Reconfiguration frequency**: Number of reconfigurations

## Example Output

```
========================================
    DMR TIMING PERFORMANCE RESULTS     
========================================

Overall Statistics:
  Total iterations completed: 1000
  Overall runtime: 45.234567 seconds

Computation Performance:
  Total time: 12.345678 seconds
  Average time: 0.012346 seconds
  Min time: 0.009876 seconds
  Max time: 0.015432 seconds
  Samples: 1000

Communication Performance:
  Total time: 8.765432 seconds
  Average time: 0.008765 seconds
  Min time: 0.006543 seconds
  Max time: 0.012345 seconds
  Samples: 1000

Performance Ratios:
  Computation ratio: 27.30%
  Communication ratio: 19.38%
  Checkpoint ratio: 2.15%
  Restart ratio: 0.00%
  Reconfiguration ratio: 0.50%
```

## Customization

### Adding New Metrics

To add new performance measurements:

1. Add timing variables to `timing_data_t` structure in `timing.h`
2. Add corresponding statistics to `timing_results_t` structure  
3. Implement measurement functions in `timing_functions.c`
4. Update collection and reporting functions

### Modifying Workloads

Computational and communication workloads can be modified in:
- `perform_computation_work()` - Adjust computational intensity
- `perform_communication_test()` - Modify communication patterns

### Custom Communication Patterns

The communication test includes:
- All-to-all exchange
- Broadcast operations  
- Reduction operations
- Ring-based point-to-point communication

Additional patterns can be added to `perform_communication_test()`.

## Troubleshooting

### Common Issues

1. **Build Errors**: Ensure DMR library and MPI are properly installed
2. **Permission Errors**: Check write permissions for checkpoints/ and results/ directories
3. **Memory Issues**: Adjust MESSAGE_SIZE and WORKLOAD_SIZE for available memory
4. **MPI Errors**: Verify MPI environment is properly configured

### Debugging

Enable debug output by adding debug prints or using MPI debugging tools:

```bash
mpirun -np 4 gdb ./timing
```

### Log Files

Check SLURM output files for detailed execution logs:
- `results/timing-<jobid>.out` - Standard output
- `results/timing-<jobid>.err` - Error output

## Performance Analysis

### Interpreting Results

- **High computation ratio**: CPU-intensive workload
- **High communication ratio**: Network-bound application  
- **High checkpoint ratio**: Frequent checkpointing overhead
- **Reconfiguration events**: Dynamic adaptation activity

### Optimization Opportunities

- Adjust checkpoint frequency based on checkpoint ratio
- Optimize communication patterns if communication ratio is high
- Balance workload distribution across processes
- Tune DMR reconfiguration thresholds

## Integration

This timing benchmark can be integrated with:
- Performance monitoring systems
- Automated testing frameworks  
- Continuous integration pipelines
- Cluster scheduling systems

The CSV output format enables easy integration with analysis tools and visualization software.
