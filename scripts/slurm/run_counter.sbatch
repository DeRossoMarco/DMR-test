#!/bin/bash
#SBATCH --job-name=dmr-counter
#SBATCH --output=results/counter-%j.out
#SBATCH --ntasks=16
#SBATCH --cpus-per-task=1
#SBATCH --mem-per-cpu=2G
#SBATCH --oversubscribe
#SBATCH --time=01:00:00
#SBATCH --no-kill

# DMR Counter Performance Benchmark SLURM Script
# This script runs the counter performance benchmark with different configurations

echo "DMR Counter Performance Benchmark"
echo "=================================="
echo "Job ID: $SLURM_JOB_ID"
echo "Nodes: $SLURM_NNODES"
echo "Tasks per node: $SLURM_NTASKS_PER_NODE"
echo "Total tasks: $SLURM_NTASKS"
echo "Start time: $(date)"
echo ""

# Get project root - use current directory since SLURM starts from where sbatch was called
PROJECT_ROOT="$(pwd)"
cd "$PROJECT_ROOT"

# Ensure results directory exists
mkdir -p results

if [ ! -f "./counter" ]; then
    echo "Error: counter executable not found. Please build first."
    exit 1
fi

echo "Build verified."
echo ""

# Run the counter benchmark
echo "Starting counter performance benchmark..."
echo "Run 1: Standard configuration"
dmr_wrapper mpirun --with-ft ulfm -np $SLURM_NTASKS ./counter

echo ""
echo "Counter benchmark completed."
echo "End time: $(date)"

# Return to project root for results
cd "$PROJECT_ROOT"

# Display final results summary
echo ""
echo "Results Summary:"
echo "================"
echo "Check the following files for detailed results:"
find checkpoints/ -name "counter_*.out" -newer results/counter-$SLURM_JOB_ID.out 2>/dev/null | head -5