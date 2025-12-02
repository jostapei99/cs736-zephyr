#!/bin/bash
#
# Automated Scheduler Comparison Script
# 
# This script automatically tests all Zephyr schedulers and generates
# a comparison report.
#

set -e

ZEPHYR_BASE="/home/jack/cs736-project/zephyr"
APP_PATH="app/scheduler_evaluation/comprehensive_scheduler_test"
RESULTS_DIR="scheduler_comparison_results_$(date +%Y%m%d_%H%M%S)"

cd "$ZEPHYR_BASE"
source ~/.venv/zephyr/bin/activate

echo "========================================="
echo "Comprehensive Scheduler Comparison Test"
echo "========================================="
echo ""
echo "This will test all 4 scheduler configurations:"
echo "  1. SIMPLE (O(N) list)"
echo "  2. SCALABLE (O(log N) red-black tree)"
echo "  3. MULTIQ (O(1) array)"
echo "  4. DEADLINE (EDF with SIMPLE)"
echo ""
echo "Results will be saved to: $RESULTS_DIR"
echo ""

mkdir -p "$RESULTS_DIR"

# Backup original config
cp "$APP_PATH/prj.conf" "$APP_PATH/prj.conf.backup"

# ============================================================================
# Test 1: SIMPLE Scheduler
# ============================================================================

echo ""
echo "========================================="
echo "Test 1/4: SIMPLE Scheduler"
echo "========================================="
echo ""

cat > "$APP_PATH/prj.conf" << 'EOF'
# Comprehensive Scheduler Test - SIMPLE
CONFIG_TIMING_FUNCTIONS=y
CONFIG_PRINTK=y
CONFIG_EARLY_CONSOLE=y
CONFIG_MAIN_STACK_SIZE=2048
CONFIG_THREAD_NAME=y
CONFIG_THREAD_STACK_INFO=y
CONFIG_THREAD_RUNTIME_STATS=y
CONFIG_SCHED_THREAD_USAGE=y
CONFIG_SYS_CLOCK_TICKS_PER_SEC=10000
CONFIG_SCHED_SIMPLE=y
CONFIG_NUM_COOP_PRIORITIES=0
CONFIG_NUM_PREEMPT_PRIORITIES=32
CONFIG_TIMESLICING=n
EOF

west build -p -b qemu_cortex_m3 "$APP_PATH" 2>&1 | tee "$RESULTS_DIR/build_simple.log"
echo "Running SIMPLE scheduler test..."
timeout 35 west build -t run 2>&1 | tee "$RESULTS_DIR/results_simple.txt" || true

# ============================================================================
# Test 2: SCALABLE Scheduler
# ============================================================================

echo ""
echo "========================================="
echo "Test 2/4: SCALABLE Scheduler"
echo "========================================="
echo ""

cat > "$APP_PATH/prj.conf" << 'EOF'
# Comprehensive Scheduler Test - SCALABLE
CONFIG_TIMING_FUNCTIONS=y
CONFIG_PRINTK=y
CONFIG_EARLY_CONSOLE=y
CONFIG_MAIN_STACK_SIZE=2048
CONFIG_THREAD_NAME=y
CONFIG_THREAD_STACK_INFO=y
CONFIG_THREAD_RUNTIME_STATS=y
CONFIG_SCHED_THREAD_USAGE=y
CONFIG_SYS_CLOCK_TICKS_PER_SEC=10000
CONFIG_SCHED_SCALABLE=y
CONFIG_NUM_COOP_PRIORITIES=0
CONFIG_NUM_PREEMPT_PRIORITIES=32
CONFIG_TIMESLICING=n
EOF

west build -p -b qemu_cortex_m3 "$APP_PATH" 2>&1 | tee "$RESULTS_DIR/build_scalable.log"
echo "Running SCALABLE scheduler test..."
timeout 35 west build -t run 2>&1 | tee "$RESULTS_DIR/results_scalable.txt" || true

# ============================================================================
# Test 3: MULTIQ Scheduler
# ============================================================================

echo ""
echo "========================================="
echo "Test 3/4: MULTIQ Scheduler"
echo "========================================="
echo ""

cat > "$APP_PATH/prj.conf" << 'EOF'
# Comprehensive Scheduler Test - MULTIQ
CONFIG_TIMING_FUNCTIONS=y
CONFIG_PRINTK=y
CONFIG_EARLY_CONSOLE=y
CONFIG_MAIN_STACK_SIZE=2048
CONFIG_THREAD_NAME=y
CONFIG_THREAD_STACK_INFO=y
CONFIG_THREAD_RUNTIME_STATS=y
CONFIG_SCHED_THREAD_USAGE=y
CONFIG_SYS_CLOCK_TICKS_PER_SEC=10000
CONFIG_SCHED_MULTIQ=y
CONFIG_NUM_COOP_PRIORITIES=0
CONFIG_NUM_PREEMPT_PRIORITIES=32
CONFIG_TIMESLICING=n
EOF

west build -p -b qemu_cortex_m3 "$APP_PATH" 2>&1 | tee "$RESULTS_DIR/build_multiq.log"
echo "Running MULTIQ scheduler test..."
timeout 35 west build -t run 2>&1 | tee "$RESULTS_DIR/results_multiq.txt" || true

# ============================================================================
# Test 4: DEADLINE Scheduler (EDF)
# ============================================================================

echo ""
echo "========================================="
echo "Test 4/4: DEADLINE Scheduler (EDF)"
echo "========================================="
echo ""

