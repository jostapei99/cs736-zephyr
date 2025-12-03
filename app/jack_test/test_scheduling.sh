#!/bin/bash
# Quick test to verify task scheduling

echo "Testing task scheduling with new preemptive priorities..."

cd /home/jack/cs736-project/zephyr/app/jack_test

# Rebuild with new priorities
ninja -C build

echo "Running 5-second test using existing run script..."
timeout 5s ./run_and_analyze.sh 2>&1 | head -50 | tee scheduling_test.log

echo -e "\n=== TASK EXECUTION SUMMARY ==="
echo "Critical tasks:"
grep -c "CRIT:" scheduling_test.log || echo "0"

echo "High priority tasks:"
grep -c "NAV:\|COMM:" scheduling_test.log || echo "0"  

echo "Medium priority tasks:"
grep -c "SENSOR:" scheduling_test.log || echo "0"

echo "Low priority tasks:"
grep -c "HOUSE:" scheduling_test.log || echo "0"

echo -e "\nAll tasks should now execute!"