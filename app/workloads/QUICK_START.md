# Workload Evaluation Quick Start Guide

This guide will help you quickly start evaluating the 6 RT scheduling algorithms using the workload suite.

## Step 1: Quick Test of a Single Workload (5 minutes)

Let's start with the light load workload and EDF scheduler:

```bash
cd app/workloads/periodic/light_load
west build -b native_sim
west build -t run
```

You should see output like:
```
================================================================================
RT Scheduler Workload Evaluation
Workload: Light Load (50% Utilization)
Scheduler: EDF
================================================================================

Theoretical CPU Utilization: 52.5%
Test Duration: 10000 ms

All tasks created, running for 10000 ms...

[CSV data appears here...]

Task Statistics:
  Task 1: 100 activations, 0 misses (0.00%), avg response: 20.5ms
  Task 2: 50 activations, 0 misses (0.00%), avg response: 30.2ms
  Task 3: 25 activations, 0 misses (0.00%), avg response: 40.1ms
  Task 4: 12 activations, 0 misses (0.00%), avg response: 60.3ms

Overall Summary:
  Total Activations: 187
  Deadline Misses: 0 (0.00%)
  Avg Response Time: 32.5 ms
  Response Time Jitter: 15.2 ms
================================================================================
```

## Step 2: Test Different Schedulers (10 minutes)

Edit `prj.conf` to try different schedulers:

### Test Weighted EDF

```bash
# Edit prj.conf and change to:
# CONFIG_736_MOD_EDF=y

west build -b native_sim -p
west build -t run
```

### Test WSRT (requires runtime stats)

```bash
# Edit prj.conf:
# CONFIG_736_WSRT=y
# CONFIG_SCHED_THREAD_USAGE=y
# CONFIG_THREAD_RUNTIME_STATS=y
# CONFIG_736_TIME_LEFT=y

west build -b native_sim -p
west build -t run
```

### Test All 6 Schedulers

Run each configuration and save the output:
1. EDF (baseline)
2. Weighted EDF
3. WSRT
4. RMS
5. LLF
6. PFS

## Step 3: Compare Interesting Workloads (15 minutes)

### Heavy Load - Test Under Stress

```bash
cd app/workloads/periodic/heavy_load
west build -b native_sim
west build -t run
```

This will show how schedulers perform at 90% utilization. Look for:
- Are there any deadline misses?
- How does response time change?
- Which scheduler has lowest jitter?

### Mixed-Criticality - Test Weight Handling

```bash
cd app/workloads/mixed_criticality/high_low
west build -b native_sim
west build -t run
```

This tests whether weighted schedulers actually protect high-importance tasks. Compare:
- **EDF** (ignores weights)
- **Weighted EDF** (should protect critical tasks)
- **PFS** (fair allocation by weight)

Key question: Do critical tasks (weight=10) perform better than best-effort tasks (weight=1)?

### Overload - Test Graceful Degradation

```bash
cd app/workloads/overload/sustained_overload
west build -b native_sim
west build -t run
```

This pushes utilization to 110%. ALL schedulers will miss deadlines. Look for:
- Which scheduler has the lowest overall miss rate?
- Which tasks suffer most under each scheduler?
- Do weighted schedulers protect high-weight tasks?

## Step 4: Automated Evaluation (30 minutes)

Run all workloads with all schedulers automatically:

```bash
cd app/workloads/scripts
./run_all_workloads.sh
```

This will:
1. Test all 4 workloads
2. With all 6 schedulers
3. Generate CSV data and logs
4. Create a summary in `results/summary.csv`

**Note:** This takes ~20-30 minutes to complete (24 test runs total)

## Step 5: Analyze Results (10 minutes)

After running the automated suite:

```bash
cd app/workloads/scripts
python3 compare_schedulers.py
```

This generates comparison reports showing:
- Best scheduler for each workload
- Performance across all metrics
- Strengths and weaknesses of each algorithm

## Understanding the Output

### CSV Data Format

```csv
timestamp_ms,task_id,activation,response_ms,missed,preempted,scheduler
1000,1,1,20,0,0,EDF
```

- `timestamp_ms`: When the task was activated
- `task_id`: Which task (1-N)
- `activation`: Activation count for this task
- `response_ms`: How long the task took (activation to completion)
- `missed`: 1 if deadline was missed, 0 otherwise
- `preempted`: 1 if task was preempted, 0 otherwise
- `scheduler`: Which scheduler was used

