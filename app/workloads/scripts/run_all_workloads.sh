#!/bin/bash
#
# Run all workloads with all 6 schedulers and collect results
#
# Usage: ./run_all_workloads.sh [duration_ms]
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKLOADS_DIR="$(dirname "$SCRIPT_DIR")"
RESULTS_DIR="${WORKLOADS_DIR}/results"
TEST_DURATION=${1:-10000}

# Activate venv if it exists
VENV_ACTIVATE="/home/jack/cs736-project/.venv/bin/activate"
if [ -f "$VENV_ACTIVATE" ]; then
    source "$VENV_ACTIVATE"
fi

# Scheduler configurations
SCHEDULERS=(
    "EDF:CONFIG_SCHED_DEADLINE_ONLY=y"
    "Weighted_EDF:CONFIG_736_MOD_EDF=y"
    "WSRT:CONFIG_736_WSRT=y:CONFIG_SCHED_THREAD_USAGE=y:CONFIG_THREAD_RUNTIME_STATS=y:CONFIG_736_TIME_LEFT=y"
    "RMS:CONFIG_736_RMS=y"
    "LLF:CONFIG_736_LLF=y:CONFIG_SCHED_THREAD_USAGE=y:CONFIG_THREAD_RUNTIME_STATS=y:CONFIG_736_TIME_LEFT=y"
    "PFS:CONFIG_736_PFS=y"
)

# Workloads to test
WORKLOADS=(
    "periodic/light_load"
    "periodic/heavy_load"
    "mixed_criticality/high_low"
    "overload/sustained_overload"
)

echo "================================================================================"
echo "RT Scheduler Workload Evaluation - Automated Test Suite"
echo "================================================================================"
echo ""
echo "Test Duration: ${TEST_DURATION} ms"
echo "Number of Schedulers: ${#SCHEDULERS[@]}"
echo "Number of Workloads: ${#WORKLOADS[@]}"
echo "Total Tests: $((${#SCHEDULERS[@]} * ${#WORKLOADS[@]}))"
echo ""
echo "Results will be saved to: ${RESULTS_DIR}"
echo ""

# Create results directory
mkdir -p "${RESULTS_DIR}"

# Summary CSV
SUMMARY_CSV="${RESULTS_DIR}/summary.csv"
echo "workload,scheduler,total_activations,deadline_misses,miss_rate_percent,avg_response_ms,jitter_ms,test_duration_ms" > "${SUMMARY_CSV}"

test_count=0
total_tests=$((${#SCHEDULERS[@]} * ${#WORKLOADS[@]}))

# Iterate through workloads
for workload_path in "${WORKLOADS[@]}"; do
    workload_name=$(basename "${workload_path}")
    workload_dir="${WORKLOADS_DIR}/${workload_path}"
    
    if [ ! -d "${workload_dir}" ]; then
        echo "WARNING: Workload directory not found: ${workload_dir}"
        continue
    fi
    
    echo "--------------------------------------------------------------------------------"
    echo "Testing Workload: ${workload_name}"
    echo "--------------------------------------------------------------------------------"
    
    # Iterate through schedulers
    for sched_config in "${SCHEDULERS[@]}"; do
        IFS=':' read -ra PARTS <<< "$sched_config"
        sched_name="${PARTS[0]}"
        
        test_count=$((test_count + 1))
        echo ""
        echo "[${test_count}/${total_tests}] Running: ${workload_name} with ${sched_name}"
        
        # Create result directory for this combination
        result_dir="${RESULTS_DIR}/${workload_name}/${sched_name}"
        mkdir -p "${result_dir}"
        
        # Backup original prj.conf
        cp "${workload_dir}/prj.conf" "${workload_dir}/prj.conf.bak"
        
        # Create modified prj.conf with scheduler config
        {
            echo "# Auto-generated configuration for ${sched_name}"
            echo "CONFIG_SCHED_DEADLINE=y"
            
            # Add scheduler-specific configs
            for ((i=1; i<${#PARTS[@]}; i++)); do
                echo "${PARTS[i]}"
            done
            
            # Add common configs
            echo "CONFIG_736_RT_STATS=y"
            echo "CONFIG_736_RT_STATS_DETAILED=y"
            echo "CONFIG_PRINTK=y"
            echo "CONFIG_CBPRINTF_FP_SUPPORT=y"
            echo "CONFIG_CONSOLE=y"
            echo "CONFIG_SERIAL=y"
            echo "CONFIG_TICKLESS_KERNEL=y"
            echo "CONFIG_MAIN_STACK_SIZE=4096"
            echo "CONFIG_HEAP_MEM_POOL_SIZE=2048"
        } > "${workload_dir}/prj.conf"
        
        # Build
        echo "  Building..."
        (cd "${workload_dir}" && west build -b native_sim -p > "${result_dir}/build.log" 2>&1)
        
        if [ $? -ne 0 ]; then
            echo "  ERROR: Build failed for ${sched_name}"
            echo "  See log: ${result_dir}/build.log"
            mv "${workload_dir}/prj.conf.bak" "${workload_dir}/prj.conf"
            continue
        fi
        
        # Run
        echo "  Running for ${TEST_DURATION} ms..."
        (cd "${workload_dir}" && timeout 30s west build -t run > "${result_dir}/output.log" 2>&1) || true
        
        # Extract CSV data
        grep "^[0-9]" "${result_dir}/output.log" | grep -v "^CONFIG" > "${result_dir}/data.csv" || true
        
        # Extract summary statistics
        deadline_misses=$(grep "Deadline Misses:" "${result_dir}/output.log" | awk '{print $3}' || echo "0")
        total_activations=$(grep "Total Activations:" "${result_dir}/output.log" | awk '{print $3}' || echo "0")
        avg_response=$(grep "Avg Response Time:" "${result_dir}/output.log" | awk '{print $4}' || echo "0")
        jitter=$(grep "Response Time Jitter:" "${result_dir}/output.log" | awk '{print $4}' || echo "0")
        
        # Calculate miss rate
        if [ "$total_activations" -gt 0 ]; then
            miss_rate=$(echo "scale=2; 100 * $deadline_misses / $total_activations" | bc)
        else
            miss_rate="0"
        fi
        
        echo "  Results: ${deadline_misses}/${total_activations} misses (${miss_rate}%)"
        
        # Append to summary CSV
        echo "${workload_name},${sched_name},${total_activations},${deadline_misses},${miss_rate},${avg_response},${jitter},${TEST_DURATION}" >> "${SUMMARY_CSV}"
        
        # Restore original prj.conf
        mv "${workload_dir}/prj.conf.bak" "${workload_dir}/prj.conf"
        
        # Clean build directory
        rm -rf "${workload_dir}/build"
    done
    
    echo ""
done

echo ""
echo "================================================================================"
echo "Evaluation Complete!"
echo "================================================================================"
echo ""
echo "Results saved to: ${RESULTS_DIR}"
echo "Summary CSV: ${SUMMARY_CSV}"
echo ""
echo "To analyze results, run:"
echo "  python3 ${SCRIPT_DIR}/compare_schedulers.py"
echo "  python3 ${SCRIPT_DIR}/plot_results.py"
echo ""
