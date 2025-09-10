#!/bin/bash

#========================================
# SLURM Job Configuration
#========================================
#SBATCH --job-name=dmr-timing
#SBATCH --output=results/timing-%j.out
#SBATCH --ntasks=64
#SBATCH --overcommit
#SBATCH --time=10:00:00

#========================================
# DMR Timing Performance Benchmark Script
#========================================

#----------------------------------------
# Signal Handling and Cleanup Functions
#----------------------------------------

# Clean up on script exit
cleanup_on_exit() {
  echo "Script terminating, cleaning up processes..."
  cleanup_timing_processes
  # Clean up nodefile
  if [ -f "$NODEFILE" ]; then
    rm -f "$NODEFILE"
    echo "Cleaned up nodefile: $NODEFILE"
  fi
}

# Set trap to call cleanup function on script exit or signals
trap cleanup_on_exit EXIT TERM INT

#----------------------------------------
# Job Initialization
#----------------------------------------

echo "DMR Timing Performance Benchmark"
echo "================================="
echo "Job ID: $SLURM_JOB_ID"
echo "Nodes: $SLURM_NNODES"
echo "Total allocated tasks: $SLURM_NTASKS"
echo "Start time: $(date)"

# Set up project directory
PROJECT_ROOT="$(pwd)"
cd "$PROJECT_ROOT"
mkdir -p results

# Verify executable exists
if [ ! -x ./timing ]; then
  echo "Error: timing executable not found (./timing). Build it before submitting." >&2
  exit 1
fi

echo "Executable present. Starting test matrix..."

#----------------------------------------
# MPI Configuration
#----------------------------------------

# Create nodefile for MPI
NODEFILE="$PROJECT_ROOT/nodefile.${SLURM_JOB_ID}"
if [ -n "$SLURM_JOB_NODELIST" ]; then
  # Extract node names and create nodefile with slots
  scontrol show hostnames "$SLURM_JOB_NODELIST" > "$NODEFILE.tmp"
  
  # Calculate slots per node (total tasks / number of nodes)
  SLOTS_PER_NODE=$((SLURM_NTASKS / SLURM_NNODES))
  
  # Create nodefile with explicit slot counts
  > "$NODEFILE"
  while read -r node; do
    echo "${node} slots=${SLOTS_PER_NODE}" >> "$NODEFILE"
  done < "$NODEFILE.tmp"
  
  rm -f "$NODEFILE.tmp"
  
  echo "Created nodefile with $SLURM_NNODES nodes, $SLOTS_PER_NODE slots each:"
  cat "$NODEFILE"
  echo ""
else
  echo "Warning: SLURM_JOB_NODELIST not available, creating simple nodefile"
  echo "localhost slots=$SLURM_NTASKS" > "$NODEFILE"
fi

#----------------------------------------
# Test Configuration Parameters
#----------------------------------------

# Process counts to test (can include multiple values)
PROCESS_LIST="64 32 16 8"

# Test repetition configuration
NUM_REPETITIONS=5        # number of times to repeat each test case

# Iteration configuration (total iterations and trigger indices)
TOTAL_ITERS=10           # total compute iterations
EXPAND_ITER=2            # 0-based iteration index to trigger expansion
SHRINK_ITER=7            # 0-based iteration index to trigger shrink

#----------------------------------------
# Configuration Functions
#----------------------------------------

# Set expand and shrink amounts based on initial process count
set_expand_shrink_amounts() {
  local np=$1
  case $np in
    8)
      EXPAND_AMOUNTS="2 4 8"
      SHRINK_AMOUNTS="2 4 8"
      ;;
    16)
      EXPAND_AMOUNTS="4 8 16"
      SHRINK_AMOUNTS="4 8 16"
      ;;
    32)
      EXPAND_AMOUNTS="8 16 32"
      SHRINK_AMOUNTS="8 16 32"
      ;;
    64)
      EXPAND_AMOUNTS="16 32 64"
      SHRINK_AMOUNTS="16 32 64"
      ;;
    128)
      EXPAND_AMOUNTS="16 32 64"
      SHRINK_AMOUNTS="16 32 64"
      ;;
    *)
      # Default scaling for other process counts
      local quarter=$((np / 4))
      local half=$((np / 2))
      EXPAND_AMOUNTS="$quarter $half $np"
      SHRINK_AMOUNTS="$quarter $half $np"
      ;;
  esac
  echo "Set expand amounts: $EXPAND_AMOUNTS"
  echo "Set shrink amounts: $SHRINK_AMOUNTS"
}

# Set timeout based on process count
set_timeout_for_np() {
  local np=$1
  case $np in
    8)
      TIMEOUT_SECONDS=660
      ;;
    16)
      TIMEOUT_SECONDS=340
      ;;
    32)
      TIMEOUT_SECONDS=180
      ;;
    64)
      TIMEOUT_SECONDS=100
      ;;
    128)
      TIMEOUT_SECONDS=60
      ;;
    *)
      # Default scaling: base 300s + 50s per 8 processes
      TIMEOUT_SECONDS=$((300 + (np * 50 / 8)))
      ;;
  esac
  echo "Set timeout for np=$np: ${TIMEOUT_SECONDS}s ($(($TIMEOUT_SECONDS / 60)) minutes)"
}

