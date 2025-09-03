#!/bin/bash
#SBATCH --job-name=dmr-comprehensive-timing
#SBATCH --output=results/comprehensive-timing-%j.out
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=12
#SBATCH --time=02:00:00
#SBATCH --no-kill

# DMR Comprehensive Timing Tests SLURM Script
# This script runs all three comprehensive timing test scenarios

echo "DMR Comprehensive Timing Tests"
echo "=============================="
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

# Build the comprehensive timing test
echo "Building comprehensive timing test..."
make comprehensive_timing

if [ $? -ne 0 ]; then
    echo "Error: Failed to build comprehensive timing test"
    exit 1
fi

echo "Build successful."
echo ""

# Change to timing example directory
cd src/examples/timing

echo "Build successful."
echo ""

# Function to run a test with specific parameters
run_dmr_test() {
    local test_num=$1
    local test_name="$2"
    local proc_count=$3
    
    echo "========================================="
    echo "Running $test_name"
    echo "Test Number: $test_num"
    echo "Process Count: $proc_count"
    echo "========================================="
    
    # Use DMR wrapper with ULFM support
    cmd="dmr_wrapper mpirun --with-ft ulfm -np $proc_count ./comprehensive_timing $test_num"
    echo "Command: $cmd"
    echo ""
    
    $cmd
    
    if [ $? -eq 0 ]; then
        echo "✓ Test $test_num completed successfully with $proc_count processes"
    else
        echo "✗ Test $test_num failed with $proc_count processes"
    fi
    echo ""
}

# Test 1: Fixed expansion/shrinking with varying initial processes
echo "Starting Test 1: Fixed expansion/shrinking with varying initial processes"
echo "========================================================================"

for procs in 4 6 8 10 12; do
    if [ $procs -le $SLURM_NTASKS ]; then
        run_dmr_test 1 "Test 1: Varying Initial Processes" $procs
        sleep 5  # Small delay between runs
    else
        echo "Skipping $procs processes (exceeds allocated tasks: $SLURM_NTASKS)"
    fi
done

# Test 2: Fixed initial processes with varying expansion/shrinking
echo "Starting Test 2: Fixed initial processes with varying expansion/shrinking"
echo "========================================================================"

for procs in 4 6 8; do
    if [ $procs -le $SLURM_NTASKS ]; then
        run_dmr_test 2 "Test 2: Varying Expand/Shrink" $procs
        sleep 5  # Small delay between runs
    else
        echo "Skipping $procs processes (exceeds allocated tasks: $SLURM_NTASKS)"
    fi
done

# Test 3: Multiple shrinking without expansion
echo "Starting Test 3: Multiple shrinking without expansion"
echo "=================================================="

for procs in 6 8 10 12; do
    if [ $procs -le $SLURM_NTASKS ]; then
        run_dmr_test 3 "Test 3: Shrink Only" $procs
        sleep 5  # Small delay between runs
    else
        echo "Skipping $procs processes (exceeds allocated tasks: $SLURM_NTASKS)"
    fi
done

echo ""
echo "All comprehensive timing tests completed!"
echo "========================================"
echo "End time: $(date)"

# Display results summary
echo ""
echo "Generated CSV Files:"
echo "==================="
for file in results/test*.csv; do
    if [ -f "$file" ]; then
        lines=$(wc -l < "$file")
        size=$(du -h "$file" | cut -f1)
        echo "  - $file ($lines lines, $size)"
    fi
done

echo ""
echo "To generate plots, run on a machine with Python:"
echo "  python3 plot_results.py"
echo ""
echo "Required Python packages:"
echo "  pip3 install pandas matplotlib seaborn numpy"
