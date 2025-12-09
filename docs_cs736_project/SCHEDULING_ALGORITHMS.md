# CS736 Real-Time Scheduling Algorithms for Zephyr RTOS

## Table of Contents
1. [Overview](#overview)
2. [Base EDF Scheduler](#base-edf-scheduler)
3. [Custom Scheduling Algorithms](#custom-scheduling-algorithms)
   - [Weighted EDF](#weighted-edf)
   - [Rate Monotonic Scheduling (RMS)](#rate-monotonic-scheduling-rms)
   - [Weighted Shortest Remaining Time (WSRT)](#weighted-shortest-remaining-time-wsrt)
   - [Least Laxity First (LLF)](#least-laxity-first-llf)
   - [Proportional Fair Scheduling (PFS)](#proportional-fair-scheduling-pfs)
4. [API Reference](#api-reference)
5. [Configuration](#configuration)
6. [Examples](#examples)

---

## Overview

This document describes the real-time scheduling algorithms implemented for Zephyr RTOS as part of the CS736 project. The implementation includes the base Zephyr EDF scheduler plus five custom scheduling policies designed for different real-time workload characteristics.

### Scheduling Architecture

All schedulers operate at the same priority level within Zephyr's priority-based scheduling system. The scheduler selection determines which thread runs when multiple threads are ready at the same priority.

**Key Features:**
- Priority-aware: Higher priority threads always preempt lower priority
- Configurable: Select scheduler via Kconfig at compile time
- Extensible: Custom comparison functions for each algorithm
- API-driven: User-space syscalls to configure thread parameters

---

## Base EDF Scheduler

### Earliest Deadline First (CONFIG_SCHED_DEADLINE)

**Algorithm:** Standard EDF scheduling based on absolute deadlines.

**Scheduling Rule:**
```
Earlier deadline → Higher priority
```

**How It Works:**
- Each thread has an absolute deadline (in ticks/cycles)
- Threads are ordered by deadline in the ready queue
- Thread with earliest deadline executes first
- Optimal for single-processor systems (Liu & Layland, 1973)

**Advantages:**
- ✓ Optimal schedulability: Can schedule any feasible task set
- ✓ Simple and well-understood
- ✓ No need for offline analysis
- ✓ Built into Zephyr kernel

**Disadvantages:**
- ✗ All tasks treated equally (no priority differentiation)
- ✗ Can have high context switch overhead
- ✗ May miss deadlines when overloaded

**Use Cases:**
- Hard real-time systems with homogeneous tasks
- Systems where all deadlines are equally important
- Baseline scheduler for performance comparison

**Configuration:**
```c
CONFIG_SCHED_DEADLINE=y
```

**API:**
```c
void k_thread_deadline_set(k_tid_t thread, int deadline);
```

**Example:**
```c
k_tid_t tid = k_thread_create(&thread_data, stack,
                              K_THREAD_STACK_SIZEOF(stack),
                              thread_entry, NULL, NULL, NULL,
                              PRIORITY, 0, K_NO_WAIT);

/* Set absolute deadline in ticks */
k_thread_deadline_set(tid, 1000);
```

---

## Custom Scheduling Algorithms

### Weighted EDF

**Configuration:** `CONFIG_736_MOD_EDF`

**Algorithm:** Modified EDF that schedules based on deadline/weight ratio.

**Scheduling Rule:**
```
Thread priority ∝ 1 / (deadline / weight)

Lower deadline/weight ratio → Higher priority
```

**How It Works:**
- Each thread has a deadline and a weight
- Weight represents task importance/priority
- Effective deadline = deadline / weight
- Thread with lowest ratio executes first

**Mathematical Model:**
```
For threads A and B:
  A runs first if: deadline_A / weight_A < deadline_B / weight_B
```

**Comparison Function:**
```c
int32_t z_sched_cmp_weighted_edf(struct k_thread *t1, struct k_thread *t2)
{
    uint32_t d1 = t1->base.prio_deadline;
    uint32_t d2 = t2->base.prio_deadline;
    int w1 = t1->base.prio_weight;
    int w2 = t2->base.prio_weight;
    
    if (w1 == 0) w1 = 1;
    if (w2 == 0) w2 = 1;
    
    return (d2 / w2) - (d1 / w1);
}
```

**Advantages:**
- ✓ Differentiates task importance
- ✓ Higher weight tasks get more CPU time
- ✓ Compatible with traditional EDF analysis
- ✓ Graceful degradation under overload

**Disadvantages:**
- ✗ Not strictly optimal like pure EDF
- ✗ Requires weight tuning
- ✗ Integer division may lose precision

**Use Cases:**
- Mixed-criticality systems
- Systems with high and low importance tasks
- Applications requiring importance-aware scheduling
- Soft real-time systems with QoS requirements

**Configuration:**
```c
CONFIG_736_MOD_EDF=y
```

**API:**
```c
k_thread_deadline_set(tid, deadline);
k_thread_weight_set(tid, weight);  // Default: 1
```

**Example:**
```c
/* Critical task: weight=10 */
k_thread_deadline_set(critical_tid, 1000);
k_thread_weight_set(critical_tid, 10);
/* Effective deadline: 1000/10 = 100 */

/* Normal task: weight=5 */
k_thread_deadline_set(normal_tid, 500);
k_thread_weight_set(normal_tid, 5);
/* Effective deadline: 500/5 = 100 */

/* Result: Both have same effective deadline, scheduled FIFO */
```

**Test Application:** `app/testing/test_weighted_edf/`

---

### Rate Monotonic Scheduling (RMS)

**Configuration:** `CONFIG_736_RMS`

**Algorithm:** Priority assignment based on execution time (as proxy for period).

**Scheduling Rule:**
```
Shorter execution time → Higher priority
```

**How It Works:**
- Each thread has an expected execution time
- Threads are ordered by execution time
- Shorter execution time = higher priority
- Simplified RMS using exec_time instead of period

**Mathematical Model:**
```
For threads A and B:
  A runs first if: exec_time_A < exec_time_B
```

**Comparison Function:**
```c
int32_t z_sched_cmp_rms(struct k_thread *t1, struct k_thread *t2)
{
    uint32_t et1 = t1->base.prio_exec_time;
    uint32_t et2 = t2->base.prio_exec_time;
    
    return et2 - et1;  // Shorter exec_time first
}
```

**Advantages:**
- ✓ Static priority assignment
- ✓ Predictable behavior
- ✓ Well-studied schedulability analysis
- ✓ Low runtime overhead

**Disadvantages:**
- ✗ Not optimal for non-periodic tasks
- ✗ Requires offline configuration
- ✗ Can have lower utilization than EDF
- ✗ Uses exec_time as proxy for period

**Use Cases:**
- Periodic real-time tasks
- Systems with fixed task sets
- Applications requiring static priorities
- Control systems with regular sampling

**Schedulability Test:**
```
For n periodic tasks with periods T_i and execution times C_i:
  Utilization U = Σ(C_i / T_i) ≤ n(2^(1/n) - 1)
  
  For n=3: U ≤ 0.779
  For large n: U → ln(2) ≈ 0.693
```

**Configuration:**
```c
CONFIG_736_RMS=y
```

**API:**
```c
k_thread_exec_time_set(tid, exec_time);
```

**Example:**
```c
/* Fast periodic task: 10ms execution */
k_thread_exec_time_set(fast_tid, 10);

/* Slow periodic task: 50ms execution */
k_thread_exec_time_set(slow_tid, 50);

/* Fast task will always run first */
```

**Test Application:** `app/testing/test_rms/`

---

### Weighted Shortest Remaining Time (WSRT)

**Configuration:** `CONFIG_736_WSRT`

**Algorithm:** Dynamic scheduling based on weighted remaining execution time.

**Scheduling Rule:**
```
Thread priority ∝ 1 / (time_left / weight)

Lower time_left/weight ratio → Higher priority
```

**How It Works:**
- Each thread has remaining time and weight
- time_left represents work still to be done
- Thread with least weighted remaining time runs first
- Dynamic - priorities change as work completes

**Mathematical Model:**
```
For threads A and B:
  A runs first if: time_left_A / weight_A < time_left_B / weight_B
```

**Comparison Function:**
```c
int32_t z_sched_cmp_wsrt(struct k_thread *t1, struct k_thread *t2)
{
    uint32_t w1 = t1->base.prio_weight;
    uint32_t w2 = t2->base.prio_weight;
    uint32_t tl1 = t1->base.prio_time_left;
    uint32_t tl2 = t2->base.prio_time_left;
    
    if (w1 == 0) w1 = 1;
    if (w2 == 0) w2 = 1;
    
    return (tl2 / w2) - (tl1 / w1);
}
```

**Advantages:**
- ✓ Minimizes average response time
- ✓ Adaptive to actual workload
- ✓ Weight-aware for importance
- ✓ Good for bursty workloads

**Disadvantages:**
- ✗ Requires runtime tracking
- ✗ Can starve long jobs
- ✗ High context switch overhead
- ✗ Needs accurate time_left estimates

**Use Cases:**
- Batch processing systems
- Job scheduling with priorities
- Systems optimizing for response time
- Variable execution time tasks

**Configuration:**
```c
CONFIG_736_WSRT=y
```

**API:**
```c
k_thread_time_left_set(tid, remaining_time);
k_thread_weight_set(tid, weight);
```

**Example:**
```c
/* Update remaining time as work progresses */
void periodic_task(void)
{
    uint32_t work_remaining = 100;
    k_thread_time_left_set(NULL, work_remaining);
    k_thread_weight_set(NULL, 10);
    
    while (work_remaining > 0) {
        do_work();
        work_remaining -= 10;
        k_thread_time_left_set(NULL, work_remaining);
    }
}
```

**Test Application:** `app/testing/test_wsrt/`

---

### Least Laxity First (LLF)

**Configuration:** `CONFIG_736_LLF`

**Algorithm:** Dynamic priority based on slack time before deadline.

**Scheduling Rule:**
```
Laxity = deadline - time_left
Lower laxity → Higher priority (more urgent)
```

**How It Works:**
- Laxity is the slack time before missing deadline
- Thread must complete time_left work before deadline
- Lower laxity means less time to spare
- Most urgent tasks (least slack) run first

**Mathematical Model:**
```
For threads A and B:
  laxity_A = deadline_A - time_left_A
  laxity_B = deadline_B - time_left_B
  
  A runs first if: laxity_A < laxity_B
```

**Comparison Function:**
```c
int32_t z_sched_cmp_llf(struct k_thread *t1, struct k_thread *t2)
{
    uint32_t d1 = t1->base.prio_deadline;
    uint32_t d2 = t2->base.prio_deadline;
    uint32_t tl1 = t1->base.prio_time_left;
    uint32_t tl2 = t2->base.prio_time_left;
    
    int32_t laxity1 = (int32_t)(d1 - tl1);
    int32_t laxity2 = (int32_t)(d2 - tl2);
    
    return laxity2 - laxity1;  // Lower laxity first
}
```

**Advantages:**
- ✓ Optimal like EDF
- ✓ Detects deadline misses early
- ✓ Responds to execution variations
- ✓ Better than EDF for overloaded systems

**Disadvantages:**
- ✗ Very high context switch rate
- ✗ Requires accurate time_left tracking
- ✗ Thrashing when overloaded
- ✗ Complex runtime overhead

**Use Cases:**
- Hard real-time systems
- Early deadline miss detection
- Systems with variable execution times
- Safety-critical applications

**Laxity States:**
```
Laxity > 0  : Task has slack time
Laxity = 0  : Must run continuously to meet deadline
Laxity < 0  : Deadline will be missed
```

**Configuration:**
```c
CONFIG_736_LLF=y
```

**API:**
```c
k_thread_deadline_set(tid, deadline);
k_thread_time_left_set(tid, remaining_time);
```

**Example:**
```c
/* Task with tight deadline */
k_thread_deadline_set(urgent_tid, 100);
k_thread_time_left_set(urgent_tid, 95);
/* Laxity = 100 - 95 = 5 (very urgent!) */

/* Task with slack */
k_thread_deadline_set(slack_tid, 300);
k_thread_time_left_set(slack_tid, 200);
/* Laxity = 300 - 200 = 100 (can wait) */
```

**Test Application:** `app/testing/test_llf/`

---

### Proportional Fair Scheduling (PFS)

**Configuration:** `CONFIG_736_PFS`

**Algorithm:** Fair scheduling based on virtual runtime.

**Scheduling Rule:**
```
Virtual runtime = runtime / weight
Lower virtual_runtime → Higher priority (less CPU received)
```

**How It Works:**
- Tracks accumulated runtime for each thread
- Weight controls share of CPU time
- Thread with least virtual_runtime runs next
- Ensures fairness proportional to weights

**Mathematical Model:**
```
For threads A and B:
  virtual_runtime_A = exec_time_A / weight_A
  virtual_runtime_B = exec_time_B / weight_B
  
  A runs first if: virtual_runtime_A < virtual_runtime_B
```

**Comparison Function:**
```c
int32_t z_sched_cmp_pfs(struct k_thread *t1, struct k_thread *t2)
{
    uint32_t runtime1 = t1->base.prio_exec_time;
    uint32_t runtime2 = t2->base.prio_exec_time;
    int w1 = t1->base.prio_weight;
    int w2 = t2->base.prio_weight;
    
    if (w1 == 0) w1 = 1;
    if (w2 == 0) w2 = 1;
    
    return (runtime1 / w1) - (runtime2 / w2);
}
```

**Advantages:**
- ✓ Perfect fairness guarantee
- ✓ Prevents starvation
- ✓ Weight-based CPU sharing
- ✓ Good for mixed workloads

**Disadvantages:**
- ✗ Not deadline-aware
- ✗ Not suitable for hard real-time
- ✗ Requires runtime tracking
- ✗ Overhead from frequent updates

**Use Cases:**
- Mixed real-time and best-effort workloads
- Server applications
- Multi-tenant systems
- Preventing task starvation

**Fairness Property:**
```
Over time interval T, thread with weight W receives:
  CPU_time = T × (W / Σ(all_weights))
```

**Configuration:**
```c
CONFIG_736_PFS=y
```

**API:**
```c
k_thread_exec_time_set(tid, accumulated_runtime);
k_thread_weight_set(tid, weight);
```

**Example:**
```c
/* High priority thread: weight=400 */
k_thread_weight_set(high_tid, 400);
k_thread_exec_time_set(high_tid, 100);
/* Virtual runtime = 100/400 = 0.25 */

/* Low priority thread: weight=100 */
k_thread_weight_set(low_tid, 100);
k_thread_exec_time_set(low_tid, 100);
/* Virtual runtime = 100/100 = 1.0 */

/* High priority runs first (lower virtual_runtime) */
/* Over time, high_tid gets 4x more CPU than low_tid */
```

**Test Application:** `app/testing/test_pfs/`

---

## API Reference

### Thread Configuration Syscalls

#### Weight Management
```c
/**
 * @brief Set thread weight for weighted scheduling algorithms
 * @param tid Thread ID (NULL for current thread)
 * @param weight Thread weight (higher = more important)
 * @return 0 on success
 */
int k_thread_weight_set(k_tid_t tid, uint32_t weight);

/**
 * @brief Get thread weight
 * @param tid Thread ID (NULL for current thread)
 * @param weight Pointer to store weight value
 * @return 0 on success
 */
int k_thread_weight_get(k_tid_t tid, uint32_t *weight);
```

#### Execution Time Management
```c
/**
 * @brief Set thread execution time (for RMS, PFS)
 * @param tid Thread ID (NULL for current thread)
 * @param exec_time Expected or accumulated execution time
 * @return 0 on success
 */
int k_thread_exec_time_set(k_tid_t tid, uint32_t exec_time);

/**
 * @brief Get thread execution time
 * @param tid Thread ID (NULL for current thread)
 * @param exec_time Pointer to store execution time
 * @return 0 on success
 */
int k_thread_exec_time_get(k_tid_t tid, uint32_t *exec_time);
```

#### Remaining Time Management
```c
/**
 * @brief Set thread remaining execution time (for WSRT, LLF)
 * @param tid Thread ID (NULL for current thread)
 * @param time_left Remaining execution time
 * @return 0 on success
 */
int k_thread_time_left_set(k_tid_t tid, uint32_t time_left);

/**
 * @brief Get thread remaining execution time
 * @param tid Thread ID (NULL for current thread)
 * @param time_left Pointer to store remaining time
 * @return 0 on success
 */
int k_thread_time_left_get(k_tid_t tid, uint32_t *time_left);
```

#### Deadline Management (Standard Zephyr)
```c
/**
 * @brief Set thread absolute deadline
 * @param tid Thread ID
 * @param deadline Absolute deadline in ticks
 */
void k_thread_deadline_set(k_tid_t tid, int deadline);
```

### Thread Structure Fields

```c
struct _thread_base {
    // ... other fields ...
    
#ifdef CONFIG_SCHED_DEADLINE
    int prio_deadline;          /* Absolute deadline (ticks) */
#endif

#ifdef CONFIG_736_ADD_ONS
    int prio_weight;            /* Thread weight/importance */
    uint32_t prio_exec_time;    /* Expected/accumulated exec time */
    uint32_t prio_time_left;    /* Remaining execution time */
#endif
};
```

---

## Configuration

### Kconfig Options

Enable one scheduler at a time in your `prj.conf`:

```bash
# Base EDF (always available with CONFIG_SCHED_DEADLINE)
CONFIG_SCHED_DEADLINE=y

# Weighted EDF
CONFIG_736_MOD_EDF=y

# Rate Monotonic Scheduling
CONFIG_736_RMS=y

# Weighted Shortest Remaining Time
CONFIG_736_WSRT=y

# Least Laxity First
CONFIG_736_LLF=y

# Proportional Fair Scheduling
CONFIG_736_PFS=y
```

### Build Configuration

Add to your application's `CMakeLists.txt`:
```cmake
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(my_rt_app)

target_sources(app PRIVATE src/main.c)
```

---

## Examples

### Example 1: Weighted EDF for Mixed-Criticality

```c
#include <zephyr/kernel.h>
#include <zephyr/kernel/sched_rt.h>

void critical_task(void *p1, void *p2, void *p3)
{
    while (1) {
        /* Critical work */
        k_sleep(K_MSEC(100));
    }
}

void normal_task(void *p1, void *p2, void *p3)
{
    while (1) {
        /* Normal work */
        k_sleep(K_MSEC(100));
    }
}

int main(void)
{
    k_tid_t critical_tid, normal_tid;
    
    critical_tid = k_thread_create(...);
    normal_tid = k_thread_create(...);
    
    /* Critical task: deadline=1000, weight=10 */
    k_thread_deadline_set(critical_tid, 1000);
    k_thread_weight_set(critical_tid, 10);
    
    /* Normal task: deadline=2000, weight=5 */
    k_thread_deadline_set(normal_tid, 2000);
    k_thread_weight_set(normal_tid, 5);
    
    return 0;
}
```

### Example 2: LLF with Dynamic Updates

```c
void sensor_task(void *p1, void *p2, void *p3)
{
    uint32_t work_remaining = 100;
    int deadline = 1000;
    
    k_thread_deadline_set(NULL, deadline);
    k_thread_time_left_set(NULL, work_remaining);
    
    while (work_remaining > 0) {
        /* Do 10ms of work */
        process_sensor_data();
        work_remaining -= 10;
        
        /* Update remaining time for LLF */
        k_thread_time_left_set(NULL, work_remaining);
        
        k_sleep(K_MSEC(10));
    }
}
```

### Example 3: PFS for Fair CPU Sharing

```c
void high_priority_thread(void *p1, void *p2, void *p3)
{
    uint32_t runtime = 0;
    
    k_thread_weight_set(NULL, 400);  /* 4x share */
    
    while (1) {
        do_work();
        runtime += 10;
        k_thread_exec_time_set(NULL, runtime);
    }
}

void low_priority_thread(void *p1, void *p2, void *p3)
{
    uint32_t runtime = 0;
    
    k_thread_weight_set(NULL, 100);  /* 1x share */
    
    while (1) {
        do_work();
        runtime += 10;
        k_thread_exec_time_set(NULL, runtime);
    }
}
```

---

## Scheduler Comparison Table

| Algorithm | Config | Complexity | Context Switches | Optimal | Use Case |
|-----------|--------|------------|------------------|---------|----------|
| **Base EDF** | SCHED_DEADLINE | O(log n) | Medium | Yes | Hard RT, homogeneous |
| **Weighted EDF** | 736_MOD_EDF | O(log n) | Medium | No | Mixed-criticality |
| **RMS** | 736_RMS | O(log n) | Low | No* | Periodic tasks |
| **WSRT** | 736_WSRT | O(log n) | High | No | Response time |
| **LLF** | 736_LLF | O(log n) | Very High | Yes | Hard RT, variable exec |
| **PFS** | 736_PFS | O(log n) | Medium | No | Fairness, mixed loads |

*RMS is optimal among static priority algorithms

---

## Performance Considerations

### Overhead Sources
1. **Scheduling Decision:** O(log n) for all algorithms (priority queue)
2. **Context Switches:** Varies by algorithm
3. **Runtime Updates:** Required for WSRT, LLF, PFS
4. **Syscall Overhead:** Parameter updates

### Optimization Tips
- Minimize weight/time_left updates
- Use appropriate timer resolution
- Batch parameter updates when possible
- Choose algorithm matching workload characteristics

---

## References

1. Liu, C. L., & Layland, J. W. (1973). "Scheduling Algorithms for Multiprogramming in a Hard-Real-Time Environment"
2. Zephyr Project Documentation: https://docs.zephyrproject.org/
3. CS736 Course Materials, University of Wisconsin-Madison

---

## Testing

Test applications are provided for each scheduler:
- `app/testing/test_weighted_edf/` - Weighted EDF tests
- `app/testing/test_rms/` - Rate Monotonic Scheduling tests
- `app/testing/test_wsrt/` - WSRT tests
- `app/testing/test_llf/` - Least Laxity First tests
- `app/testing/test_pfs/` - Proportional Fair Scheduling tests

Run all tests:
```bash
./scripts/test_all_schedulers.sh
```

---

**Document Version:** 1.0  
**Date:** December 8, 2025  
**Project:** CS736 Real-Time Scheduling for Zephyr RTOS
