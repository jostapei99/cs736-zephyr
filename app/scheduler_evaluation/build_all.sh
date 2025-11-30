#!/bin/bash
# Quick Build and Test Script for All Workloads
# Usage: ./build_all_workloads.sh [board]
# Example: ./build_all_workloads.sh qemu_cortex_m3

BOARD=${1:-qemu_cortex_m3}
ZEPHYR_BASE=${ZEPHYR_BASE:-"../../.."}
RESULTS_DIR="./results_$(date +%Y%m%d_%H%M%S)"

echo "=================================================="
echo "Scheduler Evaluation - Build All Workloads"
echo "Board: $BOARD"
echo "Results: $RESULTS_DIR"
echo "=================================================="

mkdir -p "$RESULTS_DIR"

WORKLOADS=(
    "workload1_periodic_control"
    "workload2_event_driven"
    "workload3_mixed_criticality"
    "workload4_deadline_sporadic"
)

SCHEDULERS=(
    "simple"
    "scalable"
    "multiq"
    "deadline"
)

# Function to build a workload
build_workload() {
    local workload=$1
    local scheduler=$2
    
    echo ""
    echo "----------------------------------------"
    echo "Building: $workload with $scheduler scheduler"
    echo "----------------------------------------"
    
    cd "$ZEPHYR_BASE"
    
    # Clean build
    west build -t pristine -b "$BOARD" "app/scheduler_evaluation/$workload" 2>&1 | tee "$RESULTS_DIR/${workload}_${scheduler}_build.log"
    
    # Modify prj.conf based on scheduler
    local prj_conf="app/scheduler_evaluation/$workload/prj.conf"
    local backup="app/scheduler_evaluation/$workload/prj.conf.backup"
    
    # Backup original
    cp "$prj_conf" "$backup"
    
    # Modify for scheduler type
    case $scheduler in
        "simple")
            sed -i 's/# CONFIG_SCHED_SIMPLE is not set/CONFIG_SCHED_SIMPLE=y/' "$prj_conf"
            sed -i 's/CONFIG_SCHED_SCALABLE=y/# CONFIG_SCHED_SCALABLE is not set/' "$prj_conf"
            sed -i 's/CONFIG_SCHED_MULTIQ=y/# CONFIG_SCHED_MULTIQ is not set/' "$prj_conf"
            sed -i 's/CONFIG_SCHED_DEADLINE=y/# CONFIG_SCHED_DEADLINE is not set/' "$prj_conf"
            ;;
        "scalable")
            sed -i 's/CONFIG_SCHED_SIMPLE=y/# CONFIG_SCHED_SIMPLE is not set/' "$prj_conf"
            sed -i 's/# CONFIG_SCHED_SCALABLE is not set/CONFIG_SCHED_SCALABLE=y/' "$prj_conf"
            sed -i 's/CONFIG_SCHED_MULTIQ=y/# CONFIG_SCHED_MULTIQ is not set/' "$prj_conf"
            sed -i 's/CONFIG_SCHED_DEADLINE=y/# CONFIG_SCHED_DEADLINE is not set/' "$prj_conf"
            ;;
        "multiq")
            # Skip workload4 for multiq (incompatible with deadline)
            if [[ "$workload" == "workload4_deadline_sporadic" ]]; then
                echo "Skipping $workload with multiq (incompatible)"
                cp "$backup" "$prj_conf"
                return
            fi
            sed -i 's/CONFIG_SCHED_SIMPLE=y/# CONFIG_SCHED_SIMPLE is not set/' "$prj_conf"
            sed -i 's/CONFIG_SCHED_SCALABLE=y/# CONFIG_SCHED_SCALABLE is not set/' "$prj_conf"
            sed -i 's/# CONFIG_SCHED_MULTIQ is not set/CONFIG_SCHED_MULTIQ=y/' "$prj_conf"
            sed -i 's/CONFIG_SCHED_DEADLINE=y/# CONFIG_SCHED_DEADLINE is not set/' "$prj_conf"
            ;;
        "deadline")
            sed -i 's/# CONFIG_SCHED_SIMPLE is not set/CONFIG_SCHED_SIMPLE=y/' "$prj_conf"
            sed -i 's/CONFIG_SCHED_SCALABLE=y/# CONFIG_SCHED_SCALABLE is not set/' "$prj_conf"
            sed -i 's/CONFIG_SCHED_MULTIQ=y/# CONFIG_SCHED_MULTIQ is not set/' "$prj_conf"
            sed -i 's/# CONFIG_SCHED_DEADLINE is not set/CONFIG_SCHED_DEADLINE=y/' "$prj_conf"
            ;;
    esac
    
    # Build
    west build -b "$BOARD" "app/scheduler_evaluation/$workload" 2>&1 | tee -a "$RESULTS_DIR/${workload}_${scheduler}_build.log"
    
    if [ $? -eq 0 ]; then
        echo "Build successful!"
        
        # Run if QEMU board
        if [[ "$BOARD" == qemu* ]]; then
            echo "Running test..."
            timeout 60 west build -t run 2>&1 | tee "$RESULTS_DIR/${workload}_${scheduler}_run.log"
            echo "Test completed. Results saved to $RESULTS_DIR/${workload}_${scheduler}_run.log"
        fi
    else
        echo "Build failed! Check log: $RESULTS_DIR/${workload}_${scheduler}_build.log"
    fi
    
    # Restore original
    cp "$backup" "$prj_conf"
    rm "$backup"
}

# Main execution
echo "Starting batch build..."
echo ""

for workload in "${WORKLOADS[@]}"; do
    for scheduler in "${SCHEDULERS[@]}"; do
        build_workload "$workload" "$scheduler"
    done
done

echo ""
echo "=================================================="
echo "All builds complete!"
echo "Results directory: $RESULTS_DIR"
echo "=================================================="
echo ""
echo "To analyze results:"
echo "  grep 'Deadline Misses' $RESULTS_DIR/*_run.log"
echo "  grep 'Tardiness Rate' $RESULTS_DIR/*_run.log"
echo "  grep 'Avg Latency' $RESULTS_DIR/*_run.log"