### Key Metrics

1. **Deadline Miss Rate**: Most important for hard RT systems
   - 0% = All deadlines met (good!)
   - >0% = Some deadlines missed (bad for hard RT)

2. **Average Response Time**: How quickly tasks complete on average
   - Lower is better
   - WSRT typically has lowest response time

3. **Response Time Jitter**: Variance in response times
   - Lower is better for predictability
   - RMS typically has low jitter (static priority)

4. **Context Switches**: How often tasks preempt each other
   - Lower = less overhead
   - Dynamic schedulers (LLF, WSRT) typically have more

## Typical Results Summary

### Light Load (50% utilization)
- **All schedulers**: 0% miss rate (schedulable)
- **WSRT**: Lowest avg response time
- **RMS**: Lowest context switches
- **EDF**: Good baseline

### Heavy Load (90% utilization)
- **EDF**: 0-5% miss rate (near optimal)
- **RMS**: 5-15% miss rate (below RMS bound)
- **Weighted EDF**: 0-5% miss rate
- **LLF**: May show higher context switches

### Mixed-Criticality (75% util, varied weights)
- **Weighted EDF**: Protects high-weight tasks
- **EDF**: Treats all equally (random misses)
- **PFS**: Fair degradation by weight
- **WSRT**: Fast response for short tasks

### Sustained Overload (110% utilization)
- **All schedulers**: Significant misses (inevitable)
- **Weighted EDF**: High-weight tasks protected
- **PFS**: Proportional degradation
- **EDF**: Random distribution of misses

## Research Questions You Can Answer

Using these workloads, you can investigate:

1. **What is the practical utilization bound for each scheduler?**
   - Test with increasing load (50% → 70% → 90% → 95%)

2. **How effective is weight-based prioritization?**
   - Compare weighted vs non-weighted schedulers on mixed-criticality workload

3. **Which scheduler minimizes response time?**
   - Compare WSRT vs others on light/heavy load

4. **How do schedulers degrade under overload?**
   - Compare overload workload results

5. **What is the context switch overhead?**
   - Count preemptions in CSV output

6. **Is RMS practical for dynamic workloads?**
   - Compare RMS vs EDF on various workloads

## Next Steps

### Modify Workloads

Edit task configurations in `src/main.c` to create custom scenarios:

```c
// Increase Task 1's execution time
{
    .name = "Task1",
    .period_ms = 100,
    .exec_time_ms = 50,  // Changed from 20
    .deadline_ms = 0,
    .weight = 1,
    ...
}
```

### Add New Workloads

1. Copy an existing workload directory
2. Modify task configurations
3. Update README with expected behavior
4. Add to `run_all_workloads.sh`

### Test on Real Hardware

```bash
# Instead of native_sim, use a real board:
west build -b nucleo_f411re
west flash
```

### Collect Detailed Statistics

Enable detailed kernel statistics in `prj.conf`:

```kconfig
CONFIG_736_RT_STATS_DETAILED=y
CONFIG_736_RT_STATS_SQUARED=y
```

Then use the RT statistics API to get variance, jitter, etc.

## Troubleshooting

### Build Errors

```bash
# Clean and rebuild
west build -b native_sim -p
```

### WSRT/LLF Not Working

Make sure runtime tracking is enabled:
```kconfig
CONFIG_SCHED_THREAD_USAGE=y
CONFIG_THREAD_RUNTIME_STATS=y
CONFIG_736_TIME_LEFT=y
```

### Native Sim Timing Issues

`native_sim` timing may not be perfectly accurate. For critical measurements, test on real hardware.

### No CSV Output

Check that `CSV_OUTPUT` is enabled (should be by default in `workload_common.h`).

## Getting Help

- See `app/workloads/README.md` for detailed documentation
- Check individual workload READMEs for expected results
- Review `SCHEDULER_USER_GUIDE.md` for scheduler details
- See `ALGORITHM_COMPARISON.md` for algorithm comparison

## Summary

You now have:
- 4 different workload types testing various scenarios
- 6 scheduling algorithms to compare
- Automated tools to run comprehensive evaluations
- Scripts to analyze and compare results

Start simple (light load + EDF), then progressively test more complex scenarios and different schedulers!
