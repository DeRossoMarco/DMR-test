#!/bin/bash
#SBATCH --job-name=dmr-timing
#SBATCH --output=results/timing-%j.out
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=8
#SBATCH --time=00:30:00
#SBATCH --no-kill

# DMR Timing Performance Benchmark SLURM Script
# This script runs the timing performance benchmark with different configurations

echo "DMR Timing Performance Benchmark"
echo "================================="
echo "Job ID: $SLURM_JOB_ID"
echo "Nodes: $SLURM_NNODES"
echo "Tasks per node: $SLURM_NTASKS_PER_NODE"
echo "Total tasks: $SLURM_NTASKS"
echo "Start time: $(date)"
echo ""

# Ensure results directory exists
mkdir -p results

if [ $? -ne 0 ]; then
    echo "Error: Failed to build timing example"
    exit 1
fi

echo "Build successful."
echo ""

# Run the timing benchmark
echo "Starting timing performance benchmark..."
echo "Run 1: Standard configuration"
dmr_wrapper mpirun --with-ft ulfm -np $SLURM_NTASKS ./timing

echo ""
echo "Run 2: Reduced process count for comparison"
dmr_wrapper mpirun --with-ft ulfm -np 4 ./timing

echo ""
echo "Run 3: Single node configuration"
dmr_wrapper mpirun --with-ft ulfm -np 2 ./timing

echo ""
echo "Timing benchmark completed."
echo "End time: $(date)"

# Display final results summary
echo ""
echo "Results Summary:"
echo "================"
echo "Check the following files for detailed results:"
find results/ -name "timing_*.out" -newer results/timing-$SLURM_JOB_ID.out 2>/dev/null | head -5
