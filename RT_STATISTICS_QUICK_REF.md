# RT Statistics Quick Reference

## Configuration

```kconfig
CONFIG_736_RT_STATS=y                    # Basic statistics
CONFIG_736_RT_STATS_DETAILED=y           # + Timestamps
CONFIG_736_RT_STATS_SQUARED=y            # + Variance/jitter
CONFIG_INSTRUMENT_THREAD_SWITCHING=y     # Required
```

## API Cheat Sheet

### Core Functions

| Function | Purpose | When to Call |
|----------|---------|--------------|
| `k_thread_rt_stats_get(tid, &stats)` | Get statistics | Anytime |
| `k_thread_rt_stats_reset(tid)` | Reset counters | Task start |
| `k_thread_rt_stats_activation(tid)` | Record activation | Job release |
| `k_thread_rt_stats_deadline_miss(tid)` | Record miss | Deadline violated |

**Note:** Use `NULL` for current thread.

### Helpers (inline, zero overhead)

| Function | Returns | Units |
|----------|---------|-------|
| `k_thread_rt_stats_avg_response(&stats)` | Average response time | ms |
| `k_thread_rt_stats_avg_waiting(&stats)` | Average waiting time | ms |
| `k_thread_rt_stats_avg_exec(&stats)` | Average execution time | ms |
| `k_thread_rt_stats_miss_ratio(&stats)` | Deadline miss % | 0.0-100.0 |
| `k_thread_rt_stats_response_jitter(&stats)` | Response jitter | ms |
| `k_thread_rt_stats_response_stddev(&stats)` | Std deviation | ms |
| `k_thread_rt_stats_response_variance(&stats)` | Variance | ms² |

## Quick Start

```c
#include <zephyr/kernel.h>

void periodic_task(void *arg1, void *arg2, void *arg3)
{
    k_thread_rt_stats_reset(NULL);  /* Reset at start */
    
    while (1) {
        k_sleep(K_MSEC(period));
        k_thread_rt_stats_activation(NULL);  /* Record activation */
        
        do_work();
        
        if (k_uptime_get() > deadline) {
            k_thread_rt_stats_deadline_miss(NULL);
        }
    }
}

void print_stats(void)
{
    struct k_thread_rt_stats stats;
    k_thread_rt_stats_get(task_tid, &stats);
    
    printk("Activations: %u, Misses: %u (%.1f%%)\n",
           stats.activations, stats.deadline_misses,
           k_thread_rt_stats_miss_ratio(&stats));
}
```

## Statistics Structure

```c
struct k_thread_rt_stats {
    /* Counters (always available) */
    uint32_t activations;        
    uint32_t preemptions;        
    uint32_t context_switches;   
    uint32_t deadline_misses;    
    uint32_t priority_inversions;

    /* Timing (always available) */
    uint64_t total_response_time;
    uint64_t total_waiting_time; 
    uint64_t total_exec_time;    
    uint32_t min_response_time;  
    uint32_t max_response_time;  
    uint32_t min_waiting_time;   
    uint32_t max_waiting_time;   

    /* Variance (SQUARED=y) */
    uint64_t sum_response_time_sq;
    uint64_t sum_waiting_time_sq; 

    /* Timestamps (DETAILED=y) */
    uint64_t last_activation_time;
    uint64_t last_ready_time;     
    uint64_t last_start_time;     
    uint64_t last_completion_time;
};
```

## What's Automatic?

| Event | Automatic? | How to Track |
|-------|------------|--------------|  
| Context switches | Yes | Read `stats.context_switches` |
| Preemptions | Yes | Read `stats.preemptions` |
| Ready/start/completion times | Yes | Read `stats.last_*_time` |
| Activations | No | Call `k_thread_rt_stats_activation()` |
| Deadline misses | No | Call `k_thread_rt_stats_deadline_miss()` |## Common Patterns

### Pattern 1: Basic Tracking

```c
k_thread_rt_stats_reset(NULL);
for (int i = 0; i < 100; i++) {
    k_thread_rt_stats_activation(NULL);
    do_job();
}
struct k_thread_rt_stats stats;
k_thread_rt_stats_get(NULL, &stats);
```

### Pattern 2: Deadline Checking

```c
uint64_t deadline = release_time + period_ms;
do_work();
if (k_uptime_get() > deadline) {
    k_thread_rt_stats_deadline_miss(NULL);
}
```

### Pattern 3: Periodic Reporting

```c
if (stats.activations % 10 == 0) {
    k_thread_rt_stats_get(NULL, &stats);
    printk("Avg response: %u ms\n", 
           k_thread_rt_stats_avg_response(&stats));
}
```

### Pattern 4: CSV Export

```c
printk("CSV,%u,%u,%u,%u\n",
       stats.activations,
       stats.deadline_misses,
       k_thread_rt_stats_avg_response(&stats),
       k_thread_rt_stats_response_jitter(&stats));
```

## Memory Overhead

| Config | Bytes/Thread | What's Included |
|--------|--------------|-----------------|
| Basic | ~100 | Counters + min/max/total |
| + DETAILED | +32 | 4× timestamps |
| + SQUARED | +16 | Variance sums |
| **Total** | **~148** | Everything |

## Performance Impact

- **Context switch:** ~30 cycles overhead
- **Activation:** ~20 cycles per call
- **Get stats:** ~200 cycles per call
- **Typical impact:** < 1%

## Examples

| Example | Location |
|---------|----------|
| Complete demo | `samples/rt_stats_example/` |
| Manual tracking | `app/simple_eval_step1/src/main.c` |

## Troubleshooting

| Problem | Solution |
|---------|----------|
| All zeros | Enable `CONFIG_736_RT_STATS` and `CONFIG_INSTRUMENT_THREAD_SWITCHING` |
| No activations | Call `k_thread_rt_stats_activation()` |
| No deadline misses tracked | Call `k_thread_rt_stats_deadline_miss()` |
| Timestamps zero | Enable `CONFIG_736_RT_STATS_DETAILED` |
| Can't calculate variance | Enable `CONFIG_736_RT_STATS_SQUARED` |

## See Full Guide

[RT_STATISTICS_GUIDE.md](RT_STATISTICS_GUIDE.md) - Comprehensive documentation
