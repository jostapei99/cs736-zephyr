# Real-Time Workload Construction Guide

This guide will walk you through building RT workloads step-by-step to evaluate different schedulers in Zephyr RTOS.

## Core Concepts

### 1. Real-Time Task Model
A periodic real-time task is characterized by:
- **Period (T)**: How often the task executes (e.g., every 50ms)
- **Deadline (D)**: Time by which task must complete (often D = T)
- **Execution Time (C)**: How long the task actually runs
- **Weight (W)**: Priority parameter for weighted schedulers

### 2. Task Execution Pattern
```
Timeline:    |----Period----|----Period----|----Period----|
Task:        [Execute][Sleep][Execute][Sleep][Execute][Sleep]
             |<--C-->|       |<--C-->|       |<--C-->|
Deadline:            ^               ^               ^
```

### 3. What to Measure
- **Deadline Misses**: Did task finish before its deadline?
- **Response Time**: How long from task release to completion?
- **Jitter**: Variation in response time (consistency)
- **Context Switches**: How often the scheduler switches tasks?

## Step-by-Step Construction

### Step 1: Single Simple Task
**Goal**: Create one periodic task that runs forever

**What you'll learn**:
- How to create a Zephyr thread
- How to use `k_msleep()` for periodic execution
- How to measure time with `k_cycle_get_32()`

**Pseudo-code**:
```c
void task_function(void *arg1, void *arg2, void *arg3) {
    while (1) {
        uint32_t start_time = k_cycle_get_32();
        
        // Do some work (simulate computation)
        busy_wait_microseconds(10000);  // 10ms of work
        
        uint32_t end_time = k_cycle_get_32();
        printk("Task executed in %u cycles\n", end_time - start_time);
        
        // Sleep until next period
        k_msleep(50);  // 50ms period
    }
}
```

**Questions to consider**:
- What happens if your work takes longer than the period?
- How accurate is `k_msleep()` timing?

---

### Step 2: Multiple Tasks with Different Periods
**Goal**: Run 2-3 tasks with different periods simultaneously

**What you'll learn**:
- Task interactions and scheduling
- How Zephyr's scheduler decides which task runs
- Thread priorities in Zephyr

**Design considerations**:
- Task 1: Period = 50ms, Work = 10ms
- Task 2: Period = 100ms, Work = 15ms
- Task 3: Period = 200ms, Work = 20ms

**Key question**: If all tasks have the same priority, what order do they execute?

---

### Step 3: Measuring Deadline Adherence
**Goal**: Track whether each task meets its deadline

**What you'll learn**:
- Calculate response time
- Detect deadline misses
- Store metrics for later analysis

**Data to collect per task execution**:
```c
struct task_event {
    uint32_t timestamp;      // When did this happen?
    uint8_t task_id;         // Which task?
    uint32_t response_time;  // How long did it take?
    uint8_t deadline_met;    // Did we meet the deadline?
    int32_t lateness;        // How late were we? (negative = early)
};
```

**Challenge**: How do you safely collect this data from multiple threads?

---

### Step 4: Workload Characterization
**Goal**: Create realistic workloads that stress the scheduler

**What you'll learn**:
- CPU utilization calculation: U = Σ(C_i / T_i)
- Schedulability analysis
- Workload design patterns

**Workload types to try**:
1. **Light Load** (U ≈ 30%): All tasks easily schedulable
2. **Medium Load** (U ≈ 60%): Challenging but manageable
3. **Heavy Load** (U ≈ 85%): Near the schedulability bound
4. **Overload** (U > 100%): Guaranteed deadline misses

**Example calculation**:
- Task 1: C=10ms, T=50ms → U₁ = 10/50 = 0.20
- Task 2: C=15ms, T=100ms → U₂ = 15/100 = 0.15
- Task 3: C=20ms, T=200ms → U₃ = 20/200 = 0.10
- **Total**: U = 0.20 + 0.15 + 0.10 = 0.45 (45% utilization)

---

### Step 5: Testing Different Schedulers
**Goal**: Compare how EDF, RMS, WSRT, and Weighted EDF handle your workload

**What you'll learn**:
- How to enable different schedulers in Zephyr
- Scheduler-specific configuration (weights, priorities)
- Performance differences between algorithms

**Scheduler configurations**:
```
# EDF (Earliest Deadline First)
CONFIG_SCHED_DEADLINE=y

# Weighted EDF
CONFIG_736_MOD_EDF=y

# WSRT (Weighted Shortest Remaining Time)
CONFIG_736_WSRT=y

# RMS (Rate Monotonic Scheduling)
CONFIG_736_RMS=y
```

---

### Step 6: Statistical Analysis
**Goal**: Collect enough data to make meaningful comparisons

**What you'll learn**:
- How long to run experiments
- What statistics matter (mean, median, percentiles)
- How to visualize results

**Metrics to calculate**:
- Deadline miss ratio (% of activations that missed)
- Mean/median/P95/P99 response time
- Response time jitter (standard deviation)
- Context switch overhead

---

## Implementation Tips

### Timing in Zephyr
```c
// Get current time in cycles
uint32_t now = k_cycle_get_32();

// Convert cycles to microseconds
uint32_t us = k_cyc_to_us_floor32(cycles);

// Convert cycles to milliseconds
uint32_t ms = k_cyc_to_ms_floor32(cycles);
```

### Thread Creation
```c
#define STACK_SIZE 1024
#define PRIORITY 7  // Lower number = higher priority in Zephyr

K_THREAD_STACK_DEFINE(task1_stack, STACK_SIZE);
struct k_thread task1_thread;

k_thread_create(&task1_thread, task1_stack,
                K_THREAD_STACK_SIZEOF(task1_stack),
                task_function,
                NULL, NULL, NULL,
                PRIORITY, 0, K_NO_WAIT);
```

### Safe Data Collection
```c
// Use a spinlock for thread-safe metric storage
K_SPINLOCK_DEFINE(metrics_lock);

void record_metric(struct task_event *event) {
    k_spinlock_key_t key = k_spin_lock(&metrics_lock);
    
    // Store your metric here
    
    k_spin_unlock(&metrics_lock, key);
}
```

## Next Steps

Start with **Step 1** - create a simple periodic task and verify:
1. It prints output every period
2. The timing is accurate
3. You understand how `k_msleep()` works

Once that works, move to Step 2 and add more tasks!

## Questions to Guide Your Implementation

1. **What happens if a task's work exceeds its period?**
   - Does it block other tasks?
   - How does the scheduler handle it?

2. **Why use `k_msleep()` instead of busy-waiting?**
   - What's the difference in CPU usage?
   - What happens to other threads during sleep?

3. **How do you choose thread priorities?**
   - For EDF: All tasks same priority (scheduler decides by deadline)
   - For RMS: Priority inversely proportional to period
   - For weighted: How do weights affect priorities?

4. **How much data is enough?**
   - 10 activations? 100? 1000?
   - How long should the evaluation run?

Good luck! Start small, verify each step, and build complexity gradually.
