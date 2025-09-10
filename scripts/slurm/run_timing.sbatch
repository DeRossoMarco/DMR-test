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
#
# Features:
# - Automatic retry on SIGPIPE and communication errors
# - Configurable retry attempts (MAX_RETRY_ATTEMPTS)
# - Process cleanup between attempts
# - Detailed logging of retry attempts
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
MAX_RETRY_ATTEMPTS=3     # maximum retry attempts for failed runs (due to SIGPIPE, etc.)

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
      TIMEOUT_SECONDS=80
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

# Function to check if a test run failed due to SIGPIPE or other recoverable errors
check_run_failure() {
  local out_file=$1
  
  if [ ! -f "$out_file" ]; then
    echo "Output file not found, considering as failure"
    return 0  # Failure (file doesn't exist)
  fi
  
  # Check for SIGPIPE errors
  if grep -q "SIGPIPE" "$out_file"; then
    echo "SIGPIPE error detected in output"
    return 0  # Failure
  fi
  
  # Check for other MPI/communication errors
  if grep -q "Broken pipe" "$out_file"; then
    echo "Broken pipe error detected in output"
    return 0  # Failure
  fi
  
  if grep -q "Connection reset by peer" "$out_file"; then
    echo "Connection reset by peer error detected in output"
    return 0  # Failure
  fi
  
  if grep -q "prterun.*abort" "$out_file"; then
    echo "MPI abort detected in output"
    return 0  # Failure
  fi
  
  # Check if the timing program completed successfully
  # Look for expected completion indicators
  if ! grep -q "starting iteration" "$out_file"; then
    echo "No iteration progress found, considering as failure"
    return 0  # Failure
  fi
  
  # If none of the failure patterns found, consider success
  return 1  # Success
}

# Run a single test scenario with retry capability
run_case() {
  local np=$1
  local exp=$2
  local shr=$3
  local label=$4
  local rep=$5  # repetition number
  local attempt=$6  # attempt number (optional, defaults to 1)
  
  # Set default attempt number if not provided
  if [ -z "$attempt" ]; then
    attempt=1
  fi
  
  # Define file paths for this test case
  local base_out_file="results/timing_run_np${np}_${label}_rep${rep}"
  local out_file="${base_out_file}.out"
  if [ "$attempt" -gt 1 ]; then
    out_file="${base_out_file}_attempt${attempt}.out"
  fi
  
  local csv_file="results/timing_np${np}.csv"
  local ckpt_file="checkpoints/timing_np${np}_rep${rep}.chk"
  local plan_file="plans/plan_np${np}_${label}_rep${rep}.cfg"
  
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
  
  echo "Running command (attempt $attempt): ${cmd[*]}"
  "${cmd[@]}" | tee "${out_file}"
  local exit_code=$?
  
  # Check execution status
  local run_failed=0
  if [ $exit_code -eq 124 ]; then
    echo "WARNING: Test case timed out after $TIMEOUT_SECONDS seconds"
    run_failed=1
  elif [ $exit_code -ne 0 ]; then
    echo "WARNING: Test case exited with non-zero status: $exit_code"
    run_failed=1
  fi
  
  # Always cleanup processes after each run
  cleanup_timing_processes
  
  # Give tee time to flush output to file
  sleep 2
  
  # Additional check for specific error patterns in output
  if check_run_failure "$out_file"; then
    run_failed=1
  fi
  
  # Return status: 0 for success, 1 for failure
  return $run_failed
}

# Run a test case with automatic retry on failure
run_case_with_retry() {
  local np=$1
  local exp=$2
  local shr=$3
  local label=$4
  local rep=$5
  
  local attempt=1
  local max_attempts=$MAX_RETRY_ATTEMPTS
  local success=0
  
  echo "*** Running $label repetition $rep/$NUM_REPETITIONS for NP=$np ***"
  
  # Clean up checkpoint files for this repetition
  local ckpt_file="checkpoints/timing_np${np}_rep${rep}.chk"
  local timers_ckpt="checkpoints/timing_checkpoint_timers.bin"
  
  while [ $attempt -le $max_attempts ] && [ $success -eq 0 ]; do
    if [ $attempt -gt 1 ]; then
      echo "*** Retrying $label repetition $rep/$NUM_REPETITIONS for NP=$np (attempt $attempt/$max_attempts) ***"
    fi
    
    # Clean up checkpoint files before each attempt
    if [ -f "$ckpt_file" ]; then rm -f "$ckpt_file"; fi
    if [ -f "$timers_ckpt" ]; then rm -f "$timers_ckpt"; fi
    
    # Run the test case
    if run_case "$np" "$exp" "$shr" "$label" "$rep" "$attempt"; then
      echo "SUCCESS: $label repetition $rep completed successfully on attempt $attempt"
      success=1
    else
      echo "FAILURE: $label repetition $rep failed on attempt $attempt"
      if [ $attempt -lt $max_attempts ]; then
        echo "Will retry in 10 seconds..."
        sleep 10
      else
        echo "ERROR: $label repetition $rep failed after $max_attempts attempts"
      fi
      ((attempt++))
    fi
  done
  
  return $((1 - success))  # Return 0 for success, 1 for failure
}

