#!/bin/bash
#SBATCH --job-name=dmr-test1-varying-procs
#SBATCH --output=results/test1-varying-procs-%j.out
#SBATCH --ntasks=12
#SBATCH --cpus-per-task=1
#SBATCH --mem-per-cpu=2G
#SBATCH --oversubscribe
#SBATCH --time=01:00:00
#SBATCH --no-kill

# DMR Test 1: Fixed expansion/shrinking with varying initial processes

echo "DMR Test 1: Fixed Expansion/Shrinking with Varying Initial Processes"
echo "==================================================================="
echo "Job ID: $SLURM_JOB_ID"
echo "Total tasks: $SLURM_NTASKS"
echo "Start time: $(date)"
echo ""

# Ensure results directory exists
mkdir -p results

# Build the test
make comprehensive_timing

if [ $? -ne 0 ]; then
    echo "Error: Failed to build comprehensive timing test"
    exit 1
fi

echo "Test Configuration:"
echo "- Fixed expansion: +2 processes"
echo "- Fixed shrinking: -1 process"
echo "- Multiple cycles with different starting process counts"
echo ""

# Run Test 1 with different process counts
for procs in 4 6 8 10 12; do
    if [ $procs -le $SLURM_NTASKS ]; then
        echo "Running Test 1 with $procs initial processes..."
        dmr_wrapper mpirun --with-ft ulfm -np $procs ./comprehensive_timing 1
        
        if [ $? -eq 0 ]; then
            echo "✓ Test 1 completed successfully with $procs processes"
        else
            echo "✗ Test 1 failed with $procs processes"
        fi
        
        sleep 3
        echo ""
    else
        echo "Skipping $procs processes (exceeds allocated: $SLURM_NTASKS)"
    fi
done

echo "Test 1 completed at: $(date)"
echo ""

# Check results
if [ -f "results/test1_varying_initial.csv" ]; then
    lines=$(wc -l < "results/test1_varying_initial.csv")
    echo "Results saved to: results/test1_varying_initial.csv ($lines lines)"
else
    echo "Warning: No results file generated"
fi