#----------------------------------------
# Process Management Functions
#----------------------------------------

# Clean up any lingering timing processes
cleanup_timing_processes() {
  echo "Cleaning up any remaining timing processes..."
  
  # Get list of nodes from the nodefile
  if [ -f "$NODEFILE" ]; then
    # Extract node names from nodefile (remove slots= part)
    local nodes=$(awk '{print $1}' "$NODEFILE" | sort -u)
    
    for node in $nodes; do
      echo "Cleaning processes on node: $node"
      # Kill any timing processes on each node
      ssh "$node" "pkill -u $USER -f timing" 2>/dev/null || true
      # Also kill any mpirun processes that might be stuck
      ssh "$node" "pkill -u $USER -f mpirun" 2>/dev/null || true
      # Kill any dmr_wrapper processes
      ssh "$node" "pkill -u $USER -f dmr_wrapper" 2>/dev/null || true
    done
  else
    # Fallback to local cleanup
    echo "Cleaning processes locally"
    pkill -u $USER -f timing 2>/dev/null || true
    pkill -u $USER -f mpirun 2>/dev/null || true
    pkill -u $USER -f dmr_wrapper 2>/dev/null || true
  fi
  
  # Give processes a moment to terminate
  sleep 3
  echo "Process cleanup completed."
}

# Check for existing processes before starting a test
check_existing_processes() {
  local found_processes=0
  
  if [ -f "$NODEFILE" ]; then
    local nodes=$(awk '{print $1}' "$NODEFILE" | sort -u)
    
    for node in $nodes; do
      local timing_procs=$(ssh "$node" "pgrep -u $USER -f timing" 2>/dev/null | wc -l)
      local mpi_procs=$(ssh "$node" "pgrep -u $USER -f mpirun" 2>/dev/null | wc -l)
      
      if [ "$timing_procs" -gt 0 ] || [ "$mpi_procs" -gt 0 ]; then
        echo "WARNING: Found $timing_procs timing and $mpi_procs mpirun processes on $node"
        found_processes=1
      fi
    done
  else
    local timing_procs=$(pgrep -u $USER -f timing 2>/dev/null | wc -l)
    local mpi_procs=$(pgrep -u $USER -f mpirun 2>/dev/null | wc -l)
    
    if [ "$timing_procs" -gt 0 ] || [ "$mpi_procs" -gt 0 ]; then
      echo "WARNING: Found $timing_procs timing and $mpi_procs mpirun processes locally"
      found_processes=1
    fi
  fi
  
  if [ "$found_processes" -eq 1 ]; then
    echo "Cleaning up existing processes before starting new test..."
    cleanup_timing_processes
  fi
}

#----------------------------------------
# Test Execution Functions
#----------------------------------------

# Run a single test scenario
run_case() {
  local np=$1
  local exp=$2
  local shr=$3
  local label=$4
  local rep=$5  # repetition number
  
  # Define file paths for this test case
  local out_file="results/timing_run_np${np}_${label}_rep${rep}.out"
  local csv_file="results/timing_np${np}.csv"
  local ckpt_file="checkpoints/timing_np${np}_rep${rep}.chk"
  local plan_file="plans/plan_np${np}_${label}_rep${rep}.cfg"
  
  echo "--- np=${np} ${label} repetition ${rep} (expand=${exp} shrink=${shr}) ---"
  
  # Check for and cleanup any existing processes before starting
  check_existing_processes
  
  # Create plan file for this test case
  mkdir -p plans
  cat > "${plan_file}" <<EOF
initial_np=${np}
expand=${exp}
shrink=${shr}
expand_iter=${EXPAND_ITER}
shrink_iter=${SHRINK_ITER}
iters=${TOTAL_ITERS}
EOF

  # Execute the timing test with timeout
  local cmd=(timeout "$TIMEOUT_SECONDS" dmr_wrapper mpirun --with-ft ulfm --hostfile "$NODEFILE" -np "${np}" ./timing \
    --csv ${csv_file} --ckpt ${ckpt_file} --plan ${plan_file})
  
  echo "Running command: ${cmd[*]}"
  "${cmd[@]}" | tee "${out_file}"
  local exit_code=$?
  
  # Check execution status
  if [ $exit_code -eq 124 ]; then
    echo "WARNING: Test case timed out after $TIMEOUT_SECONDS seconds"
  elif [ $exit_code -ne 0 ]; then
    echo "WARNING: Test case exited with non-zero status: $exit_code"
  fi
  
  # Always cleanup processes after each run
  cleanup_timing_processes
}

#========================================
# Main Test Execution Loop
#========================================

