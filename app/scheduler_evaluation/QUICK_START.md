# Quick Start Guide - Scheduler Evaluation Workloads

## Immediate Testing - Try It Now!

### 1. Build and Run Workload 1 (Easiest)
```bash
cd /home/jack/cs736-project/zephyr
west build -b qemu_cortex_m3 app/scheduler_evaluation/workload1_periodic_control
west build -t run
```

Press `Ctrl+A` then `X` to exit QEMU when done.

### 2. What You'll See
```
=== Workload 1: Periodic Control System Results ===

Sensor Thread (High Priority, Period: 10ms):
  Executions: 1000
  Deadline Misses: 0
  Avg Latency: 45 us
  Max Latency: 120 us
  Avg Response Time: 2100 us
  Tardiness Rate: 0.00%

Control Thread (Medium Priority, Period: 20ms):
  Executions: 500
  Deadline Misses: 0
  ...
```

### 3. Test Different Scheduler
```bash
# Edit the config file
vim app/scheduler_evaluation/workload1_periodic_control/prj.conf

# Change from:
CONFIG_SCHED_SIMPLE=y

# To:
# CONFIG_SCHED_SIMPLE is not set
CONFIG_SCHED_SCALABLE=y

# Rebuild
west build -t pristine
west build -b qemu_cortex_m3 app/scheduler_evaluation/workload1_periodic_control
west build -t run
```

## Test All Workloads Quickly

### Option A: Manual Testing
```bash
cd /home/jack/cs736-project/zephyr

# Workload 1
west build -b qemu_cortex_m3 app/scheduler_evaluation/workload1_periodic_control
west build -t run

# Workload 2  
west build -b qemu_cortex_m3 app/scheduler_evaluation/workload2_event_driven
west build -t run

# Workload 3
west build -b qemu_cortex_m3 app/scheduler_evaluation/workload3_mixed_criticality
west build -t run

# Workload 4 (Tests EDF!)
west build -b qemu_cortex_m3 app/scheduler_evaluation/workload4_deadline_sporadic
west build -t run
```

### Option B: Automated Testing (All Workloads, All Schedulers)
```bash
cd /home/jack/cs736-project/zephyr/samples/scheduler_evaluation
./build_all.sh qemu_cortex_m3
```

This will:
- Build all 4 workloads
- Test with 4 different schedulers each
- Save results to timestamped directory
- Takes about 20-30 minutes

## Understanding Results

### Key Metrics to Look For

**Good Scheduler Performance:**
- Deadline misses = 0 (or very low)
- Tardiness rate < 5%
- Consistent latency (low max/avg ratio)
- All tasks making progress

**Poor Scheduler Performance:**
- Many deadline misses
- High tardiness rate
- Erratic latency (high variance)
- Task starvation

### Workload-Specific Goals

#### Workload 1 (Periodic Control)
✓ PASS: Zero deadline misses, avg latency < 100us
✗ FAIL: Any deadline misses by sensor thread

#### Workload 2 (Event-Driven)
✓ PASS: IRQ handler deadline misses = 0, latency < 100us
✗ FAIL: IRQ handler misses deadlines

#### Workload 3 (Mixed Criticality)
✓ PASS: Safety monitor deadline misses = 0
✗ FAIL: Safety monitor EVER misses a deadline (CRITICAL!)

#### Workload 4 (Deadline Sporadic)
✓ PASS: With EDF: deadline miss rate < 5%
✓ GOOD: Without EDF: deadline miss rate < 20%
✗ FAIL: > 50% deadline misses

## Comparing Schedulers

### Generate Comparison Report
```bash
cd results_<timestamp>
grep "Deadline Misses" *_run.log > comparison.txt
grep "Tardiness Rate" *_run.log >> comparison.txt
grep "Avg Latency" *_run.log >> comparison.txt
cat comparison.txt
```

### Expected Differences

**Simple vs Scalable:**
- Similar performance for <10 threads
- Scalable better with >20 threads
- Simple uses less code space

**Priority vs EDF (Workload 4):**
- EDF should have MUCH lower deadline miss rate
- EDF can handle higher utilization
- Priority-based may starve some tasks

**With vs Without Priority Inheritance (Workload 2):**
- Without: Higher latency spikes
- With: More predictable behavior

## Troubleshooting

### "west: command not found"
```bash
cd /home/jack/cs736-project/zephyr
source zephyr-env.sh
```

### Build fails with scheduler error
Check that scheduler combination is valid:
- DEADLINE requires SIMPLE
- MULTIQ incompatible with DEADLINE

### QEMU hangs at "Booting from ROM"
Wait 5-10 seconds, test may still be initializing.

### Results show all zeros
Test duration may be too short. Increase TEST_DURATION_MS in code.

## Next Steps

1. **Run baseline tests** - Use simple scheduler on all workloads
2. **Document results** - Save output from each configuration
3. **Compare schedulers** - Analyze metrics across schedulers
4. **Test with modifications** - Adjust periods, execution times, create overload
5. **Write up findings** - Document which scheduler works best for which workload

## File Structure
```
scheduler_evaluation/
├── README.md                          # Main documentation
├── SCHEDULER_CONFIGS.txt              # Configuration examples
├── build_all.sh                       # Automated test script
├── QUICK_START.md                     # This file
├── workload1_periodic_control/
│   ├── src/main.c
│   ├── prj.conf
│   ├── CMakeLists.txt
│   └── README.md
├── workload2_event_driven/
│   └── ...
├── workload3_mixed_criticality/
│   └── ...
└── workload4_deadline_sporadic/
    └── ...
```

## Getting Help

1. Check individual workload README files
2. Review main README.md for detailed documentation
3. Examine SCHEDULER_CONFIGS.txt for configuration examples
4. Look at source code comments in src/main.c files

## Success Criteria for Your Project

Your scheduler evaluation should demonstrate:

✓ Understanding of metrics (latency, tardiness, throughput)
✓ Ability to stress different scheduling algorithms
✓ Measurement of real-time performance
✓ Comparison across multiple scheduler implementations
✓ Analysis of scheduler behavior under various workloads

These workloads provide exactly that!

Good luck with your evaluation! 🚀
