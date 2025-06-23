# Distributed Counter Simulation with MPI and DMR

This project implements a distributed counter simulation using MPI (Message Passing Interface) with DMR (Dynamic Management of Resources) capabilities. The program demonstrates checkpointing, restarting, and dynamic process reconfiguration in a parallel computing environment.

## Features
- Distributed counters across MPI ranks
- Checkpoint/restart support for fault tolerance
- Dynamic process reconfiguration using DMR (Dynamic Management of Resources)
- Example scripts for monitoring and job management

## Project Structure
- `src/` — Source code for the main program and supporting functions
- `checkpoints/` — Directory for storing checkpoint files
- `scripts/` — Utility scripts (e.g., monitor, cancel)
- `Makefile` — Build instructions
- `run.sbatch` — Example SLURM batch script

## Building
To build the project, run:

```
make
```

Requirements:
- MPI implementation (e.g., OpenMPI, MPICH)
- DMR library (`-ldmr`)

## Running
This program is intended to be run using SLURM. Submit the job with:

```
sbatch run.sbatch
```

If you want to run manually (for testing), you can use:

```
mpirun -np 4 ./test
```

## Cleaning Up
To remove build artifacts and checkpoints:

```
make clean
```

## Author
Marco De Rosso

## License
This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
