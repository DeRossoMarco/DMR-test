#!/bin/bash

# Script to cancel all SLURM jobs for user mderosso
# Usage: ./cancel_jobs.sh

# Color definitions
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

USER="mderosso"

echo -e "${BOLD}${BLUE}Looking for jobs belonging to user: ${CYAN}$USER${NC}"

# Get all job IDs for the specified user
job_ids=$(squeue -u $USER -h -o "%i")

if [ -z "$job_ids" ]; then
    echo -e "${YELLOW}No jobs found for user ${CYAN}$USER${NC}"
    exit 0
fi

echo -e "${BOLD}${GREEN}Found the following job IDs:${NC}"
echo -e "${CYAN}$job_ids${NC}"

# Ask for confirmation
echo ""
echo -e -n "${BOLD}${YELLOW}Are you sure you want to cancel all these jobs? (y/N): ${NC}"
read confirm

if [[ $confirm =~ ^[Yy]$ ]]; then
    echo -e "${BOLD}${BLUE}Cancelling jobs...${NC}"
    
    # Cancel each job
    for job_id in $job_ids; do
        echo -e "${BLUE}Cancelling job ${CYAN}$job_id${BLUE}...${NC}"
        scancel $job_id
        if [ $? -eq 0 ]; then
            echo -e "  ${GREEN}✓ Job $job_id cancelled successfully${NC}"
        else
            echo -e "  ${RED}✗ Failed to cancel job $job_id${NC}"
        fi
    done
    
    echo ""
    echo -e "${BOLD}${GREEN}Job cancellation process completed.${NC}"
else
    echo -e "${YELLOW}Job cancellation aborted.${NC}"
fi
