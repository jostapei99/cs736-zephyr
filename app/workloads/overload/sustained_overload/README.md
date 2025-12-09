# Sustained Overload Workload

## Overview

**110% CPU utilization** - continuous overload to test graceful degradation.

## Task Configuration

| Task | Period (ms) | Exec (ms) | Weight | Utilization |
|------|-------------|-----------|--------|-------------|
| Task 1 | 100 | 35 | 2 | 35.0% |
| Task 2 | 150 | 45 | 1 | 30.0% |
| Task 3 | 200 | 50 | 3 | 25.0% |
| Task 4 | 300 | 60 | 1 | 20.0% |
| Task 5 | 400 | 70 | 1 | 17.5% |
| **Total** | - | - | - | **110.0%** |

## Expected Behavior

### All Schedulers
- Deadline misses are **inevitable** (>100% utilization)
- Observe **which tasks** suffer most
- Measure **degradation pattern**

### Weight-Aware Schedulers

**Weighted EDF:**
- Should protect Task 3 (weight=3) and Task 1 (weight=2)
- Tasks 2, 4, 5 (weight=1) more likely to miss

**WSRT:**
- Protects short, high-weight tasks
- May starve Task 5 (long execution, low weight)

**PFS:**
- Proportional degradation based on weights
- Fair distribution of misses

### Weight-Agnostic Schedulers

**EDF:**
- Random distribution of misses
- No protection for any task

**RMS:**
- Low-period tasks protected (Task 1, 2)
- Long-period tasks suffer (Task 4, 5)

**LLF:**
- May exhibit thrashing behavior
- Frequent priority changes

## Research Questions

1. **Do weighted schedulers effectively protect high-importance tasks?**
2. **Which scheduler has the most graceful degradation?**
3. **Does LLF thrash under sustained overload?**
4. **How does context switch count correlate with overload handling?**

## Building

```bash
cd app/workloads/overload/sustained_overload
west build -b native_sim
west build -t run
```

## Typical Results

### Weighted EDF
- Task 3 (weight=3): ~5% misses
- Task 1 (weight=2): ~10% misses
- Tasks 2,4,5 (weight=1): ~25-40% misses

### EDF
- All tasks: ~15-25% misses (random distribution)

### PFS
- Misses proportional to inverse weights
- Balanced degradation
