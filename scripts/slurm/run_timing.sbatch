#!/bin/bash
#SBATCH --job-name=dmr-timing
#SBATCH --output=results/timing-%j.out
#SBATCH --ntasks=16
#SBATCH --cpus-per-task=1
#SBATCH --mem-per-cpu=2G
#SBATCH --oversubscribe
#SBATCH --time=00:30:00
#SBATCH --no-kill

# DMR Timing Performance Benchmark SLURM Script
# Runs a matrix of tests for each initial process count:
#   1. Baseline (no expand/shrink)
#   2. Expand only
#   3. Shrink only
#   4. Expand then Shrink
# Results appended to per-initial-size CSV in results/.

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

# Initial process counts to test
PROCESS_LIST="4 6 8 10 12 14 16"

# Expansion / shrink parameter lists (space-separated). Adjust as needed.
# (Baseline 0/0 handled separately.)
EXPAND_AMOUNTS="2 4 8 4 6"
SHRINK_AMOUNTS="2 4 8 6 8"

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
  local cmd=(dmr_wrapper mpirun --with-ft ulfm -np "${np}" ./timing \
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
    label="expand${EXP}_shrink${SHR}"
    run_case "$NP" "$EXP" "$SHR" "$label"
  done
  echo "Completed all scenarios for NP=$NP"
done

echo "\nAll timing runs completed at $(date)."
echo "Per-scenario stdout: results/timing_run_np*_*.out"
