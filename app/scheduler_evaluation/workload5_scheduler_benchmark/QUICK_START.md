# Quick Scheduler Comparison Guide

## Running the Benchmark

This is a simple 3-step process to compare all schedulers:

### Step 1: Test with SIMPLE scheduler
```bash
cd /home/jack/cs736-project/zephyr

# Build with default SIMPLE scheduler
west build -p -b qemu_cortex_m3 app/scheduler_evaluation/workload5_scheduler_benchmark
west build -t run > results_simple.txt 2>&1
```

### Step 2: Test with SCALABLE scheduler
```bash
# Edit prj.conf to use SCALABLE
sed -i 's/CONFIG_SCHED_SIMPLE=y/# CONFIG_SCHED_SIMPLE=y/' app/scheduler_evaluation/workload5_scheduler_benchmark/prj.conf
echo "CONFIG_SCHED_SCALABLE=y" >> app/scheduler_evaluation/workload5_scheduler_benchmark/prj.conf

# Build and run
west build -p -b qemu_cortex_m3 app/scheduler_evaluation/workload5_scheduler_benchmark
west build -t run > results_scalable.txt 2>&1
```

### Step 3: Test with MULTIQ scheduler
```bash
# Edit prj.conf to use MULTIQ
sed -i 's/CONFIG_SCHED_SCALABLE=y/# CONFIG_SCHED_SCALABLE=y/' app/scheduler_evaluation/workload5_scheduler_benchmark/prj.conf
echo "CONFIG_SCHED_MULTIQ=y" >> app/scheduler_evaluation/workload5_scheduler_benchmark/prj.conf

# Build and run
west build -p -b qemu_cortex_m3 app/scheduler_evaluation/workload5_scheduler_benchmark
west build -t run > results_multiq.txt 2>&1
```

### Step 4: Compare results
```bash
# View results side by side
echo "=== SIMPLE SCHEDULER ==="
grep -A 5 "Aggregate Statistics" results_simple.txt

echo "=== SCALABLE SCHEDULER ==="
grep -A 5 "Aggregate Statistics" results_scalable.txt

echo "=== MULTIQ SCHEDULER ==="
grep -A 5 "Aggregate Statistics" results_multiq.txt
```

## What to Look For

### Latency (Phase 1)
```
Average latency: XXX us
Max latency: XXX us
```
- **Lower is better**
- Watch how it changes from 1→5→10→20→30 threads
- SIMPLE should degrade linearly
- SCALABLE/MULTIQ should stay relatively flat

### Throughput (Phase 2)
```
Throughput: XXX iterations/sec
```
- **Higher is better**
- Shows how many context switches per second
- MULTIQ should be highest
- SIMPLE should drop with many threads

### Context Switches
```
Switches/sec: XXX
```
- **Higher = less overhead**
- Shows scheduler efficiency

## Quick Decision Matrix

| Your Situation | Recommended Scheduler |
|----------------|----------------------|
| <10 threads, code size critical | **SIMPLE** |
| 10-20 threads, real-time system | **MULTIQ** |
| >20 threads, dynamic workload | **SCALABLE** |
| Need deadline scheduling | **SIMPLE** + SCHED_DEADLINE |
| SMP/multi-core system | **SCALABLE** |

## Example Results Interpretation

### Good SIMPLE Performance:
```
Testing with 5 threads:
  Throughput: 10000 iterations/sec
  Average latency: 50 us

Testing with 30 threads:
  Throughput: 8000 iterations/sec  ← Only slight drop
  Average latency: 80 us           ← Acceptable increase
```

### Poor SIMPLE Performance (switch to MULTIQ):
```
Testing with 5 threads:
  Throughput: 10000 iterations/sec
  Average latency: 50 us

Testing with 30 threads:
  Throughput: 2000 iterations/sec  ← Major drop!
  Average latency: 500 us          ← 10x worse!
```

