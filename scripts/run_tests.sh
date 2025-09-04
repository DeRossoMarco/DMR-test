#!/bin/bash
#
# Master run script for DMR tests
# Provides a unified interface to run different test types
#

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SCRIPTS_DIR="$PROJECT_ROOT/scripts"

usage() {
    echo "Usage: $0 [OPTION] [TEST_TYPE]"
    echo ""
    echo "Options:"
    echo "  -h, --help     Show this help message"
    echo ""
    echo "Test types:"
    echo "  timing         Run timing tests"
    echo "  counter        Run counter tests"
    echo ""
    echo "Examples:"
    echo "  $0 comprehensive"
    echo "  $0 timing"
}

run_slurm() {
    case "$1" in
        timing)
            echo "Submitting timing tests to SLURM..."
            sbatch "$SCRIPTS_DIR/slurm/run_timing.sbatch"
            ;;
        counter)
            echo "Submitting counter tests to SLURM..."
            sbatch "$SCRIPTS_DIR/slurm/run_counter.sbatch"
            ;;
        *)
            echo "Unknown test type: $1"
            usage
            exit 1
            ;;
    esac
}

# Parse command line arguments
TEST_TYPE=""

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            usage
            exit 0
            ;;
        *)
            if [[ -z "$TEST_TYPE" ]]; then
                TEST_TYPE="$1"
            else
                echo "Unknown option: $1"
                usage
                exit 1
            fi
            shift
            ;;
    esac
done

# Validate arguments
if [[ -z "$TEST_TYPE" ]]; then
    echo "Error: Test type not specified"
    usage
    exit 1
fi

# Execute the test via SLURM
run_slurm "$TEST_TYPE"
