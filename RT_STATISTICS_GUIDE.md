# Real-Time Scheduler Statistics

## Overview

The RT Scheduler Statistics feature provides comprehensive, kernel-level tracking of real-time thread behavior and scheduler performance. This enables detailed workload analysis without requiring manual tracking in application code.

## Features

### Automatic Event Tracking

- **Activations**: Job releases/periodic task activations
- **Preemptions**: Times thread was preempted by higher priority thread
- **Context Switches**: Total switches in and out of CPU
- **Deadline Misses**: Number of deadline violations
- **Priority Inversions**: Times blocked by lower priority thread

### Timing Statistics

- **Response Time**: Time from activation to completion (min/max/total)
- **Waiting Time**: Time spent in ready queue before execution
- **Execution Time**: Actual CPU time consumed
- **Variance & Jitter**: Statistical measures of timing variability

### Configuration Levels

Three Kconfig options provide fine-grained control:

1. **CONFIG_736_RT_STATS** (Basic)
   - Event counters
   - Min/max/total timing
   - ~100 bytes per thread overhead

2. **CONFIG_736_RT_STATS_DETAILED** (+ Timestamps)
   - Activation, ready, start, completion timestamps
   - Enables fine-grained analysis
   - +32 bytes per thread

3. **CONFIG_736_RT_STATS_SQUARED** (+ Variance)
   - Squared sums for variance/jitter calculation
   - Standard deviation support
   - +16 bytes per thread

## Quick Start

### 1. Enable in Kconfig

```kconfig
CONFIG_736_RT_STATS=y
CONFIG_736_RT_STATS_DETAILED=y     # Optional
CONFIG_736_RT_STATS_SQUARED=y      # Optional
CONFIG_INSTRUMENT_THREAD_SWITCHING=y  # Required
```

### 2. Include Header

```c
#include <zephyr/kernel.h>  /* Automatically includes rt_stats.h */
```

### 3. Use in Application

```c
void periodic_task(void *arg1, void *arg2, void *arg3)
{
    k_tid_t self = k_current_get();
    
    /* Reset statistics at task start */
    k_thread_rt_stats_reset(self);
    
    while (1) {
        /* Wait for activation time */
        k_sleep(K_MSEC(period));
        
        /* Record activation */
        k_thread_rt_stats_activation(self);
        
        /* Do work */
        do_work();
        
        /* Check deadline */
        if (missed_deadline()) {
            k_thread_rt_stats_deadline_miss(self);
        }
        
        /* Read statistics */
        struct k_thread_rt_stats stats;
        k_thread_rt_stats_get(self, &stats);
        
        printk("Activations: %u, Misses: %u (%.1f%%)\n",
               stats.activations,
               stats.deadline_misses,
               k_thread_rt_stats_miss_ratio(&stats));
    }
}
```

## API Reference

### Core Functions

#### k_thread_rt_stats_get()

```c
int k_thread_rt_stats_get(k_tid_t thread, struct k_thread_rt_stats *stats);
```

Get comprehensive statistics for a thread.

**Parameters:**
- `thread`: Thread ID (NULL for current thread)
- `stats`: Pointer to structure to fill

**Returns:** 0 on success, -EINVAL if invalid, -ENOTSUP if disabled

**Example:**
```c
struct k_thread_rt_stats stats;
k_thread_rt_stats_get(NULL, &stats);  /* Get current thread stats */
printk("Preemptions: %u\n", stats.preemptions);
```

#### k_thread_rt_stats_reset()

```c
int k_thread_rt_stats_reset(k_tid_t thread);
```

Reset all statistics counters for a thread.

**Parameters:**
- `thread`: Thread ID (NULL for current thread)

**Returns:** 0 on success

**Example:**
```c
k_thread_rt_stats_reset(NULL);  /* Reset current thread */
```

#### k_thread_rt_stats_activation()

```c
void k_thread_rt_stats_activation(k_tid_t thread);
```

Record a thread activation/job release. Increments activation counter and updates timestamps.

**Parameters:**
- `thread`: Thread ID (NULL for current thread)

**Example:**
```c
k_thread_rt_stats_activation(NULL);  /* Record activation */
```

#### k_thread_rt_stats_deadline_miss()

```c
void k_thread_rt_stats_deadline_miss(k_tid_t thread);
```

Record a deadline miss. Increments deadline_misses counter.

**Parameters:**
- `thread`: Thread ID (NULL for current thread)

