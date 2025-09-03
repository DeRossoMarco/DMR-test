# Scripts Directory

This directory contains all scripts for building, running, and managing the DMR test suite.

## Directory Structure

```
scripts/
├── build/           # Build scripts
│   └── build.sh     # Main build script
├── run/             # Local execution scripts
│   ├── run_dmr_tests.sh
│   └── run_timing_tests.sh
├── slurm/           # SLURM batch job scripts
│   ├── examples/    # Example configurations
│   ├── run_comprehensive_timing.sbatch
│   ├── run_counter.sbatch
│   ├── run_test1.sbatch
│   ├── run_test2.sbatch
│   ├── run_test3.sbatch
│   └── run_timing.sbatch
├── testing/         # Testing utilities (if needed)
├── utils/           # Utility scripts
│   ├── cancel.sh    # Cancel running jobs
│   ├── monitor.sh   # Monitor job status
│   └── plot_results.py  # Plot and analyze results
├── run_tests.sh     # Master script for running tests
└── setup.sh         # Environment setup script
```

## Quick Start

1. **Setup the environment:**
   ```bash
   ./scripts/setup.sh
   ```

2. **Run tests locally:**
   ```bash
   ./scripts/run_tests.sh --local timing
   ```

3. **Submit to SLURM:**
   ```bash
   ./scripts/run_tests.sh --slurm comprehensive
   ```

## Script Descriptions

### Build Scripts

- **`build/build.sh`**: Main build script that compiles all DMR components

### Run Scripts

- **`run_tests.sh`**: Master script providing unified interface for running tests
- **`run/run_dmr_tests.sh`**: Local DMR test execution
- **`run/run_timing_tests.sh`**: Local timing test execution

### SLURM Scripts

- **`slurm/run_timing.sbatch`**: Basic timing tests on SLURM
- **`slurm/run_comprehensive_timing.sbatch`**: Extended timing analysis
- **`slurm/run_counter.sbatch`**: Counter example tests
- **`slurm/run_test[1-3].sbatch`**: Specific test configurations

### Utility Scripts

- **`utils/monitor.sh`**: Monitor SLURM job status and progress
- **`utils/cancel.sh`**: Cancel running SLURM jobs
- **`utils/plot_results.py`**: Generate plots and analyze results

## Usage Examples

### Local Development
```bash
# Build everything
./scripts/build/build.sh

# Run timing tests locally
./scripts/run_tests.sh --local timing

# Run DMR tests locally
./scripts/run_tests.sh --local dmr
```

### SLURM Cluster
```bash
# Submit timing tests
./scripts/run_tests.sh --slurm timing

# Submit comprehensive analysis
./scripts/run_tests.sh --slurm comprehensive

# Monitor jobs
./scripts/utils/monitor.sh

# Cancel all jobs
./scripts/utils/cancel.sh
```

### Results Analysis
```bash
# Plot timing results
./scripts/utils/plot_results.py results/timing-dmr.out

# Plot comprehensive results
./scripts/utils/plot_results.py results/timing-dmr-ulfm.out
```

## Configuration

- SLURM job parameters can be modified in the respective `.sbatch` files
- Build options can be adjusted in `build/build.sh` or the main `Makefile`
- Test parameters can be modified in the run scripts

## Adding New Scripts

1. **Build scripts**: Add to `build/` directory
2. **Local run scripts**: Add to `run/` directory
3. **SLURM scripts**: Add to `slurm/` directory
4. **Utilities**: Add to `utils/` directory
5. **Update `run_tests.sh`** if adding new test types

## Best Practices

- All scripts should be executable (`chmod +x`)
- Use absolute paths or proper relative path resolution
- Include proper error handling (`set -e`)
- Document script parameters and usage
- Follow the established naming conventions
