#!/bin/bash
#
# Quick verification test - builds light_load with all 6 schedulers
#

cd "$(dirname "$0")/../periodic/light_load"

echo "================================================================================"
echo "Quick Scheduler Build Verification"
echo "================================================================================"
echo ""

CONFIGS=(
    "EDF:CONFIG_SCHED_DEADLINE=y"
    "Weighted_EDF:CONFIG_SCHED_DEADLINE=y:CONFIG_736_MOD_EDF=y"
    "WSRT:CONFIG_SCHED_DEADLINE=y:CONFIG_736_WSRT=y:CONFIG_SCHED_THREAD_USAGE=y:CONFIG_THREAD_RUNTIME_STATS=y:CONFIG_736_TIME_LEFT=y"
    "RMS:CONFIG_SCHED_DEADLINE=y:CONFIG_736_RMS=y"
    "LLF:CONFIG_SCHED_DEADLINE=y:CONFIG_736_LLF=y:CONFIG_SCHED_THREAD_USAGE=y:CONFIG_THREAD_RUNTIME_STATS=y:CONFIG_736_TIME_LEFT=y"
    "PFS:CONFIG_SCHED_DEADLINE=y:CONFIG_736_PFS=y"
)

pass_count=0
fail_count=0

for config in "${CONFIGS[@]}"; do
    IFS=':' read -ra PARTS <<< "$config"
    sched="${PARTS[0]}"
    
    echo -n "Testing $sched ... "
    
    # Create prj.conf
    cat > prj.conf << EOF
CONFIG_736_RT_STATS=y
CONFIG_736_RT_STATS_DETAILED=y
CONFIG_PRINTK=y
CONFIG_CONSOLE=y
CONFIG_SERIAL=y
CONFIG_TICKLESS_KERNEL=y
CONFIG_MAIN_STACK_SIZE=4096
CONFIG_HEAP_MEM_POOL_SIZE=2048
EOF
    
    # Add scheduler configs
    for ((i=1; i<${#PARTS[@]}; i++)); do
        echo "${PARTS[i]}" >> prj.conf
    done
    
    # Build
    rm -rf build
    if west build -b native_sim > /dev/null 2>&1; then
        echo "✓ PASS"
        ((pass_count++))
    else
        echo "✗ FAIL"
        ((fail_count++))
    fi
done

echo ""
echo "================================================================================"
echo "Results: $pass_count passed, $fail_count failed"
echo "================================================================================"

if [ $fail_count -eq 0 ]; then
    echo "SUCCESS: All schedulers build correctly!"
    exit 0
else
    echo "FAILURE: Some schedulers failed to build"
    exit 1
fi
