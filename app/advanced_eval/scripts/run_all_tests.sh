#!/bin/bash
#
# Automated Advanced RT Scheduler Evaluation Script
# Runs all combinations of schedulers and workloads, collecting CSV data
#

set -e  # Exit on error

# Configuration
PROJECT_ROOT="/home/jack/cs736-project/zephyr"
APP_DIR="${PROJECT_ROOT}/app/advanced_eval"
OUTPUT_DIR="${APP_DIR}/results"
TIMEOUT=40  # seconds per test (longer than simple_eval due to 100 activations)

# Schedulers to test
SCHEDULERS=(
    "EDF:CONFIG_SCHED_DEADLINE"
    "WEIGHTED_EDF:CONFIG_736_MOD_EDF"
    "WSRT:CONFIG_736_WSRT"
    "RMS:CONFIG_736_RMS"
)

# Workloads to test
WORKLOADS=(
    "LIGHT:WORKLOAD_LIGHT"
    "MEDIUM:WORKLOAD_MEDIUM"
    "HEAVY:WORKLOAD_HEAVY"
    "OVERLOAD:WORKLOAD_OVERLOAD"
)

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Advanced RT Scheduler Evaluation${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Create output directory
mkdir -p "${OUTPUT_DIR}"
echo -e "${GREEN}Output directory: ${OUTPUT_DIR}${NC}"
echo ""

# Backup original files
cp "${APP_DIR}/prj.conf" "${OUTPUT_DIR}/prj.conf.backup"
cp "${APP_DIR}/include/workloads.h" "${OUTPUT_DIR}/workloads.h.backup"
echo -e "${GREEN}✓ Backed up configuration files${NC}"
echo ""

# Counter for tests
total_tests=$((${#SCHEDULERS[@]} * ${#WORKLOADS[@]}))
current_test=0

# Iterate through all combinations
for scheduler in "${SCHEDULERS[@]}"; do
    IFS=':' read -r sched_name sched_config <<< "$scheduler"
    
    for workload in "${WORKLOADS[@]}"; do
        IFS=':' read -r wl_name wl_config <<< "$workload"
        
        current_test=$((current_test + 1))
        
        echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
        echo -e "${YELLOW}Test ${current_test}/${total_tests}: ${sched_name} + ${wl_name}${NC}"
        echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
        
        # Update workload configuration
        echo -e "Configuring workload: ${wl_name}..."
        sed -i "s/#define CURRENT_WORKLOAD WORKLOAD_.*/#define CURRENT_WORKLOAD ${wl_config}/" \
            "${APP_DIR}/include/workloads.h"
        
        # Update scheduler configuration
        echo -e "Configuring scheduler: ${sched_name}..."
        
        # Comment out all schedulers first
        sed -i 's/^CONFIG_736_MOD_EDF=y/# CONFIG_736_MOD_EDF=y/' "${APP_DIR}/prj.conf"
        sed -i 's/^CONFIG_736_WSRT=y/# CONFIG_736_WSRT=y/' "${APP_DIR}/prj.conf"
        sed -i 's/^CONFIG_736_RMS=y/# CONFIG_736_RMS=y/' "${APP_DIR}/prj.conf"
        
        # Enable selected scheduler (EDF is just CONFIG_SCHED_DEADLINE, no extra config)
        if [ "$sched_config" != "CONFIG_SCHED_DEADLINE" ]; then
            sed -i "s/# ${sched_config}=y/${sched_config}=y/" "${APP_DIR}/prj.conf"
        fi
        
        # Build
        echo -e "Building..."
        cd "${PROJECT_ROOT}"
        if ! west build -b native_sim app/advanced_eval > /dev/null 2>&1; then
            echo -e "${RED}✗ Build failed${NC}"
            continue
        fi
        echo -e "${GREEN}✓ Build successful${NC}"
        
        # Run and capture output
        output_file="${OUTPUT_DIR}/${sched_name}_${wl_name}.log"
        csv_file="${OUTPUT_DIR}/${sched_name}_${wl_name}.csv"
        
        echo -e "Running simulation (timeout: ${TIMEOUT}s)..."
        if timeout ${TIMEOUT}s west build -t run > "${output_file}" 2>&1; then
            echo -e "${GREEN}✓ Simulation completed${NC}"
        else
            echo -e "${YELLOW}⚠ Simulation timed out (expected)${NC}"
        fi
        
        # Extract CSV data
        grep "^CSV," "${output_file}" > "${csv_file}" 2>/dev/null || true
        csv_lines=$(wc -l < "${csv_file}")
        echo -e "${GREEN}✓ Extracted ${csv_lines} CSV records${NC}"
        
        # Extract summary statistics
        summary_file="${OUTPUT_DIR}/${sched_name}_${wl_name}_summary.txt"
        grep -A 25 "╔═" "${output_file}" > "${summary_file}" 2>/dev/null || true
        
        echo ""
    done
done

# Restore original files
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${GREEN}Restoring original configuration files...${NC}"
cp "${OUTPUT_DIR}/prj.conf.backup" "${APP_DIR}/prj.conf"
cp "${OUTPUT_DIR}/workloads.h.backup" "${APP_DIR}/include/workloads.h"
rm "${OUTPUT_DIR}/prj.conf.backup" "${OUTPUT_DIR}/workloads.h.backup"

echo ""
echo -e "${BLUE}========================================${NC}"
echo -e "${GREEN}✓ All tests completed!${NC}"
echo -e "${BLUE}========================================${NC}"
echo -e "Results saved to: ${OUTPUT_DIR}"
echo -e "CSV files: ${OUTPUT_DIR}/*.csv"
echo -e "Log files: ${OUTPUT_DIR}/*.log"
echo -e "Summary files: ${OUTPUT_DIR}/*_summary.txt"
echo ""
echo -e "${YELLOW}Next step: Run the graphing script${NC}"
echo -e "  python3 scripts/generate_graphs.py"
echo ""
