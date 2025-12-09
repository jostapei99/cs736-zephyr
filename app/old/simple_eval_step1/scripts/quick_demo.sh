#!/bin/bash
#
# Quick Demo - Test a few scheduler/workload combinations
# Use this to verify the setup works before running the full test suite
#

set -e

PROJECT_ROOT="/home/jack/cs736-project/zephyr"
APP_DIR="${PROJECT_ROOT}/app/simple_eval_step1"
OUTPUT_DIR="${APP_DIR}/results"
TIMEOUT=15

# Just test 4 combinations (one from each category)
TESTS=(
    "EDF:CONFIG_SCHED_DEADLINE:LIGHT:WORKLOAD_LIGHT"
    "WEIGHTED_EDF:CONFIG_736_MOD_EDF:MEDIUM:WORKLOAD_MEDIUM"
    "WSRT:CONFIG_736_WSRT:HEAVY:WORKLOAD_HEAVY"
    "RMS:CONFIG_736_RMS:OVERLOAD:WORKLOAD_OVERLOAD"
)

GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}RT Scheduler Evaluation - Quick Demo${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

mkdir -p "${OUTPUT_DIR}"

# Backup original files
cp "${APP_DIR}/prj.conf" "${OUTPUT_DIR}/prj.conf.backup"
cp "${APP_DIR}/include/workloads.h" "${OUTPUT_DIR}/workloads.h.backup"

test_num=0
for test in "${TESTS[@]}"; do
    IFS=':' read -r sched_name sched_config wl_name wl_config <<< "$test"
    test_num=$((test_num + 1))
    
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${YELLOW}Demo Test ${test_num}/4: ${sched_name} + ${wl_name}${NC}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    
    # Update configurations
    sed -i "s/#define CURRENT_WORKLOAD WORKLOAD_.*/#define CURRENT_WORKLOAD ${wl_config}/" \
        "${APP_DIR}/include/workloads.h"
    
    sed -i 's/^CONFIG_736_MOD_EDF=y/# CONFIG_736_MOD_EDF=y/' "${APP_DIR}/prj.conf"
    sed -i 's/^CONFIG_736_WSRT=y/# CONFIG_736_WSRT=y/' "${APP_DIR}/prj.conf"
    sed -i 's/^CONFIG_736_RMS=y/# CONFIG_736_RMS=y/' "${APP_DIR}/prj.conf"
    
    if [ "$sched_config" != "CONFIG_SCHED_DEADLINE" ]; then
        sed -i "s/# ${sched_config}=y/${sched_config}=y/" "${APP_DIR}/prj.conf"
    fi
    
    # Build
    cd "${PROJECT_ROOT}"
    echo -e "Building..."
    if ! west build -b native_sim app/simple_eval_step1 > /dev/null 2>&1; then
        echo -e "${RED}✗ Build failed${NC}"
        continue
    fi
    echo -e "${GREEN}✓ Build successful${NC}"
    
    # Run
    output_file="${OUTPUT_DIR}/${sched_name}_${wl_name}.log"
    csv_file="${OUTPUT_DIR}/${sched_name}_${wl_name}.csv"
    
    echo -e "Running..."
    timeout ${TIMEOUT}s west build -t run > "${output_file}" 2>&1 || true
    
    grep "^CSV," "${output_file}" > "${csv_file}" 2>/dev/null || true
    csv_lines=$(wc -l < "${csv_file}")
    echo -e "${GREEN}✓ Captured ${csv_lines} CSV records${NC}"
    echo ""
done

# Restore
cp "${OUTPUT_DIR}/prj.conf.backup" "${APP_DIR}/prj.conf"
cp "${OUTPUT_DIR}/workloads.h.backup" "${APP_DIR}/include/workloads.h"
rm "${OUTPUT_DIR}/prj.conf.backup" "${OUTPUT_DIR}/workloads.h.backup"

echo -e "${BLUE}========================================${NC}"
echo -e "${GREEN}✓ Demo completed!${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""
echo "Results: ${OUTPUT_DIR}"
echo ""
echo "Next steps:"
echo "  1. Review CSV files: ls ${OUTPUT_DIR}/*.csv"
echo "  2. Run full test suite: bash scripts/run_all_tests.sh"
echo "  3. Generate graphs: python3 scripts/generate_graphs.py"
echo ""
