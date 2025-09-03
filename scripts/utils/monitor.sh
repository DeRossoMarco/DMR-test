#!/bin/bash

# Enhanced SLURM job monitoring script for user mderosso
# Usage: ./monitor.sh [refresh_interval]

# Color definitions
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
BOLD='\033[1m'
DIM='\033[2m'
NC='\033[0m' # No Color

# Configuration
USER="mderosso"
REFRESH_INTERVAL=${1:-5}  # Default 5 seconds, can be overridden by command line argument

# Function to display header
show_header() {
    local current_time=$(date '+%Y-%m-%d %H:%M:%S')
    echo -e "${BOLD}${BLUE}╔═════════════════════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BOLD}${BLUE}║${NC}                          ${BOLD}${CYAN}SLURM Job Monitor${NC}.                                 ${BOLD}${BLUE}║${NC}"
    echo -e "${BOLD}${BLUE}║${NC} User: ${CYAN}${USER}${NC}                                    Time: ${CYAN}${current_time}${NC} ${BOLD}${BLUE}║${NC}"
    echo -e "${BOLD}${BLUE}╚═════════════════════════════════════════════════════════════════════════════╝${NC}"
    echo ""
}

# Function to get job statistics
get_job_stats() {
    local running=$(squeue -u $USER -h -t RUNNING | wc -l)
    local pending=$(squeue -u $USER -h -t PENDING | wc -l)
    local total=$(squeue -u $USER -h | wc -l)
    local completing=$(squeue -u $USER -h -t COMPLETING | wc -l)
    
    echo -e "${BOLD}${GREEN}Job Statistics:${NC}"
    echo -e "  ${CYAN}Running:${NC} ${GREEN}$running${NC}"
    echo -e "  ${CYAN}Pending:${NC} ${YELLOW}$pending${NC}"
    echo -e "  ${CYAN}Completing:${NC} ${BLUE}$completing${NC}"
    echo -e "  ${CYAN}Total:${NC} ${BOLD}$total${NC}"
    echo ""
}

# Function to show detailed job information
show_detailed_jobs() {
    echo -e "${BOLD}${MAGENTA}Detailed Job Information:${NC}"
    echo -e "${DIM}────────────────────────────────────────────────────────────────────────────────${NC}"
    
    # Custom squeue format for better readability
    squeue -u $USER --format="%.8i %.12P %.20j %.8u %.2t %.10M %.6D %.20R %.8Q" --sort=-t,i 2>/dev/null
    
    if [ $? -ne 0 ] || [ $(squeue -u $USER -h | wc -l) -eq 0 ]; then
        echo -e "${YELLOW}No jobs found for user ${CYAN}$USER${NC}"
    fi
    echo ""
}

# Function to show resource usage summary
show_resource_summary() {
    echo -e "${BOLD}${CYAN}Resource Usage Summary:${NC}"
    echo -e "${DIM}────────────────────────────────────────────────────────────────────────────────${NC}"
    
    # Get CPU and memory usage for running jobs
    local cpu_total=0
    local mem_total=0
    local node_count=0
    
    while IFS= read -r line; do
        if [ ! -z "$line" ]; then
            local cpus=$(echo "$line" | awk '{print $7}')
            local nodes=$(echo "$line" | awk '{print $8}')
            # Validate that cpus and nodes are numeric before adding
            if [[ "$cpus" =~ ^[0-9]+$ ]]; then
                cpu_total=$((cpu_total + cpus))
            fi
            if [[ "$nodes" =~ ^[0-9]+$ ]]; then
                node_count=$((node_count + nodes))
            fi
        fi
    done < <(squeue -u $USER -h -t RUNNING --format="%.8i %.12P %.20j %.8u %.2t %.10M %C %D" 2>/dev/null)
    
    echo -e "  ${CYAN}Total CPUs in use:${NC} ${GREEN}$cpu_total${NC}"
    echo -e "  ${CYAN}Total Nodes in use:${NC} ${GREEN}$node_count${NC}"
    echo ""
}

# Function to show recent job history
show_recent_history() {
    echo -e "${BOLD}${YELLOW}Recent Job History (last 10 completed jobs):${NC}"
    echo -e "${DIM}────────────────────────────────────────────────────────────────────────────────${NC}"
    
    sacct -u $USER --starttime=$(date -d '24 hours ago' '+%Y-%m-%d') --format="JobID%10,JobName%20,State%12,ExitCode%10,Elapsed%12,End%19" --parsable2 | tail -n 11 | head -n 10 2>/dev/null
    
    if [ $? -ne 0 ]; then
        echo -e "${DIM}Job history not available${NC}"
    fi
    echo ""
}

# Function to show partition information
show_partition_info() {
    echo -e "${BOLD}${BLUE}Available Partitions:${NC}"
    echo -e "${DIM}────────────────────────────────────────────────────────────────────────────────${NC}"
    
    sinfo --format="%.12P %.5a %.10l %.6D %.6t %.14C %.8G %.10m %.20f" 2>/dev/null | head -n 6
    echo ""
}

# Function to handle script termination
cleanup() {
    echo -e "\n${YELLOW}Monitoring stopped.${NC}"
    exit 0
}

# Trap Ctrl+C
trap cleanup SIGINT

# Main monitoring loop
echo -e "${BOLD}${GREEN}Starting SLURM job monitoring for user ${CYAN}$USER${NC}"
echo -e "${DIM}Press Ctrl+C to stop monitoring${NC}"
echo -e "${DIM}Refresh interval: ${REFRESH_INTERVAL} seconds${NC}"
echo ""

while true; do
    clear
    
    show_header
    get_job_stats
    show_detailed_jobs
    show_resource_summary
    show_recent_history
    show_partition_info
    
    echo -e "${DIM}Last updated: $(date '+%H:%M:%S') | Next refresh in ${REFRESH_INTERVAL}s | Press Ctrl+C to exit${NC}"
    
    sleep $REFRESH_INTERVAL
done