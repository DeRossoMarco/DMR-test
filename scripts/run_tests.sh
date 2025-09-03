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
    echo "  -l, --local    Run tests locally"
    echo "  -s, --slurm    Submit tests to SLURM"
    echo ""
    echo "Test types:"
    echo "  timing         Run timing tests"
    echo "  comprehensive  Run comprehensive timing tests"
    echo "  dmr            Run DMR-specific tests"
    echo "  comparison     Run DMR version comparison tests"
    echo "  counter        Run counter tests"
    echo ""
    echo "Examples:"
    echo "  $0 --local timing"
    echo "  $0 --slurm comprehensive"
}

run_local() {
    case "$1" in
        timing)
            echo "Running local timing tests..."
            "$SCRIPTS_DIR/run/run_timing_tests.sh"
            ;;
        dmr)
            echo "Running local DMR tests..."
            "$SCRIPTS_DIR/run/run_dmr_tests.sh"
            ;;
        comparison)
            echo "Running DMR version comparison tests..."
            "$SCRIPTS_DIR/run_dmr_comparison.sh"
            ;;
        *)
            echo "Unknown test type: $1"
            usage
            exit 1
            ;;
    esac
}

run_slurm() {
    case "$1" in
        timing)
            echo "Submitting timing tests to SLURM..."
            sbatch "$SCRIPTS_DIR/slurm/run_timing.sbatch"
            ;;
        comprehensive)
            echo "Submitting comprehensive timing tests to SLURM..."
            sbatch "$SCRIPTS_DIR/slurm/run_comprehensive_timing.sbatch"
            ;;
        counter)
            echo "Submitting counter tests to SLURM..."
            sbatch "$SCRIPTS_DIR/slurm/run_counter.sbatch"
            ;;
        comparison)
            echo "Submitting DMR version comparison tests to SLURM..."
            sbatch "$SCRIPTS_DIR/slurm/run_dmr_comparison.sbatch"
            ;;
        *)
            echo "Unknown test type: $1"
            usage
            exit 1
            ;;
    esac
}

# Parse command line arguments
EXECUTION_MODE=""
TEST_TYPE=""

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            usage
            exit 0
            ;;
        -l|--local)
            EXECUTION_MODE="local"
            shift
            ;;
        -s|--slurm)
            EXECUTION_MODE="slurm"
            shift
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
if [[ -z "$EXECUTION_MODE" ]]; then
    echo "Error: Execution mode not specified"
    usage
    exit 1
fi

if [[ -z "$TEST_TYPE" ]]; then
    echo "Error: Test type not specified"
    usage
    exit 1
fi

# Execute the appropriate command
case "$EXECUTION_MODE" in
    local)
        run_local "$TEST_TYPE"
        ;;
    slurm)
        run_slurm "$TEST_TYPE"
        ;;
esac
