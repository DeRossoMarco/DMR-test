#!/bin/bash
#SBATCH --job-name=dmr-test3-shrink-only
#SBATCH --output=results/test3-shrink-only-%j.out
#SBATCH --ntasks=12
#SBATCH --cpus-per-task=1
#SBATCH --mem-per-cpu=2G
#SBATCH --oversubscribe
#SBATCH --time=01:00:00
#SBATCH --no-kill

# DMR Test 3: Multiple shrinking without expansion

echo "DMR Test 3: Multiple Shrinking Without Expansion"
echo "=============================================="
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
echo "- Progressive shrinking from initial size down to minimum (2 processes)"
echo "- No expansion operations"
echo "- Performance analysis of pure shrinking"
echo ""

# Run Test 3 with different process counts
# Start with higher process counts for this test to allow more shrinking
for procs in 6 8 10 12; do
    if [ $procs -le $SLURM_NTASKS ]; then
        echo "Running Test 3 with $procs initial processes..."
        echo "(Will progressively shrink down to 2 processes)"
        
        dmr_wrapper mpirun --with-ft ulfm -np $procs ./comprehensive_timing 3
        
        if [ $? -eq 0 ]; then
            echo "✓ Test 3 completed successfully with $procs initial processes"
        else
            echo "✗ Test 3 failed with $procs initial processes"
        fi
        
        sleep 3
        echo ""
    else
        echo "Skipping $procs processes (exceeds allocated: $SLURM_NTASKS)"
    fi
done

echo "Test 3 completed at: $(date)"
echo ""

# Check results
if [ -f "results/test3_shrink_only.csv" ]; then
    lines=$(wc -l < "results/test3_shrink_only.csv")
    echo "Results saved to: results/test3_shrink_only.csv ($lines lines)"
else
    echo "Warning: No results file generated"
fi
