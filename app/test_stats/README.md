# RT Statistics Test Application

This application tests the real-time statistics collection framework for custom schedulers.

## Overview

Tests RT statistics APIs across all 5 custom scheduling algorithms:
- Weighted EDF
- Rate Monotonic Scheduling (RMS)
- Weighted Shortest Remaining Time (WSRT)
- Least Laxity First (LLF)
- Proportional Fair Scheduling (PFS)

## Features Tested

1. **Basic Statistics Collection**
   - Activations
   - Completions
   - Preemptions
   - Context switches
   - Deadline misses

2. **Timing Metrics**
   - Response time (min/max/average)
   - Waiting time (min/max/average)
   - Execution time tracking

3. **Advanced Statistics** (with CONFIG_736_RT_STATS_DETAILED)
   - Timestamps for all events
   - Variance/jitter calculation (with CONFIG_736_RT_STATS_SQUARED)

4. **Multi-Scheduler Comparison**
   - Run same workload on different schedulers
   - Compare performance metrics
   - Validate statistical accuracy

## Build Instructions

### Basic Statistics Test
```bash
cd /home/jack/cs736-project/zephyr
west build -b native_sim app/test_stats -p
west build -t run
```

### With Detailed Statistics
```bash
west build -b native_sim app/test_stats -p -- \
  -DCONFIG_736_RT_STATS_DETAILED=y
west build -t run
```

### With Variance Calculation
```bash
west build -b native_sim app/test_stats -p -- \
  -DCONFIG_736_RT_STATS_DETAILED=y \
  -DCONFIG_736_RT_STATS_SQUARED=y
west build -t run
```

### Test Specific Scheduler
```bash
# Test Weighted EDF
west build -b native_sim app/test_stats -p -- \
  -DCONFIG_736_MOD_EDF=y

# Test RMS
west build -b native_sim app/test_stats -p -- \
  -DCONFIG_736_RMS=y

# Test WSRT
west build -b native_sim app/test_stats -p -- \
  -DCONFIG_736_WSRT=y

# Test LLF
west build -b native_sim app/test_stats -p -- \
  -DCONFIG_736_LLF=y

# Test PFS
west build -b native_sim app/test_stats -p -- \
  -DCONFIG_736_PFS=y
```

## Test Cases

### Test 1: Basic Statistics Collection
- Creates 3 periodic threads with different parameters
- Validates activation counting
- Checks response time tracking
- Verifies waiting time measurement

### Test 2: Deadline Miss Detection
- Creates threads with tight deadlines
- Intentionally causes deadline misses
- Verifies miss counting accuracy

### Test 3: Preemption Tracking
- Tests preemption counting
- Validates context switch statistics
- Checks priority inversion detection (if applicable)

### Test 4: Statistical Accuracy
- Runs many iterations
- Validates min/max/average calculations
- Tests variance computation (if enabled)

### Test 5: Multi-Scheduler Comparison
- Runs same workload on each scheduler
- Compares statistics across algorithms
- Generates performance comparison data

## Expected Output

```
=== RT Statistics Test Suite ===

[Test 1] Basic Statistics Collection
  Thread A: activations=10, response_time_avg=5.2ms
  Thread B: activations=10, response_time_avg=8.7ms
  Thread C: activations=10, response_time_avg=12.1ms
  ✓ PASS

[Test 2] Deadline Miss Detection
  Thread A: deadline_misses=0
  Thread B: deadline_misses=3
  Thread C: deadline_misses=7
  ✓ PASS

[Test 3] Preemption Tracking
  Thread A: preemptions=15
  Thread B: preemptions=8
  Thread C: preemptions=2
  ✓ PASS

[Test 4] Statistical Accuracy
  Response time variance: 2.34ms²
  Waiting time jitter: 1.52ms
  ✓ PASS

[Test 5] Multi-Scheduler Comparison
  Weighted EDF: avg_response=8.3ms, misses=0
  RMS:          avg_response=9.1ms, misses=0
  WSRT:         avg_response=7.8ms, misses=0
  LLF:          avg_response=8.9ms, misses=1
  PFS:          avg_response=10.2ms, misses=0
  ✓ PASS

All tests passed!
```

## Configuration Options

| Option | Description | Default |
|--------|-------------|---------|
| `CONFIG_736_RT_STATS` | Enable RT statistics | Required |
| `CONFIG_736_RT_STATS_DETAILED` | Track timestamps | Optional |
| `CONFIG_736_RT_STATS_SQUARED` | Variance calculation | Optional |
| `NUM_TEST_ITERATIONS` | Number of test iterations | 10 |
| `TEST_WORKLOAD_SIZE` | Simulated work per job | 1000 |

## Statistics API Reference

### Get Statistics
```c
struct k_thread_rt_stats stats;
k_thread_rt_stats_get(thread_id, &stats);
printk("Activations: %u\n", stats.activations);
printk("Response time: %llu ms\n", stats.total_response_time / stats.activations);
```

### Reset Statistics
```c
k_thread_rt_stats_reset(thread_id);
```

### Mark Activation
```c
// Call at start of each periodic job
k_thread_rt_stats_activation(NULL);  // NULL = current thread
```

### Record Deadline Miss
```c
if (k_uptime_get() > my_deadline) {
    k_thread_rt_stats_deadline_miss(NULL);
}
```

## Troubleshooting

### No Statistics Collected
- Ensure `CONFIG_736_RT_STATS=y` in prj.conf
- Verify scheduler is one of the custom algorithms
- Check that `CONFIG_736_ADD_ONS=y`

### Zero Response Times
- Enable `CONFIG_736_RT_STATS_DETAILED=y`
- Call `k_thread_rt_stats_activation()` at job start
- Ensure threads are actually scheduled

### Incorrect Deadline Miss Count
- Verify deadline checking logic
- Call `k_thread_rt_stats_deadline_miss()` when miss detected
- Check clock synchronization

## Related Documentation

- [SCHEDULING_ALGORITHMS.md](../../docs_cs736_project/SCHEDULING_ALGORITHMS.md) - Algorithm descriptions
- [RT_STATISTICS_QUICK_REF.md](../../RT_STATISTICS_QUICK_REF.md) - API quick reference
- [app/testing/README.md](../testing/README.md) - Scheduler test applications