**Example:**
```c
if (k_uptime_get() > deadline) {
    k_thread_rt_stats_deadline_miss(NULL);
}
```

### Helper Functions

All helpers are static inline (zero overhead).

#### k_thread_rt_stats_avg_response()

```c
uint32_t k_thread_rt_stats_avg_response(const struct k_thread_rt_stats *stats);
```

Calculate average response time in milliseconds.

**Formula:** `total_response_time / activations`

#### k_thread_rt_stats_avg_waiting()

```c
uint32_t k_thread_rt_stats_avg_waiting(const struct k_thread_rt_stats *stats);
```

Calculate average waiting time in milliseconds.

#### k_thread_rt_stats_avg_exec()

```c
uint32_t k_thread_rt_stats_avg_exec(const struct k_thread_rt_stats *stats);
```

Calculate average execution time in milliseconds.

#### k_thread_rt_stats_miss_ratio()

```c
float k_thread_rt_stats_miss_ratio(const struct k_thread_rt_stats *stats);
```

Calculate deadline miss percentage (0.0 to 100.0).

**Formula:** `100.0 * deadline_misses / activations`

### Variance/Jitter Functions

Available when `CONFIG_736_RT_STATS_SQUARED=y`

#### k_thread_rt_stats_response_variance()

```c
uint64_t k_thread_rt_stats_response_variance(const struct k_thread_rt_stats *stats);
```

Calculate response time variance in ms².

**Formula:** `Var(X) = E[X²] - E[X]²`

#### k_thread_rt_stats_response_stddev()

```c
uint32_t k_thread_rt_stats_response_stddev(const struct k_thread_rt_stats *stats);
```

Calculate response time standard deviation in ms.

Uses integer square root approximation.

#### k_thread_rt_stats_response_jitter()

```c
uint32_t k_thread_rt_stats_response_jitter(const struct k_thread_rt_stats *stats);
```

Calculate response time jitter (range) in ms.

**Formula:** `max_response_time - min_response_time`

#### k_thread_rt_stats_waiting_variance()

```c
uint64_t k_thread_rt_stats_waiting_variance(const struct k_thread_rt_stats *stats);
```

Calculate waiting time variance in ms².

#### k_thread_rt_stats_waiting_stddev()

```c
uint32_t k_thread_rt_stats_waiting_stddev(const struct k_thread_rt_stats *stats);
```

Calculate waiting time standard deviation in ms.

### Detailed Timestamp Functions

Available when `CONFIG_736_RT_STATS_DETAILED=y`

#### k_thread_rt_stats_last_response()

```c
uint32_t k_thread_rt_stats_last_response(const struct k_thread_rt_stats *stats);
```

Get response time of last activation in ms.

**Formula:** `last_completion_time - last_activation_time`

#### k_thread_rt_stats_last_waiting()

```c
uint32_t k_thread_rt_stats_last_waiting(const struct k_thread_rt_stats *stats);
```

Get waiting time of last activation in ms.

**Formula:** `last_start_time - last_ready_time`

#### k_thread_rt_stats_last_exec()

```c
uint32_t k_thread_rt_stats_last_exec(const struct k_thread_rt_stats *stats);
```

Get execution time of last activation in ms.

**Formula:** `last_completion_time - last_start_time`

## Statistics Structure

```c
struct k_thread_rt_stats {
    /* Event counters */
    uint32_t activations;          /* Number of job releases */
    uint32_t preemptions;          /* Times preempted */
    uint32_t context_switches;     /* Total context switches */
    uint32_t deadline_misses;      /* Deadline violations */
    uint32_t priority_inversions;  /* Times blocked by lower priority */

    /* Timing statistics (ms) */
    uint64_t total_response_time;  /* Σ response times */
    uint64_t total_waiting_time;   /* Σ waiting times */
    uint64_t total_exec_time;      /* Σ execution times */
    uint32_t min_response_time;    /* Minimum response time */
    uint32_t max_response_time;    /* Maximum response time */
    uint32_t min_waiting_time;     /* Minimum waiting time */
    uint32_t max_waiting_time;     /* Maximum waiting time */

#ifdef CONFIG_736_RT_STATS_SQUARED
    uint64_t sum_response_time_sq; /* Σ(response²) */
    uint64_t sum_waiting_time_sq;  /* Σ(waiting²) */
#endif

#ifdef CONFIG_736_RT_STATS_DETAILED
    uint64_t last_activation_time;  /* Last job release */
    uint64_t last_ready_time;       /* Last ready queue entry */
    uint64_t last_start_time;       /* Last CPU dispatch */
    uint64_t last_completion_time;  /* Last completion */
#endif
};
```

