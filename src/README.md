# DMR Examples - Source Code Organization

This directory contains the reorganized source code for DMR (Dynamic Memory Recovery) examples with MPI support.

## Directory Structure

```
src/
├── common/                    # Shared utilities and libraries
│   ├── dmr_utils.h           # Common DMR utility functions
│   ├── dmr_utils.c
│   ├── mpi_helpers.h         # Common MPI helper functions
│   └── mpi_helpers.c
├── examples/                  # Individual DMR examples
│   ├── counter/              # Distributed counter simulation
│   │   ├── counter.h
│   │   ├── counter.c
│   │   └── counter_functions.c
│   └── matrix_multiply/      # Matrix multiplication example (template)
│       └── matrix.h
└── include/                  # Global headers (if needed)
```

## Available Examples

### 1. Counter Example (`src/examples/counter/`)
A distributed counter simulation where each MPI rank manages a subset of counters. Features:
- Load balancing across MPI ranks
- Checkpoint/restart capabilities
- Dynamic process reconfiguration
- Example of basic DMR patterns

### 2. Matrix Multiplication Example (`src/examples/matrix_multiply/`)
*[Template created - implementation pending]*
A distributed matrix multiplication example that would demonstrate:
- Block-wise matrix distribution
- Parallel computation patterns
- Large data checkpoint/restart

## Building Examples

### Build All Examples
```bash
make -f Makefile.new all
```

### Build Specific Example
```bash
make -f Makefile.new counter
```

### List Available Examples
```bash
make -f Makefile.new list
```

### Create New Example Template
```bash
make -f Makefile.new new-example NAME=my_example
```

## Common Utilities

### DMR Utils (`src/common/dmr_utils.*`)
- `LoadDistribution`: Structure for work distribution across ranks
- `calculate_load_distribution()`: Balanced load distribution
- `format_checkpoint_path()`: Checkpoint file path management
- `safe_fopen()`: Safe file operations with error handling

### MPI Helpers (`src/common/mpi_helpers.*`)
- `mpi_init_and_get_info()`: MPI initialization wrapper
- `print_rank_message()`: Rank-aware debugging output
- `synchronized_barrier()`: Barrier with optional debug messages
- `is_root_rank()`: Root rank checking

## Adding New Examples

1. Create a new directory under `src/examples/`
2. Create the necessary `.h`, `.c`, and `*_functions.c` files
3. Add the example name to the `EXAMPLES` list in `Makefile.new`
4. Include common utilities in your headers:
   ```c
   #include "dmr_utils.h"
   #include "mpi_helpers.h"
   ```

## Migration Notes

The original files have been reorganized as follows:
- `src/test.h` → `src/examples/counter/counter.h`
- `src/test.c` → `src/examples/counter/counter.c`
- `src/test_functions.c` → `src/examples/counter/counter_functions.c`

The old `Makefile` is preserved, and the new one is in `Makefile.new`.
