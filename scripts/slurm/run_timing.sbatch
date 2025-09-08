#!/bin/bash
#SBATCH --job-name=dmr-timing
#SBATCH --output=results/timing-%j.out
#SBATCH --ntasks=128
#SBATCH --nodes=2
#SBATCH --overcommit
#SBATCH --time=05:00:00
#SBATCH --no-kill

# DMR Timing Performance Benchmark SLURM Script

echo "DMR Timing Performance Benchmark"
echo "================================="
echo "Job ID: $SLURM_JOB_ID"
echo "Nodes: $SLURM_NNODES"
echo "Total allocated tasks: $SLURM_NTASKS"
echo "Start time: $(date)"

PROJECT_ROOT="$(pwd)"
cd "$PROJECT_ROOT"
mkdir -p results

if [ ! -x ./timing ]; then
  echo "Error: timing executable not found (./timing). Build it before submitting." >&2
  exit 1
fi

echo "Executable present. Starting test matrix..."

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
  
  # Verify total slots
  TOTAL_SLOTS=$((SLURM_NNODES * SLOTS_PER_NODE))
  echo "Total available slots: $TOTAL_SLOTS"
  if [ "$TOTAL_SLOTS" -lt 128 ]; then
    echo "Warning: Only $TOTAL_SLOTS slots available, but test requires up to 128"
  fi
  echo ""
else
  echo "Warning: SLURM_JOB_NODELIST not available, creating simple nodefile"
  echo "localhost slots=$SLURM_NTASKS" > "$NODEFILE"
fi

# Initial process counts to test 8
#PROCESS_LIST="8"
#EXPAND_AMOUNTS="2 4 8 4 8"
#SHRINK_AMOUNTS="2 4 8 6 12"

# Initial process counts to test 16
#PROCESS_LIST="16"
#EXPAND_AMOUNTS="4 8 16 8 16"
#SHRINK_AMOUNTS="4 8 16 12 24"

# Initial process counts to test 32
#PROCESS_LIST="32"
#EXPAND_AMOUNTS="8 16 32 16 32"
#SHRINK_AMOUNTS="8 16 32 24 48"

# Initial process counts to test 64
#PROCESS_LIST="64"
#EXPAND_AMOUNTS="16 32 64 32 64"
#SHRINK_AMOUNTS="16 32 64 48 96"

# Initial process counts to test 128
PROCESS_LIST="128"
EXPAND_AMOUNTS="16 32 64 32 64"
SHRINK_AMOUNTS="16 32 64 48 96"

# Iteration configuration (total iterations and trigger indices)
TOTAL_ITERS=10           # total compute iterations
EXPAND_ITER=2            # 0-based iteration index to trigger expansion
SHRINK_ITER=7            # 0-based iteration index to trigger shrink

# Function to run a single scenario
run_case() {
  local np=$1
  local exp=$2
  local shr=$3
  local label=$4
  local out_file="results/timing_run_np${np}_${label}.out"
  local csv_file="results/timing_np${np}.csv"
  local ckpt_file="checkpoints/timing_np${np}.chk"
  local plan_file="plans/plan_np${np}_${label}.cfg"
  echo "--- np=${np} ${label} (expand=${exp} shrink=${shr}) ---"
  mkdir -p plans
  cat > "${plan_file}" <<EOF
initial_np=${np}
expand=${exp}
shrink=${shr}
expand_iter=${EXPAND_ITER}
shrink_iter=${SHRINK_ITER}
iters=${TOTAL_ITERS}
EOF
  # Use nodefile-based MPI execution
  local cmd=(dmr_wrapper mpirun --with-ft ulfm --hostfile "$NODEFILE" -np "${np}" ./timing \
    --csv ${csv_file} --ckpt ${ckpt_file} --plan ${plan_file})
  "${cmd[@]}" | tee "${out_file}"
}

for NP in $PROCESS_LIST; do
  if [ "$NP" -gt "$SLURM_NTASKS" ]; then
    echo "Skipping NP=$NP (exceeds allocated tasks $SLURM_NTASKS)"
    continue
  fi
  echo ""; echo "=== Initial processes: $NP ==="
  # Remove any existing checkpoint for this NP before starting scenarios
  ckpt_file="checkpoints/timing_np${NP}.chk"
  if [ -f "$ckpt_file" ]; then rm -f "$ckpt_file"; fi
  # Remove any existing timers checkpoint file
  timers_ckpt="checkpoints/timing_checkpoint_timers.bin"
  if [ -f "$timers_ckpt" ]; then rm -f "$timers_ckpt"; fi
  # Prepare CSV (create header once per NP)
  csv_file="results/timing_np${NP}.csv"
  if [ ! -f "$csv_file" ]; then echo "initial_procs,expand_amount,shrink_amount,expand_time,shrink_time,total_time" > "$csv_file"; fi
  # Baseline (ensure exactly once) using plan file with expand=0 shrink=0
  run_case "$NP" 0 0 "baseline"

  # Build paired arrays
  read -r -a EXP_LIST <<< "$EXPAND_AMOUNTS"
  read -r -a SHR_LIST <<< "$SHRINK_AMOUNTS"
  if [ ${#EXP_LIST[@]} -ne ${#SHR_LIST[@]} ]; then
    echo "ERROR: EXPAND_AMOUNTS and SHRINK_AMOUNTS must have same length for pairing" >&2
    exit 2
  fi
  for idx in "${!EXP_LIST[@]}"; do
    EXP=${EXP_LIST[$idx]}
    SHR=${SHR_LIST[$idx]}
    # Skip baseline pair (0,0) already executed
    if [ "$EXP" -eq 0 ] && [ "$SHR" -eq 0 ]; then
      continue
    fi
    if [ -f "$ckpt_file" ]; then rm -f "$ckpt_file"; fi
    # Also remove timers checkpoint file between test cases
    if [ -f "$timers_ckpt" ]; then rm -f "$timers_ckpt"; fi
    label="expand${EXP}_shrink${SHR}"
    run_case "$NP" "$EXP" "$SHR" "$label"
  done
  echo "Completed all scenarios for NP=$NP"
done

echo "\nAll timing runs completed at $(date)."
echo "Per-scenario stdout: results/timing_run_np*_*.out"

# Clean up nodefile
if [ -f "$NODEFILE" ]; then
  rm -f "$NODEFILE"
  echo "Cleaned up nodefile: $NODEFILE"
fi