cat > "$APP_PATH/prj.conf" << 'EOF'
# Comprehensive Scheduler Test - DEADLINE
CONFIG_TIMING_FUNCTIONS=y
CONFIG_PRINTK=y
CONFIG_EARLY_CONSOLE=y
CONFIG_MAIN_STACK_SIZE=2048
CONFIG_THREAD_NAME=y
CONFIG_THREAD_STACK_INFO=y
CONFIG_THREAD_RUNTIME_STATS=y
CONFIG_SCHED_THREAD_USAGE=y
CONFIG_SYS_CLOCK_TICKS_PER_SEC=10000
CONFIG_SCHED_SIMPLE=y
CONFIG_SCHED_DEADLINE=y
CONFIG_NUM_COOP_PRIORITIES=0
CONFIG_NUM_PREEMPT_PRIORITIES=32
CONFIG_TIMESLICING=n
EOF

west build -p -b qemu_cortex_m3 "$APP_PATH" 2>&1 | tee "$RESULTS_DIR/build_deadline.log"
echo "Running DEADLINE scheduler test..."
timeout 35 west build -t run 2>&1 | tee "$RESULTS_DIR/results_deadline.txt" || true

# ============================================================================
# Generate Comparison Report
# ============================================================================

echo ""
echo "========================================="
echo "Generating Comparison Report"
echo "========================================="
echo ""

cat > "$RESULTS_DIR/COMPARISON_REPORT.md" << 'EOF'
# Scheduler Comparison Report

This report compares all 4 Zephyr scheduler configurations on the comprehensive test suite.

## Test Configuration
- **Board**: qemu_cortex_m3 (12 MHz ARM Cortex-M3)
- **Test Duration**: 30 seconds per scheduler
- **Phases**: 6 test phases per scheduler

## How to Read This Report

### Phase 1: Periodic Tasks
Look for deadline misses and max response time.
- **Lower is better** for both metrics

### Phase 2: Event-Driven
Compare max latency across priorities.
- **Lower latency = better responsiveness**

### Phase 3: Scalability
**This is the KEY differentiator!**
Compare throughput at 1, 5, 10, 20 threads.
- **SIMPLE**: Should degrade as threads increase (O(N))
- **SCALABLE**: Should scale better (O(log N))
- **MULTIQ**: Should be constant (O(1))

### Phase 4: Priority Inversion
Check inversion duration.
- **<6ms = good** (priority inheritance working)
- **>10ms = problematic**

### Phase 5: Overload
Under overload, which tasks miss deadlines?
- **Lower priority tasks should miss first**
- Compare max tardiness values

### Phase 6: Deadline (EDF)
Only DEADLINE scheduler runs this phase.
- Should have **0 deadline misses** with proper configuration

## Quick Comparison

Extract key metrics from each results file:

```bash
# Phase 3 scalability - the smoking gun!
echo "=== SCALABILITY COMPARISON ==="
echo "SIMPLE:"
grep "iterations" results_simple.txt | grep "threads:"
echo ""
echo "SCALABLE:"
grep "iterations" results_scalable.txt | grep "threads:"
echo ""
echo "MULTIQ:"
grep "iterations" results_multiq.txt | grep "threads:"
echo ""

# Phase 4 priority inversion
echo "=== PRIORITY INVERSION ==="
grep "Priority inversion duration" results_*.txt
echo ""

# Phase 5 overload misses
echo "=== OVERLOAD DEADLINE MISSES ==="
echo "SIMPLE:"
grep -A 10 "Overload:" results_simple.txt | grep "Misses"
echo ""
echo "SCALABLE:"
grep -A 10 "Overload:" results_scalable.txt | grep "Misses"
echo ""
echo "MULTIQ:"
grep -A 10 "Overload:" results_multiq.txt | grep "Misses"
```

## Recommendations

Based on the results:

### Use SIMPLE if:
- You have <10 threads
- Code size is critical
- You need EDF deadline scheduling

### Use SCALABLE if:
- You have >20 threads
- Thread count varies
- You need SMP support

### Use MULTIQ if:
- You need O(1) determinism
- Traditional RTOS behavior
- Moderate thread count

### Use DEADLINE if:
- You have explicit deadline requirements
- Need optimal single-core scheduling
- Can use with SIMPLE scheduler

## Detailed Results

See individual result files:
- results_simple.txt
- results_scalable.txt
- results_multiq.txt
- results_deadline.txt

EOF

# Restore original config
cp "$APP_PATH/prj.conf.backup" "$APP_PATH/prj.conf"

echo ""
echo "========================================="
echo "All Tests Complete!"
echo "========================================="
echo ""
echo "Results saved to: $RESULTS_DIR/"
echo ""
echo "View comparison:"
echo "  cat $RESULTS_DIR/COMPARISON_REPORT.md"
echo ""
echo "View detailed results:"
echo "  cat $RESULTS_DIR/results_simple.txt"
echo "  cat $RESULTS_DIR/results_scalable.txt"
echo "  cat $RESULTS_DIR/results_multiq.txt"
echo "  cat $RESULTS_DIR/results_deadline.txt"
echo ""

# Generate quick stats
echo "=== Quick Statistics ==="
echo ""
echo "Phase 3 Scalability (15 threads):"
echo "SIMPLE:    $(grep "15 threads:" "$RESULTS_DIR/results_simple.txt" | tail -1)"
echo "SCALABLE:  $(grep "15 threads:" "$RESULTS_DIR/results_scalable.txt" | tail -1)"
echo "MULTIQ:    $(grep "15 threads:" "$RESULTS_DIR/results_multiq.txt" | tail -1)"
echo ""
echo "Phase 4 Priority Inversion:"
grep "Priority inversion duration" "$RESULTS_DIR/results_"*.txt
echo ""

echo "Full report: $RESULTS_DIR/COMPARISON_REPORT.md"
