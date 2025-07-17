#!/bin/bash
#SBATCH --job-name=test
#SBATCH --time=00:15:00

# Number of nodes
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=10
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
cmd="dmr_wrapper mpirun --with-ft ulfm --mca mpi_ft_verbose 1 --host $NODELIST_WITH_COUNTS -np 10 ./test"

echo $cmd
$cmd