# Heavy Load Periodic Workload

## Overview

Approximately **90% CPU utilization** with 5 periodic tasks. Tests scheduler performance under high (but theoretically schedulable) load conditions.

## Task Configuration

| Task | Period (ms) | Execution Time (ms) | Utilization |
|------|-------------|---------------------|-------------|
| Task 1 | 50 | 15 | 30.0% |
| Task 2 | 100 | 25 | 25.0% |
| Task 3 | 200 | 40 | 20.0% |
| Task 4 | 400 | 70 | 17.5% |
| Task 5 | 500 | 80 | 16.0% |
| **Total** | - | - | **90.0%** |

## Expected Results

### EDF
- Should be schedulable (EDF is optimal up to 100%)
- May show occasional misses due to system overhead
- Moderate context switches

### Weighted EDF
- Similar to EDF (all weights = 1)
- Slight overhead from weight calculations

### WSRT
- Should complete short tasks faster
- May show higher context switches
- Possible fairness issues under stress

### RMS
- May have deadline misses (RMS bound ~69% for 5 tasks)
- Static priority: Task1 > Task2 > Task3 > Task4 > Task5

### LLF
- Should detect any issues early
- Risk of thrashing under high load
- Higher context switches expected

### PFS
- Fair degradation if misses occur
- Balanced response times across tasks

## Building

```bash
cd app/workloads/periodic/heavy_load
west build -b native_sim
west build -t run
```

Change scheduler in `prj.conf` and rebuild with `-p` flag.
