#!/bin/bash
#SBATCH --job-name=dmr-test
#SBATCH --time=01:00:00

# Number of nodes
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=50
#SBATCH --no-kill

NODELIST="$(scontrol show hostname $SLURM_JOB_NODELIST)"

NODELIST_WITH_COUNTS=""
for node in $NODELIST; do
    NTASKS=${SLURM_NTASKS_PER_NODE:-$DEFAULT_NTASKS_PER_NODE}
    NODELIST_WITH_COUNTS+="${node}:${NTASKS},"
done

# Just trim the trailing comma
NODELIST_WITH_COUNTS="${NODELIST_WITH_COUNTS%,}"

# Remember to add DMR_PATH/bin to PATH
cmd="dmr_wrapper mpirun --with-ft ulfm --host $NODELIST_WITH_COUNTS -np 50 ./counter"

echo $cmd
$cmd