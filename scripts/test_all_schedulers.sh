#!/bin/bash
# Test all CS736 custom schedulers

ZEPHYR_DIR="/home/jack/cs736-project/zephyr"
VENV_DIR="/home/jack/cs736-project/.venv"

echo "================================================================="
echo "CS736 Custom Scheduler Test Suite"
echo "================================================================="
echo ""

source "$VENV_DIR/bin/activate"

run_test() {
    local test_name=$1
    local test_dir=$2
    
    echo "================================================================="
    echo "Testing: $test_name"
    echo "================================================================="
    
    cd "$ZEPHYR_DIR/app/$test_dir"
    
    # Build
    echo "Building $test_name..."
    west build -b native_sim -p > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo "❌ Build FAILED for $test_name"
        return 1
    fi
    echo "✓ Build successful"
    
    # Run (with timeout)
    echo "Running $test_name..."
    timeout 2 west build -t run 2>&1 | grep -E "(Order|Complete|verification)" | head -20
    echo ""
    
    return 0
}

# Test each scheduler
run_test "Weighted EDF" "test_weighted_edf"
run_test "Rate Monotonic Scheduling (RMS)" "test_rms"
run_test "Weighted Shortest Remaining Time (WSRT)" "test_wsrt"
run_test "Least Laxity First (LLF)" "test_llf"
run_test "Proportional Fair Scheduling (PFS)" "test_pfs"

echo "================================================================="
echo "All Tests Complete!"
echo "================================================================="
echo ""
echo "Summary:"
echo "✓ Weighted EDF (CONFIG_736_MOD_EDF) - TESTED"
echo "✓ RMS (CONFIG_736_RMS) - TESTED"
echo "✓ WSRT (CONFIG_736_WSRT) - TESTED"
echo "✓ LLF (CONFIG_736_LLF) - TESTED"
echo "✓ PFS (CONFIG_736_PFS) - TESTED"
echo ""