for NP in $PROCESS_LIST; do
  # Skip if requested process count exceeds allocation
  if [ "$NP" -gt "$SLURM_NTASKS" ]; then
    echo "Skipping NP=$NP (exceeds allocated tasks $SLURM_NTASKS)"
    continue
  fi
  
  echo ""; echo "=== Initial processes: $NP ==="
  
  # Configure test parameters for this process count
  set_expand_shrink_amounts "$NP"
  set_timeout_for_np "$NP"
  
  # Initialize CSV file (create header once per NP)
  csv_file="results/timing_np${NP}.csv"
  if [ ! -f "$csv_file" ]; then 
    echo "initial_procs,expand_amount,shrink_amount,expand_time,shrink_time,total_time" > "$csv_file"
  fi
  
  #----------------------------------------
  # Run Baseline Test (No Expansion/Shrink)
  #----------------------------------------
  
  echo ""; echo "*** Running baseline for NP=$NP ***"
  
  # Clean up any existing checkpoint files
  ckpt_file="checkpoints/timing_np${NP}.chk"
  timers_ckpt="checkpoints/timing_checkpoint_timers.bin"
  if [ -f "$ckpt_file" ]; then rm -f "$ckpt_file"; fi
  if [ -f "$timers_ckpt" ]; then rm -f "$timers_ckpt"; fi
  
  # Set up baseline test files
  out_file="results/timing_run_np${NP}_baseline.out"
  csv_file="results/timing_np${NP}.csv"
  ckpt_file="checkpoints/timing_np${NP}.chk"
  plan_file="plans/plan_np${NP}_baseline.cfg"
  
  echo "--- np=${NP} baseline (expand=0 shrink=0) ---"
  
  # Ensure clean environment
  check_existing_processes
  
  # Create baseline plan file
  mkdir -p plans
  cat > "${plan_file}" <<EOF
initial_np=${NP}
expand=0
shrink=0
expand_iter=${EXPAND_ITER}
shrink_iter=${SHRINK_ITER}
iters=${TOTAL_ITERS}
EOF

  # Execute baseline test
  cmd=(timeout "$TIMEOUT_SECONDS" dmr_wrapper mpirun --with-ft ulfm --hostfile "$NODEFILE" -np "${NP}" ./timing \
    --csv ${csv_file} --ckpt ${ckpt_file} --plan ${plan_file})
  
  echo "Running baseline command: ${cmd[*]}"
  "${cmd[@]}" | tee "${out_file}"
  exit_code=$?
  
  # Check baseline execution status
  if [ $exit_code -eq 124 ]; then
    echo "WARNING: Baseline test timed out after $TIMEOUT_SECONDS seconds"
  elif [ $exit_code -ne 0 ]; then
    echo "WARNING: Baseline test exited with non-zero status: $exit_code"
  fi
  
  # Cleanup after baseline
  cleanup_timing_processes

  #----------------------------------------
  # Run Expand/Shrink Test Cases
  #----------------------------------------
  
  # Build paired arrays for expand/shrink combinations
  read -r -a EXP_LIST <<< "$EXPAND_AMOUNTS"
  read -r -a SHR_LIST <<< "$SHRINK_AMOUNTS"
  if [ ${#EXP_LIST[@]} -ne ${#SHR_LIST[@]} ]; then
    echo "ERROR: EXPAND_AMOUNTS and SHRINK_AMOUNTS must have same length for pairing" >&2
    exit 2
  fi
  
  # Execute each expand/shrink combination
  for idx in "${!EXP_LIST[@]}"; do
    EXP=${EXP_LIST[$idx]}
    SHR=${SHR_LIST[$idx]}
    
    # Skip baseline pair (0,0) as it was already executed above
    if [ "$EXP" -eq 0 ] && [ "$SHR" -eq 0 ]; then
      continue
    fi
    
    label="expand${EXP}_shrink${SHR}"
    
    # Run multiple repetitions for this test case
    for rep in $(seq 1 $NUM_REPETITIONS); do
      echo ""; echo "*** Running $label repetition $rep/$NUM_REPETITIONS for NP=$NP ***"
      
      # Clean up checkpoint files for this repetition
      ckpt_file="checkpoints/timing_np${NP}_rep${rep}.chk"
      timers_ckpt="checkpoints/timing_checkpoint_timers.bin"
      if [ -f "$ckpt_file" ]; then rm -f "$ckpt_file"; fi
      if [ -f "$timers_ckpt" ]; then rm -f "$timers_ckpt"; fi
      
      # Execute the test case
      run_case "$NP" "$EXP" "$SHR" "$label" "$rep"
    done
  done
  
  echo "Completed all scenarios and repetitions for NP=$NP"
done

#========================================
# Job Completion
#========================================

echo ""
echo "All timing runs completed at $(date)."
echo "Completed $NUM_REPETITIONS repetitions for each test case."
echo "Per-scenario stdout: results/timing_run_np*_*_rep*.out"

# Final cleanup (also handled by EXIT trap)
echo "Performing final cleanup..."
cleanup_timing_processes