## Automatic Tracking

The kernel automatically tracks events:

| Event | When Tracked | Function |
|-------|--------------|----------|
| **Activation** | Manual call | `k_thread_rt_stats_activation()` |
| **Deadline Miss** | Manual call | `k_thread_rt_stats_deadline_miss()` |
| **Context Switch** | Thread switched in | `z_thread_mark_switched_in()` |
| **Preemption** | Thread preempted | `update_cache()` |
| **Ready Time** | Added to ready queue | `ready_thread()` |
| **Start Time** | Dispatched to CPU | `z_thread_mark_switched_in()` |
| **Completion Time** | Switched out | `z_thread_mark_switched_out()` |

### What's Automatic vs Manual

**Automatic (zero application code):**
- Context switches
- Preemptions
- Timestamps (ready, start, completion)

**Manual (requires API calls):**
- Activations - call `k_thread_rt_stats_activation()`
- Deadline misses - call `k_thread_rt_stats_deadline_miss()`

**Why?** The kernel can't know when a "job" starts (application-specific concept), but it knows when threads switch.

## Performance Overhead

### Memory Overhead (per thread)

| Configuration | Bytes | Description |
|---------------|-------|-------------|
| Base CONFIG_736_RT_STATS | ~100 | Event counters + timing stats |
| + DETAILED | +32 | 4× uint64_t timestamps |
| + SQUARED | +16 | 2× uint64_t squared sums |
| **Total (all enabled)** | **~148** | Complete statistics |

### CPU Overhead

| Operation | Cycles (estimate) | When |
|-----------|-------------------|------|
| Activation tracking | ~20 | Per `k_thread_rt_stats_activation()` call |
| Deadline miss | ~10 | Per `k_thread_rt_stats_deadline_miss()` call |
| Context switch tracking | ~30 | Per context switch (automatic) |
| Preemption tracking | ~15 | Per preemption (automatic) |
| Get stats | ~200 | Per `k_thread_rt_stats_get()` call |

**Typical Impact:** < 1% for most workloads

## Use Cases

### 1. Workload Characterization

```c
/* Run workload, then analyze */
struct k_thread_rt_stats stats;
k_thread_rt_stats_get(task_tid, &stats);

printk("Task Characterization:\n");
printk("  Activations: %u\n", stats.activations);
printk("  Avg Response: %u ms\n", k_thread_rt_stats_avg_response(&stats));
printk("  Jitter: %u ms\n", k_thread_rt_stats_response_jitter(&stats));
printk("  CPU Util: %.1f%%\n", 
       100.0 * stats.total_exec_time / (stats.activations * period_ms));
```

### 2. Scheduler Comparison

```c
/* Test multiple schedulers */
for_each_scheduler() {
    reconfigure_scheduler();
    k_thread_rt_stats_reset(task_tid);
    
    run_workload();
    
    k_thread_rt_stats_get(task_tid, &stats);
    save_results(scheduler_name, &stats);
}

compare_schedulers();
```

### 3. Real-Time Validation

```c
/* Verify deadline compliance */
struct k_thread_rt_stats stats;
k_thread_rt_stats_get(task_tid, &stats);

if (stats.deadline_misses > 0) {
    printk("[FAILED]: %u deadline misses\n", stats.deadline_misses);
} else if (stats.max_response_time > deadline_ms) {
    printk("[WARNING]: Max response (%u) > deadline (%u)\n",
           stats.max_response_time, deadline_ms);
} else {
    printk("[PASSED]: All deadlines met\n");
}
```

### 4. Debugging Performance Issues

```c
struct k_thread_rt_stats stats;
k_thread_rt_stats_get(slow_task, &stats);

printk("Performance Analysis:\n");
printk("  Context Switches: %u\n", stats.context_switches);
printk("  Preemptions: %u\n", stats.preemptions);
printk("  Avg Waiting: %u ms\n", k_thread_rt_stats_avg_waiting(&stats));
printk("  Avg Exec: %u ms\n", k_thread_rt_stats_avg_exec(&stats));

if (stats.preemptions > stats.activations * 2) {
    printk("[WARNING]: High preemption rate - consider priority adjustment\n");
}
```

