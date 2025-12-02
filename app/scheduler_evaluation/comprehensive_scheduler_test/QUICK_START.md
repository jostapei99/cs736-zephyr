# Quick Start Guide

## Run Single Test (Manual)

### 1. Test SIMPLE scheduler (default):
```bash
cd /home/jack/cs736-project/zephyr
source ~/.venv/zephyr/bin/activate
west build -p -b qemu_cortex_m3 app/scheduler_evaluation/comprehensive_scheduler_test
timeout 35 west build -t run
```

### 2. Test SCALABLE scheduler:
```bash
# Edit prj.conf - change from SIMPLE to SCALABLE
vim app/scheduler_evaluation/comprehensive_scheduler_test/prj.conf

# Change:
# CONFIG_SCHED_SIMPLE=y
# to:
# CONFIG_SCHED_SCALABLE=y

west build -p -b qemu_cortex_m3 app/scheduler_evaluation/comprehensive_scheduler_test
timeout 35 west build -t run
```

### 3. Test MULTIQ scheduler:
```bash
# Edit prj.conf - change to MULTIQ
# CONFIG_SCHED_MULTIQ=y

west build -p -b qemu_cortex_m3 app/scheduler_evaluation/comprehensive_scheduler_test
timeout 35 west build -t run
```

### 4. Test DEADLINE scheduler:
```bash
# Edit prj.conf - add DEADLINE
# CONFIG_SCHED_SIMPLE=y
# CONFIG_SCHED_DEADLINE=y

west build -p -b qemu_cortex_m3 app/scheduler_evaluation/comprehensive_scheduler_test
timeout 35 west build -t run
```

## Run All Tests (Automated)

```bash
cd /home/jack/cs736-project/zephyr
chmod +x app/scheduler_evaluation/comprehensive_scheduler_test/test_all_schedulers.sh
./app/scheduler_evaluation/comprehensive_scheduler_test/test_all_schedulers.sh
```

This will:
- Test all 4 schedulers automatically
- Save results to timestamped directory
- Generate comparison report
- Show quick statistics

## What to Look For

### Phase 1 (Periodic Tasks)
- Should have 0 deadline misses under normal load
- All schedulers should perform similarly here

### Phase 2 (Event-Driven)
- Low latency = good
- All schedulers should be similar

### Phase 3 (Scalability) ⭐ **KEY TEST**
- Watch throughput at different thread counts
- **SIMPLE**: Degrades as threads increase (O(N))
- **SCALABLE**: Better scaling (O(log N))
- **MULTIQ**: Constant performance (O(1))

Example output:
```
Testing with 1 threads...
  1 threads: 18000 iterations, 9000 iter/sec
Testing with 5 threads...
  5 threads: 85000 iterations, 42500 iter/sec
Testing with 10 threads...
  10 threads: 160000 iterations, 80000 iter/sec
Testing with 15 threads...
  15 threads: 280000 iterations, 140000 iter/sec  <-- Compare this!
```

### Phase 4 (Priority Inversion)
- Duration should be <6ms with priority inheritance
- All schedulers support priority inheritance

### Phase 5 (Overload)
- Under overload, lower priority tasks should miss deadlines
- Compare which tasks miss and how much tardiness

### Phase 6 (Deadline/EDF)
- Only runs with CONFIG_SCHED_DEADLINE=y
- Should have 0 deadline misses

## Comparing Results

After running all tests:

```bash
# Compare scalability (most important!)
grep "15 threads:" results_*/results_*.txt

# Compare priority inversion
grep "Priority inversion duration" results_*/results_*.txt

# Compare overload behavior
grep -A 5 "Overload:" results_*/results_*.txt
```

## Quick Decision Guide

**Choose SIMPLE if:**
- ✓ You have <10 threads
- ✓ Code size is critical
- ✓ You need EDF scheduling

**Choose SCALABLE if:**
- ✓ You have >20 threads
- ✓ Thread count varies
- ✓ You need good scaling

**Choose MULTIQ if:**
- ✓ You need O(1) performance
- ✓ Predictable real-time behavior
- ✓ Traditional RTOS feel

**Choose DEADLINE if:**
- ✓ You have explicit deadlines
- ✓ Need optimal scheduling
- ✓ Can use SIMPLE as base

## Expected Runtime

- Single test: ~30 seconds
- All 4 tests: ~2-3 minutes total

## Troubleshooting

**Test times out:**
- Increase timeout: `timeout 60 west build -t run`

**Build fails:**
- Clean build: `west build -t pristine`
- Check prj.conf syntax

**No output:**
- Make sure CONFIG_PRINTK=y
- Check QEMU started correctly

## Next Steps

1. Run the automated script
2. Review the comparison report
3. Look at Phase 3 scalability results
4. Choose the best scheduler for your needs
5. Apply to your real application

See README.md for detailed information.
