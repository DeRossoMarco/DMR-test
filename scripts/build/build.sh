#!/bin/bash
#
# DMR Build Script
# Builds all components of the DMR test suite
#

set -e  # Exit on any error

# Get the project root directory
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$PROJECT_ROOT"

echo "Building DMR Test Suite..."
echo "Project root: $PROJECT_ROOT"

# Clean previous builds
echo "Cleaning previous builds..."
make clean

# Build all targets
echo "Building all targets..."
make all

echo "Build completed successfully!"
echo ""
echo "Available executables:"
echo "  - src/examples/timing/comprehensive_timing"
echo "  - src/examples/counter/counter (if implemented)"
echo ""
echo "To run tests:"
echo "  - Use scripts in scripts/run/ for local execution"
echo "  - Use scripts in scripts/slurm/ for SLURM cluster execution"
