#!/bin/bash
#
# Quick setup script for DMR development environment
#

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "Setting up DMR development environment..."
echo "Project root: $PROJECT_ROOT"

# Create necessary directories
echo "Creating directory structure..."
mkdir -p "$PROJECT_ROOT/results"
mkdir -p "$PROJECT_ROOT/checkpoints"
mkdir -p "$PROJECT_ROOT/logs"

# Build the project
echo "Building project..."
"$PROJECT_ROOT/scripts/build/build.sh"

echo ""
echo "Setup complete! You can now:"
echo "  - Run local tests: ./scripts/run_tests.sh --local timing"
echo "  - Submit SLURM jobs: ./scripts/run_tests.sh --slurm comprehensive"
echo "  - Monitor jobs: ./scripts/utils/monitor.sh"
echo "  - Plot results: ./scripts/utils/plot_results.py"
