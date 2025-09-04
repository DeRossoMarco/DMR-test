#!/bin/bash
#SBATCH --job-name=dmr-test2-expand-shrink
#SBATCH --output=results/test2-expand-shrink-%j.out
#SBATCH --ntasks=8
#SBATCH --cpus-per-task=1
#SBATCH --mem-per-cpu=2G
#SBATCH --oversubscribe
#SBATCH --time=01:30:00
#SBATCH --no-kill

# DMR Test 2: Fixed initial processes with varying expansion/shrinking

echo "DMR Test 2: Fixed Initial Processes with Varying Expansion/Shrinking"
echo "=================================================================="
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
echo "- Fixed initial process count"
echo "- Testing expansion amounts: +1, +2, +3, +4 processes"
echo "- Testing shrinking amounts: -1, -2, -3 processes"
echo "- Multiple cycles for each configuration"
echo ""

# Run Test 2 with different process counts
# Using smaller process counts for this test since it tests many configurations
for procs in 4 6 8; do
    if [ $procs -le $SLURM_NTASKS ]; then
        echo "Running Test 2 with $procs initial processes..."
        dmr_wrapper mpirun --with-ft ulfm -np $procs ./comprehensive_timing 2
        
        if [ $? -eq 0 ]; then
            echo "✓ Test 2 completed successfully with $procs processes"
        else
            echo "✗ Test 2 failed with $procs processes"
        fi
        
        sleep 5  # Longer delay for this intensive test
        echo ""
    else
        echo "Skipping $procs processes (exceeds allocated: $SLURM_NTASKS)"
    fi
done

echo "Test 2 completed at: $(date)"
echo ""

# Check results
if [ -f "results/test2_varying_expand_shrink.csv" ]; then
    lines=$(wc -l < "results/test2_varying_expand_shrink.csv")
    echo "Results saved to: results/test2_varying_expand_shrink.csv ($lines lines)"
else
    echo "Warning: No results file generated"
fi
