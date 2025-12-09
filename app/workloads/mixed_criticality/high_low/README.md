# Mixed-Criticality Workload

## Overview

Tests how schedulers handle tasks with different importance levels. 75% total utilization split across three criticality tiers.

## Task Configuration

| Task | Period (ms) | Exec (ms) | Weight | Criticality | Utilization |
|------|-------------|-----------|--------|-------------|-------------|
| Task 1 | 100 | 20 | 10 | Critical | 20.0% |
| Task 2 | 200 | 20 | 10 | Critical | 10.0% |
| Task 3 | 150 | 25 | 5 | Important | 16.7% |
| Task 4 | 300 | 25 | 5 | Important | 8.3% |
| Task 5 | 250 | 30 | 1 | Best-Effort | 12.0% |
| Task 6 | 500 | 20 | 1 | Best-Effort | 4.0% |
| **Total** | - | - | - | - | **71.0%** |

## Research Questions

1. **Do weighted schedulers protect high-importance tasks?**
   - Weighted EDF, WSRT, PFS should prioritize critical tasks
   - EDF, RMS, LLF treat all equally (ignore weights)

2. **What happens if we increase load to >100%?**
   - Which tasks suffer most under each scheduler?
   - Do weights provide effective differentiation?

3. **Is fairness maintained across criticality levels?**
   - PFS should allocate CPU proportional to weights
   - Others may show bias

## Expected Results

### EDF (Weight-Agnostic)
- All tasks treated equally
- Misses distributed randomly if overload occurs
- No protection for critical tasks

### Weighted EDF
- Critical tasks (weight=10) protected
- Best-effort tasks (weight=1) more likely to miss under stress
- Effective deadline becomes deadline/weight

### WSRT
- Short critical tasks complete quickly
- Weight influences priority: higher weight = higher priority
- May starve long low-weight tasks

### RMS
- Ignores weights entirely
- Priority based solely on execution time
- Task 2 and Task 6 (both 20ms) have highest priority

### LLF
- Weight-agnostic
- Dynamic priority based on laxity
- May thrash under high load

### PFS
- Fair CPU allocation proportional to weights
- Critical tasks get 10x more CPU than best-effort
- Good for mixed workloads

## Metrics to Compare

1. **Deadline miss rate by criticality level**
   - Critical tasks should have lowest miss rate with weighted schedulers
2. **Response time distribution**
   - Are high-weight tasks faster?
3. **Fairness index**
   - Does PFS provide proportional fairness?

## Building

```bash
cd app/workloads/mixed_criticality/high_low
west build -b native_sim
west build -t run
```

Test different schedulers by editing `prj.conf`.

## Testing Under Overload

To test protection of critical tasks, modify a best-effort task to increase utilization:

```c
// In main.c, change Task 5 or 6:
.exec_time_ms = 80,  // Increased from 30
```

This pushes utilization over 100%. Observe which tasks suffer most under each scheduler.
