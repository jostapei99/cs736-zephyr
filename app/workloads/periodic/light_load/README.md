# Light Load Periodic Workload

## Overview

This workload provides approximately **50% CPU utilization** with 4 harmonic periodic tasks. It is designed to test basic scheduler functionality under comfortable, non-stressed conditions.

## Expected Behavior

All 6 scheduling algorithms should handle this workload **without any deadline misses**, as the utilization is well below 100% and the task set is schedulable.

## Task Configuration

| Task | Period (ms) | Execution Time (ms) | Deadline (ms) | Weight | Utilization |
|------|-------------|---------------------|---------------|--------|-------------|
| Task 1 | 100 | 20 | 100 | 1 | 20.0% |
| Task 2 | 200 | 30 | 200 | 1 | 15.0% |
| Task 3 | 400 | 40 | 400 | 1 | 10.0% |
| Task 4 | 800 | 60 | 800 | 1 | 7.5% |
| **Total** | - | - | - | - | **52.5%** |

**Note:** Periods are harmonic (each is a multiple of the previous), which generally makes scheduling easier.

## Building and Running

### With EDF (Baseline)
```bash
west build -b native_sim
west build -t run
```

### With Other Schedulers

Edit `prj.conf` and change the scheduler, then rebuild:

```kconfig
# For Weighted EDF
CONFIG_736_MOD_EDF=y

# For WSRT
CONFIG_736_WSRT=y
CONFIG_SCHED_THREAD_USAGE=y
CONFIG_THREAD_RUNTIME_STATS=y
CONFIG_736_TIME_LEFT=y

# For RMS
CONFIG_736_RMS=y

# For LLF
CONFIG_736_LLF=y
CONFIG_SCHED_THREAD_USAGE=y
CONFIG_THREAD_RUNTIME_STATS=y
CONFIG_736_TIME_LEFT=y

# For PFS
CONFIG_736_PFS=y
```

Then rebuild:
```bash
west build -b native_sim -p
west build -t run
```

## Expected Results

### All Schedulers
- **Deadline Misses:** 0 (0%)
- **Schedulability:** Yes (utilization << 100%)

### Performance Differences

While all should meet deadlines, they will differ in:

1. **Context Switches:**
   - **EDF/Weighted EDF/RMS:** Low (static or semi-static priority)
   - **WSRT/LLF:** Medium (dynamic priority based on progress)
   - **PFS:** Medium (fairness adjustments)

2. **Response Times:**
   - **WSRT:** Should have lowest average response time
   - **EDF:** Optimizes deadline adherence
   - **PFS:** Balances response times fairly

3. **Task Ordering:**
   - **EDF:** Tasks scheduled by absolute deadline
   - **Weighted EDF:** Same as EDF (all weights = 1)
   - **WSRT:** Short remaining work first
   - **RMS:** Task 1 > Task 2 > Task 3 > Task 4 (by execution time)
   - **LLF:** Dynamic based on laxity
   - **PFS:** Balances accumulated runtime

## Metrics to Compare

When running this workload with different schedulers, compare:

1. **Deadline Miss Rate** (should be 0% for all)
2. **Average Response Time** (WSRT should be best)
3. **Response Time Jitter** (lower is better)
4. **Context Switches** (overhead indicator)
5. **Min/Max Response Times** (predictability indicator)

## Output Format

CSV output is generated for easy analysis:
```csv
timestamp_ms,task_id,activation,response_ms,missed,preempted,scheduler
1000,1,1,20,0,0,EDF
1100,2,1,30,0,0,EDF
...
```

## Typical Results

### EDF
```
Total Activations: ~600
Deadline Misses: 0 (0.00%)
Avg Response Time: ~35 ms
Context Switches: ~400
```

### WSRT
```
Total Activations: ~600
Deadline Misses: 0 (0.00%)
Avg Response Time: ~25 ms  (lower than EDF)
Context Switches: ~450 (slightly higher)
```

### RMS
```
Total Activations: ~600
Deadline Misses: 0 (0.00%)
Avg Response Time: ~32 ms
Context Switches: ~380 (lowest - static priority)
```

## Research Questions

This workload helps answer:

1. **What is the baseline performance under low load?**
2. **How much overhead does dynamic scheduling add?**
3. **Does response time optimization (WSRT) make a difference?**
4. **Is static priority (RMS) sufficient for simple workloads?**

## Notes

- This workload uses `native_sim` for reproducibility
- Actual execution times may vary slightly due to system overhead
- Harmonic periods minimize hyperperiod, making analysis easier
- All weights are equal (1), so weight-based schedulers behave similarly to non-weighted versions
