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
- `monitor_timing.sh` - Script for monitoring benchmark execution

## Configuration

The benchmark behavior can be configured by modifying constants in `timing.h`:

```c
#define MAX_ITERATIONS 1000        // Maximum number of iterations
#define CHECKPOINT_INTERVAL 100    // Checkpoint every N iterations  
#define MESSAGE_SIZE 1024          // Communication test message size
#define WORKLOAD_SIZE 10000        // Computational workload size
```

## Building

Build the timing example using make:

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
