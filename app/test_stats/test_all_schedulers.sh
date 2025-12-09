#!/bin/bash
#
# Build and Test Script for RT Statistics
# Tests all schedulers and collects comparative data
#

ZEPHYR_BASE="/home/jack/cs736-project/zephyr"
APP_DIR="$ZEPHYR_BASE/app/test_stats"

cd "$ZEPHYR_BASE"

echo "========================================="
echo "RT Statistics Test Suite Builder"
echo "========================================="
echo ""

# Array of schedulers to test
schedulers=(
    "736_MOD_EDF:Weighted EDF"
    "736_RMS:Rate Monotonic"
    "736_WSRT:Weighted SRT"
    "736_LLF:Least Laxity First"
    "736_PFS:Proportional Fair"
)

# Function to test a specific scheduler
test_scheduler() {
    local config=$1
    local name=$2
    
    echo "========================================="
    echo "Testing: $name"
    echo "========================================="
    
    # Build
    echo "Building with CONFIG_$config=y..."
    west build -b native_sim app/test_stats -p -- \
        -DCONFIG_$config=y \
        -DCONFIG_736_RT_STATS=y \
        -DCONFIG_736_RT_STATS_DETAILED=y \
        -DCONFIG_736_RT_STATS_SQUARED=y
    
    if [ $? -ne 0 ]; then
        echo "ERROR: Build failed for $name"
        return 1
    fi
    
    echo ""
    echo "Running test..."
    west build -t run
    
    echo ""
    echo "Test completed for $name"
    echo ""
}

# Main test flow
echo "Select test mode:"
echo "  1) Test all schedulers"
echo "  2) Test specific scheduler"
echo "  3) Quick test (basic stats only)"
echo ""
read -p "Enter choice [1-3]: " choice

case $choice in
    1)
        echo "Testing all schedulers..."
        for sched in "${schedulers[@]}"; do
            IFS=: read -r config name <<< "$sched"
            test_scheduler "$config" "$name"
            echo ""
            read -p "Press Enter to continue to next scheduler..."
        done
        ;;
    
    2)
        echo ""
        echo "Available schedulers:"
        for i in "${!schedulers[@]}"; do
            IFS=: read -r config name <<< "${schedulers[$i]}"
            echo "  $((i+1))) $name"
        done
        echo ""
        read -p "Enter choice [1-${#schedulers[@]}]: " sched_choice
        
        if [ $sched_choice -ge 1 ] && [ $sched_choice -le ${#schedulers[@]} ]; then
            IFS=: read -r config name <<< "${schedulers[$((sched_choice-1))]}"
            test_scheduler "$config" "$name"
        else
            echo "Invalid choice"
            exit 1
        fi
        ;;
    
    3)
        echo "Running quick test with Weighted EDF..."
        west build -b native_sim app/test_stats -p -- \
            -DCONFIG_736_MOD_EDF=y \
            -DCONFIG_736_RT_STATS=y
        west build -t run
        ;;
    
    *)
        echo "Invalid choice"
        exit 1
        ;;
esac

echo ""
echo "========================================="
echo "Test suite completed"
echo "========================================="
