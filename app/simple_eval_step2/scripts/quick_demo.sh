#!/bin/bash
#
# Quick Demo - Test a few representative scheduler/workload/DW combinations
#

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$(dirname "$SCRIPT_DIR")")")"
APP_DIR="${PROJECT_ROOT}/app/simple_eval_step2"
OUTPUT_DIR="${APP_DIR}/results/quick_demo"
TIMEOUT=70

# Test combinations (scheduler:config, workload:config, dw:value)
TESTS=(
    "EDF:CONFIG_SCHED_DEADLINE:LIGHT:WORKLOAD_LIGHT:OFF:DYNAMIC_WEIGHTING_OFF"
    "WEIGHTED_EDF:CONFIG_736_MOD_EDF:MEDIUM:WORKLOAD_MEDIUM:ON:DYNAMIC_WEIGHTING_ON"
    "WSRT:CONFIG_736_WSRT:HEAVY:WORKLOAD_HEAVY:OFF:DYNAMIC_WEIGHTING_OFF"
    "RMS:CONFIG_736_RMS:OVERLOAD:WORKLOAD_OVERLOAD:ON:DYNAMIC_WEIGHTING_ON"
)

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Quick Demo - Testing 4 Combinations${NC}"
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

# Run tests
test_num=0
total_tests=${#TESTS[@]}

for test in "${TESTS[@]}"; do
    IFS=':' read -r sched_name sched_config wl_name wl_config dw_name dw_value <<< "$test"
    test_num=$((test_num + 1))
    
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${YELLOW}Test ${test_num}/${total_tests}: ${sched_name} + ${wl_name} + DW ${dw_name}${NC}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    
    # Update configurations
    echo -e "Configuring dynamic weighting: ${dw_name}..."
    sed -i "s/#define DYNAMIC_WEIGHTING DYNAMIC_WEIGHTING_.*/#define DYNAMIC_WEIGHTING ${dw_value}/" \
        "${APP_DIR}/include/workloads.h"
    
    echo -e "Configuring workload: ${wl_name}..."
    sed -i "s/#define CURRENT_WORKLOAD WORKLOAD_.*/#define CURRENT_WORKLOAD ${wl_config}/" \
        "${APP_DIR}/include/workloads.h"
    
    echo -e "Configuring scheduler: ${sched_name}..."
    sed -i 's/^CONFIG_736_MOD_EDF=y/# CONFIG_736_MOD_EDF=y/' "${APP_DIR}/prj.conf"
    sed -i 's/^CONFIG_736_WSRT=y/# CONFIG_736_WSRT=y/' "${APP_DIR}/prj.conf"
    sed -i 's/^CONFIG_736_RMS=y/# CONFIG_736_RMS=y/' "${APP_DIR}/prj.conf"
    
    if [ "$sched_config" != "CONFIG_SCHED_DEADLINE" ]; then
        sed -i "s/# ${sched_config}=y/${sched_config}=y/" "${APP_DIR}/prj.conf"
    fi
    
    # Build
    echo -e "Building..."
    cd "${PROJECT_ROOT}"
    if ! west build -b native_sim -p always app/simple_eval_step2 > /dev/null 2>&1; then
        echo -e "${RED}✗ Build failed${NC}"
        continue
    fi
    echo -e "${GREEN}✓ Build successful${NC}"
    
    # Run
    output_file="${OUTPUT_DIR}/${sched_name}_${wl_name}_DW${dw_name}.log"
    csv_file="${OUTPUT_DIR}/${sched_name}_${wl_name}_DW${dw_name}.csv"
    
    echo -e "Running simulation (timeout: ${TIMEOUT}s)..."
    if timeout ${TIMEOUT}s west build -t run > "${output_file}" 2>&1; then
        echo -e "${GREEN}✓ Simulation completed${NC}"
    else
        echo -e "${YELLOW}⚠ Simulation timed out (expected)${NC}"
    fi
    
    # Extract data
    grep "^CSV," "${output_file}" > "${csv_file}" 2>/dev/null || true
    csv_lines=$(wc -l < "${csv_file}")
    echo -e "${GREEN}✓ Extracted ${csv_lines} CSV records${NC}"
    
    summary_file="${OUTPUT_DIR}/${sched_name}_${wl_name}_DW${dw_name}_summary.txt"
    grep -A 5 "=== Task" "${output_file}" > "${summary_file}" 2>/dev/null || true
    
    echo ""
done

# Restore original files
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${GREEN}Restoring original configuration files...${NC}"
cp "${OUTPUT_DIR}/prj.conf.backup" "${APP_DIR}/prj.conf"
cp "${OUTPUT_DIR}/workloads.h.backup" "${APP_DIR}/include/workloads.h"
rm "${OUTPUT_DIR}/prj.conf.backup" "${OUTPUT_DIR}/workloads.h.backup"

echo ""
echo -e "${BLUE}========================================${NC}"
echo -e "${GREEN}✓ Quick demo completed!${NC}"
echo -e "${BLUE}========================================${NC}"
echo -e "Results: ${OUTPUT_DIR}"
echo ""