## Examples

See:
- `samples/rt_stats_example/` - Complete example with statistics
- `app/simple_eval_step1/src/main.c` - Manual tracking (comparison)

## Troubleshooting

### Stats are all zero

**Check:**
1. Is `CONFIG_736_RT_STATS=y` enabled?
2. Is `CONFIG_INSTRUMENT_THREAD_SWITCHING=y` enabled?
3. Did you call `k_thread_rt_stats_activation()` for activations?
4. Did you call `k_thread_rt_stats_deadline_miss()` for misses?

### Context switches seem too high

Context switches include both voluntary (sleep, wait) and involuntary (preemption). This is normal for RT systems.

### Timestamps are 0 with DETAILED enabled

Check that `CONFIG_736_RT_STATS_DETAILED=y` is actually set in your `prj.conf`.

### Variance calculation returns 0

Variance requires at least 2 samples. Check `stats.activations >= 2`.

## Best Practices

### 1. Reset at Task Start

```c
k_thread_rt_stats_reset(k_current_get());
```

Ensures clean baseline.

### 2. Record Activations Consistently

```c
/* At the START of each job */
k_thread_rt_stats_activation(NULL);
```

### 3. Use Helpers for Calculations

```c
/* Good: Uses helper */
float miss_rate = k_thread_rt_stats_miss_ratio(&stats);

/* Bad: Manual calculation (prone to division by zero) */
float miss_rate = 100.0 * stats.deadline_misses / stats.activations;
```

### 4. Check activations > 0 Before Using Stats

```c
if (stats.activations > 0) {
    uint32_t avg = k_thread_rt_stats_avg_response(&stats);
}
```

### 5. Export to CSV for Analysis

```c
printk("CSV,%u,%u,%u,%u\n",
       stats.activations,
       stats.deadline_misses,
       k_thread_rt_stats_avg_response(&stats),
       k_thread_rt_stats_response_stddev(&stats));
```

Then use Python/R for statistical analysis.

## Advanced Topics

### Combining with Manual Tracking

You can mix kernel stats with application-specific metrics:

```c
struct app_stats {
    uint32_t cache_misses;
    uint32_t packets_sent;
};

struct app_stats app_stats;
struct k_thread_rt_stats kernel_stats;

k_thread_rt_stats_get(NULL, &kernel_stats);

printk("Combined Analysis:\n");
printk("  Kernel: %u context switches\n", kernel_stats.context_switches);
printk("  App: %u cache misses\n", app_stats.cache_misses);
```

### Statistical Analysis

With `CONFIG_736_RT_STATS_SQUARED=y`:

```c
uint32_t avg = k_thread_rt_stats_avg_response(&stats);
uint32_t stddev = k_thread_rt_stats_response_stddev(&stats);

/* 95% confidence interval (assuming normal distribution) */
printk("Response time: %u ± %u ms (95%% CI)\n", avg, 2 * stddev);

/* Coefficient of variation */
float cv = (float)stddev / avg;
if (cv > 0.3) {
    printk("[WARNING]: High variability (CV = %.2f)\n", cv);
}
```

### Per-Scheduler Statistics

```c
struct scheduler_results {
    char name[32];
    struct k_thread_rt_stats stats;
};

struct scheduler_results results[6];

/* Run tests for each scheduler */
for (int i = 0; i < 6; i++) {
    configure_scheduler(schedulers[i]);
    k_thread_rt_stats_reset(task);
    
    run_workload();
    
    strcpy(results[i].name, schedulers[i]);
    k_thread_rt_stats_get(task, &results[i].stats);
}

/* Compare */
int best = find_lowest_miss_ratio(results);
printk("Best scheduler: %s\n", results[best].name);
```

## See Also

- [SCHEDULER_USER_GUIDE.md](SCHEDULER_USER_GUIDE.md) - RT scheduler usage
- [ALGORITHM_COMPARISON.md](ALGORITHM_COMPARISON.md) - Algorithm analysis
- `include/zephyr/kernel/rt_stats.h` - API header
- `kernel/sched.c` - Implementation

## Summary

RT Scheduler Statistics provides:
- Zero-overhead automatic tracking of scheduler events
- Comprehensive timing statistics
- Variance/jitter analysis
- Fine-grained timestamps
- Simple API
- Minimal memory footprint (~148 bytes/thread)

Enable it for any RT workload analysis or debugging!