# Run baseline test with retry logic
run_baseline_with_retry() {
  local np=$1
  local attempt=1
  local max_attempts=$MAX_RETRY_ATTEMPTS
  local success=0
  
  echo "*** Running baseline for NP=$np ***"
  
  while [ $attempt -le $max_attempts ] && [ $success -eq 0 ]; do
    if [ $attempt -gt 1 ]; then
      echo "*** Retrying baseline for NP=$np (attempt $attempt/$max_attempts) ***"
    fi
    
    # Clean up any existing checkpoint files
    local ckpt_file="checkpoints/timing_np${np}.chk"
    local timers_ckpt="checkpoints/timing_checkpoint_timers.bin"
    if [ -f "$ckpt_file" ]; then rm -f "$ckpt_file"; fi
    if [ -f "$timers_ckpt" ]; then rm -f "$timers_ckpt"; fi
    
    # Set up baseline test files
    local base_out_file="results/timing_run_np${np}_baseline"
    local out_file="${base_out_file}.out"
    if [ "$attempt" -gt 1 ]; then
      out_file="${base_out_file}_attempt${attempt}.out"
    fi
    
    local csv_file="results/timing_np${np}.csv"
    local plan_file="plans/plan_np${np}_baseline.cfg"
    
    # Ensure clean environment
    check_existing_processes
    
    # Create baseline plan file
    mkdir -p plans
    cat > "${plan_file}" <<EOF
initial_np=${np}
expand=0
shrink=0
expand_iter=${EXPAND_ITER}
shrink_iter=${SHRINK_ITER}
iters=${TOTAL_ITERS}
EOF

    # Execute baseline test
    local cmd=(timeout "$TIMEOUT_SECONDS" dmr_wrapper mpirun --with-ft ulfm --hostfile "$NODEFILE" -np "${np}" ./timing \
      --csv ${csv_file} --ckpt ${ckpt_file} --plan ${plan_file})
    
    echo "Running baseline command (attempt $attempt): ${cmd[*]}"
    "${cmd[@]}" | tee "${out_file}"
    local exit_code=$?
    
    # Check baseline execution status
    local run_failed=0
    if [ $exit_code -eq 124 ]; then
      echo "WARNING: Baseline test timed out after $TIMEOUT_SECONDS seconds"
      run_failed=1
    elif [ $exit_code -ne 0 ]; then
      echo "WARNING: Baseline test exited with non-zero status: $exit_code"
      run_failed=1
    fi
    
    # Cleanup after baseline
    cleanup_timing_processes
    
    # Give tee time to flush output to file
    sleep 2
    
    # Additional check for specific error patterns in output
    if check_run_failure "$out_file"; then
      run_failed=1
    fi
    
    if [ $run_failed -eq 0 ]; then
      echo "SUCCESS: Baseline test completed successfully on attempt $attempt"
      success=1
    else
      echo "FAILURE: Baseline test failed on attempt $attempt"
      if [ $attempt -lt $max_attempts ]; then
        echo "Will retry baseline in 10 seconds..."
        sleep 10
      else
        echo "ERROR: Baseline test failed after $max_attempts attempts"
      fi
      ((attempt++))
    fi
  done
  
  return $((1 - success))  # Return 0 for success, 1 for failure
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
  
  echo ""
  run_baseline_with_retry "$NP"
  if [ $? -ne 0 ]; then
    echo "ERROR: Baseline test failed after all retry attempts for NP=$NP"
    # Continue with other tests even if baseline fails
  fi

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
      echo ""
      
      # Execute the test case with retry capability
      run_case_with_retry "$NP" "$EXP" "$SHR" "$label" "$rep"
      if [ $? -ne 0 ]; then
        echo "WARNING: $label repetition $rep failed after all retry attempts for NP=$NP"
        # Continue with next repetition
      fi
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
echo "Maximum retry attempts per test: $MAX_RETRY_ATTEMPTS"
echo ""
echo "Output files:"
echo "  Per-scenario stdout: results/timing_run_np*_*_rep*.out"
echo "  Retry attempts: results/timing_run_np*_*_rep*_attempt*.out"
echo "  CSV results: results/timing_np*.csv"
echo ""

# Check for any retry attempts that occurred
echo "Checking for retry attempts..."
retry_files=$(find results/ -name "*_attempt*.out" 2>/dev/null | wc -l)
if [ "$retry_files" -gt 0 ]; then
  echo "Found $retry_files retry attempt output files:"
  find results/ -name "*_attempt*.out" 2>/dev/null | sort
else
  echo "No retry attempts were needed - all tests succeeded on first try."
fi

# Final cleanup (also handled by EXIT trap)
echo ""
echo "Performing final cleanup..."
cleanup_timing_processes