### Ideal MULTIQ Performance:
```
Testing with 5 threads:
  Throughput: 12000 iterations/sec
  Average latency: 40 us

Testing with 30 threads:
  Throughput: 11500 iterations/sec ← Stays high!
  Average latency: 45 us           ← Minimal increase
```

## Automated Test Script

Save this as `test_all_schedulers.sh`:

```bash
#!/bin/bash
cd /home/jack/cs736-project/zephyr

PRJCONF="app/scheduler_evaluation/workload5_scheduler_benchmark/prj.conf"

echo "Testing all schedulers..."

# Test SIMPLE
echo "CONFIG_SCHED_SIMPLE=y" > $PRJCONF
cat >> $PRJCONF << EOF
CONFIG_TIMING_FUNCTIONS=y
CONFIG_PRINTK=y
CONFIG_EARLY_CONSOLE=y
CONFIG_MAIN_STACK_SIZE=4096
CONFIG_THREAD_NAME=y
CONFIG_THREAD_STACK_INFO=y
CONFIG_SYS_CLOCK_TICKS_PER_SEC=10000
CONFIG_NUM_COOP_PRIORITIES=0
CONFIG_NUM_PREEMPT_PRIORITIES=32
CONFIG_TIMESLICING=n
CONFIG_THREAD_RUNTIME_STATS=y
CONFIG_SCHED_THREAD_USAGE=y
EOF

west build -p -b qemu_cortex_m3 app/scheduler_evaluation/workload5_scheduler_benchmark
echo "Running SIMPLE scheduler test..."
timeout 60 west build -t run 2>&1 | tee results_simple.txt

# Test SCALABLE
sed -i 's/CONFIG_SCHED_SIMPLE=y/CONFIG_SCHED_SCALABLE=y/' $PRJCONF
west build -p -b qemu_cortex_m3 app/scheduler_evaluation/workload5_scheduler_benchmark
echo "Running SCALABLE scheduler test..."
timeout 60 west build -t run 2>&1 | tee results_scalable.txt

# Test MULTIQ
sed -i 's/CONFIG_SCHED_SCALABLE=y/CONFIG_SCHED_MULTIQ=y/' $PRJCONF
west build -p -b qemu_cortex_m3 app/scheduler_evaluation/workload5_scheduler_benchmark
echo "Running MULTIQ scheduler test..."
timeout 60 west build -t run 2>&1 | tee results_multiq.txt

echo "Done! Results saved to results_*.txt"
```

Make it executable and run:
```bash
chmod +x test_all_schedulers.sh
./test_all_schedulers.sh
```

## Memory Footprint Comparison

After each build, check code size:

```bash
# For each scheduler
west build -t rom_report | grep "Total"
west build -t ram_report | grep "Total"
```

Expected differences:
- SIMPLE: ~2KB smaller FLASH
- MULTIQ: Moderate, needs priority array in RAM
- SCALABLE: ~2KB larger FLASH (rbtree code)

## Integration with Your Project

After determining the best scheduler:

1. Copy the winning configuration to your `prj.conf`:
```
CONFIG_SCHED_MULTIQ=y  # or whichever won
```

2. Document your decision:
```
# Scheduler choice: MULTIQ
# Reason: System has 15 threads, needs O(1) real-time performance
# Benchmark results: 12000 iter/sec throughput, 45us avg latency
# Date tested: 2025-12-01
```

3. Re-run benchmark periodically if thread count changes significantly

## Troubleshooting

### Benchmark hangs or doesn't output
- Increase QEMU timeout: `timeout 120 west build -t run`
- Check console output is enabled: `CONFIG_PRINTK=y`

### Results seem inconsistent
- Run multiple times and average results
- QEMU timing may vary, test on real hardware for final decision

### All schedulers show similar results
- Your thread count may be too low to show differences
- Try increasing `MAX_THREADS` in source code
- Test on real hardware with actual workload
