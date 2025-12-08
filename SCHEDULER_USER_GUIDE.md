# Real-Time Scheduler User Guide

## Table of Contents

1. [Introduction](#introduction)
2. [Quick Start](#quick-start)
3. [Choosing a Scheduler](#choosing-a-scheduler)
4. [Configuration Guide](#configuration-guide)
5. [Programming Guide](#programming-guide)
6. [Complete Examples](#complete-examples)
7. [Testing and Evaluation](#testing-and-evaluation)
8. [Troubleshooting](#troubleshooting)
9. [Performance Tuning](#performance-tuning)
10. [Advanced Topics](#advanced-topics)

---

## Introduction

This guide explains how to use the 6 real-time scheduling algorithms implemented in this Zephyr fork:

1. **EDF (Earliest Deadline First)** - Standard deadline-based scheduling
2. **Weighted EDF** - EDF with task importance weighting
3. **WSRT (Weighted Shortest Remaining Time)** - Work-based with importance
4. **RMS (Rate Monotonic Scheduling)** - Static priority based on period
5. **LLF (Least Laxity First)** - Dynamic priority based on slack time
6. **PFS (Proportional Fair Scheduling)** - Fairness-oriented scheduling

### Who Should Use This Guide?

- Embedded systems developers working on real-time applications
- Researchers comparing scheduling algorithms
- Students learning real-time operating systems
- Engineers implementing mixed-criticality systems

### Prerequisites

- Basic understanding of Zephyr RTOS
- Familiarity with real-time scheduling concepts
- Knowledge of C programming
- Understanding of periodic tasks and deadlines

---

## Quick Start

### 1. Select a Scheduler

Edit your `prj.conf`:

```kconfig
# Standard EDF (baseline)
CONFIG_SCHED_DEADLINE=y

# Or choose one of the custom schedulers:
# CONFIG_736_MOD_EDF=y           # Weighted EDF
# CONFIG_736_WSRT=y              # WSRT
# CONFIG_736_RMS=y               # RMS
# CONFIG_736_LLF=y               # LLF
# CONFIG_736_PFS=y               # PFS
```

### 2. Enable Required Dependencies

For **WSRT** or **LLF**, add runtime tracking:

```kconfig
CONFIG_SCHED_THREAD_USAGE=y
CONFIG_THREAD_RUNTIME_STATS=y
CONFIG_736_TIME_LEFT=y
```

### 3. Write Your Application

```c
#include <zephyr/kernel.h>
#include <zephyr/kernel/sched_rt.h>

void my_rt_task(void *p1, void *p2, void *p3)
{
    // Configure RT parameters: period=100ms, exec=30ms, weight=2
    k_thread_rt_config(k_current_get(), 100, 30, 2);
    
    while (1) {
        k_sleep(K_MSEC(100));  // Wait for next period
        
        // Do periodic work
        do_work();
    }
}
```

### 4. Build and Run

```bash
west build -b native_sim
west build -t run
```

---

## Choosing a Scheduler

### Decision Matrix

Use this table to select the right scheduler for your needs:

| Your Requirement | Recommended Scheduler | Why? |
|------------------|----------------------|------|
| Hard deadlines, all tasks equal priority | **EDF** | Optimal for single CPU |
| Hard deadlines, tasks have different importance | **Weighted EDF** | Maintains deadlines while prioritizing critical tasks |
| Minimize average response time | **WSRT** | Optimizes for shortest weighted completion time |
| Need predictable static priorities | **RMS** | Classical fixed-priority scheduling |
| Detect deadline misses early | **LLF** | Laxity-based early warning system |
| Prevent thread starvation, ensure fairness | **PFS** | Fair CPU allocation regardless of deadlines |
| Mixed real-time and best-effort tasks | **PFS** | Balances all tasks fairly |

### Detailed Comparison

#### When to Use EDF
```
When all tasks must meet deadlines
When tasks are equally important
When we need maximum CPU utilization (up to 100%)
When we want simple, well-understood behavior
When we don't need task differentiation
```

**Example Use Cases:**
- Sensor data collection with uniform sampling rates
- Control systems where all loops are critical
- Systems where deadline misses are unacceptable

#### When to Use Weighted EDF
```
When tasks have different criticality levels
When we need graceful degradation under overload
When we want to protect high-priority tasks
When we still need deadline awareness
When we can tolerate some low-priority deadline misses
```

**Example Use Cases:**
- Mixed-criticality avionics systems
- Industrial control with safety-critical and monitoring tasks
- Medical devices with critical alarms and routine logging

#### When to Use WSRT
```
When we want to minimize average response time
When tasks have variable execution times
When we want adaptive scheduling based on progress
When we can accept higher overhead
When we don't need hard deadline guarantees
```

**Example Use Cases:**
- Interactive embedded systems
- Task queues with varying work amounts
- Systems where responsiveness matters more than deadlines

#### When to Use RMS
```
When we need predictable, static priorities
When we want to analyze system offline
When we have periodic tasks with fixed periods
When safety certification is required
When we can accept lower utilization (up to 69%)
```

**Example Use Cases:**
- Safety-critical systems requiring certification
- Automotive ECUs with fixed-rate tasks
- Systems where predictability > utilization

#### When to Use LLF
```
When we need early deadline miss detection
When we want dynamic priority adjustment
When we are researching scheduling behavior
When we can tolerate high context switch overhead
When we are willing to handle thrashing under overload
```

**Example Use Cases:**
- Research prototypes
- Systems needing early warnings of overload
- Comparing dynamic vs static scheduling

#### When to Use PFS
```
When fairness is more important than deadlines
When we need to prevent thread starvation
When we have mixed real-time and non-real-time tasks
When we have long-running systems
When we don't need hard real-time guarantees
```

**Example Use Cases:**
- Multi-tenant embedded systems
- Mixed workload servers
- Systems with background maintenance tasks

---

## Configuration Guide

### Basic Configuration

#### Minimal Configuration (EDF)

```kconfig
# prj.conf
CONFIG_SCHED_DEADLINE=y
CONFIG_PRINTK=y
CONFIG_CONSOLE=y
```

#### Weighted Schedulers Configuration

```kconfig
# For Weighted EDF, RMS, or PFS
CONFIG_736_MOD_EDF=y    # or CONFIG_736_RMS=y or CONFIG_736_PFS=y
CONFIG_SCHED_DEADLINE=y
CONFIG_PRINTK=y
CONFIG_CONSOLE=y
```

#### Runtime Tracking Configuration (WSRT/LLF)

```kconfig
# For WSRT or LLF
CONFIG_736_WSRT=y       # or CONFIG_736_LLF=y
CONFIG_SCHED_DEADLINE=y

# Required for runtime tracking
CONFIG_SCHED_THREAD_USAGE=y
CONFIG_THREAD_RUNTIME_STATS=y
CONFIG_736_TIME_LEFT=y

# Optional: Enable thread usage analysis
CONFIG_SCHED_THREAD_USAGE_AUTO_ENABLE=n
CONFIG_SCHED_THREAD_USAGE_ALL=n

CONFIG_PRINTK=y
CONFIG_CONSOLE=y
```

### Advanced Configuration

#### Enable Timing Precision

```kconfig
# Use tickless kernel for accurate timing
CONFIG_TICKLESS_KERNEL=y

# Enable high-resolution timers (if available on your platform)
CONFIG_SYS_CLOCK_TICKS_PER_SEC=10000
```

#### Enable Thread Names (for debugging)

```kconfig
CONFIG_THREAD_NAME=y
```

#### Enable Floating Point Output (for statistics)

```kconfig
CONFIG_CBPRINTF_FP_SUPPORT=y
```

#### Optimize Stack Sizes

```kconfig
# Adjust based on your task requirements
CONFIG_MAIN_STACK_SIZE=2048
CONFIG_IDLE_STACK_SIZE=512
```

---

## Programming Guide

### API Overview

The scheduler API is defined in `<zephyr/kernel/sched_rt.h>` and provides:

```c
// Set thread weight (importance)
void k_thread_weight_set(k_tid_t tid, int weight);

// Set expected execution time
void k_thread_exec_time_set(k_tid_t tid, int exec_time);

// Convenience: set exec time in milliseconds
void k_thread_exec_time_set_ms(k_tid_t tid, uint32_t exec_time_ms);

// All-in-one configuration
void k_thread_rt_config(k_tid_t tid, uint32_t period_ms,
                        uint32_t exec_time_ms, int weight);

// Standard Zephyr APIs (still used)
void k_thread_deadline_set(k_tid_t tid, int deadline_cycles);
void k_thread_absolute_deadline_set(k_tid_t tid, int absolute_deadline);
```

### Basic Task Pattern

#### Simple Periodic Task

```c
#include <zephyr/kernel.h>
#include <zephyr/kernel/sched_rt.h>

#define TASK_PERIOD_MS    100
#define TASK_EXEC_MS      30
#define TASK_WEIGHT       1

void simple_task(void *p1, void *p2, void *p3)
{
    // Configure RT parameters once
    k_thread_rt_config(k_current_get(), 
                       TASK_PERIOD_MS, 
                       TASK_EXEC_MS, 
                       TASK_WEIGHT);
    
    while (1) {
        // Wait for next period
        k_sleep(K_MSEC(TASK_PERIOD_MS));
        
        // Do work (should complete within TASK_EXEC_MS)
        process_data();
    }
}
```

#### Task with Explicit Deadline Management

```c
void deadline_aware_task(void *p1, void *p2, void *p3)
{
    uint64_t next_release = k_uptime_get() + TASK_PERIOD_MS;
    
    k_thread_weight_set(k_current_get(), 2);  // Higher priority
    
    while (1) {
        // Set deadline for this activation
        k_thread_deadline_set(k_current_get(), 
                             k_ms_to_cyc_ceil32(TASK_PERIOD_MS));
        
        // Wait until release time
        int64_t sleep_time = next_release - k_uptime_get();
        if (sleep_time > 0) {
            k_sleep(K_MSEC(sleep_time));
        }
        
        uint64_t start = k_uptime_get();
        
        // Do work
        process_data();
        
        uint64_t end = k_uptime_get();
        uint64_t response_time = end - start;
        
        // Check if we met the deadline
        if (end > next_release + TASK_PERIOD_MS) {
            printk("Deadline missed! Response: %llu ms\n", response_time);
        }
        
        next_release += TASK_PERIOD_MS;
    }
}
```

#### Task with Statistics Tracking

```c
typedef struct {
    uint32_t activations;
    uint32_t deadline_misses;
    uint64_t total_response_time;
    uint32_t min_response;
    uint32_t max_response;
} task_stats_t;

void monitored_task(void *p1, void *p2, void *p3)
{
    task_stats_t stats = {0};
    stats.min_response = UINT32_MAX;
    
    k_thread_rt_config(k_current_get(), 100, 30, 1);
    
    while (1) {
        uint64_t start = k_uptime_get();
        k_sleep(K_MSEC(100));
        
        // Do work
        process_data();
        
        uint64_t end = k_uptime_get();
        uint32_t response = (uint32_t)(end - start);
        
        // Update statistics
        stats.activations++;
        stats.total_response_time += response;
        if (response < stats.min_response) stats.min_response = response;
        if (response > stats.max_response) stats.max_response = response;
        
        // Check deadline
        if (response > 100) {
            stats.deadline_misses++;
        }
        
        // Report every 10 activations
        if (stats.activations % 10 == 0) {
            uint32_t avg = stats.total_response_time / stats.activations;
            printk("Stats: avg=%u min=%u max=%u misses=%u\n",
                   avg, stats.min_response, stats.max_response,
                   stats.deadline_misses);
        }
    }
}
```

### Creating Tasks

#### Static Task Creation

```c
#define STACK_SIZE 1024
#define PRIORITY 5

K_THREAD_STACK_DEFINE(task_stack, STACK_SIZE);
struct k_thread task_thread;

void create_tasks(void)
{
    k_thread_create(&task_thread, task_stack, STACK_SIZE,
                    simple_task,
                    NULL, NULL, NULL,
                    PRIORITY, 0, K_NO_WAIT);
    
    k_thread_name_set(&task_thread, "SimpleTask");
}
```

#### Dynamic Task Creation

```c
k_tid_t create_rt_task(const char *name, 
                       uint32_t period_ms,
                       uint32_t exec_ms,
                       int weight)
{
    struct k_thread *thread;
    k_thread_stack_t *stack;
    
    // Allocate thread and stack
    thread = k_malloc(sizeof(struct k_thread));
    stack = k_malloc(K_THREAD_STACK_SIZEOF(1024));
    
    if (!thread || !stack) {
        return NULL;
    }
    
    k_tid_t tid = k_thread_create(thread, stack, 1024,
                                   periodic_task,
                                   (void *)(uintptr_t)period_ms,
                                   (void *)(uintptr_t)exec_ms,
                                   (void *)(uintptr_t)weight,
                                   5, 0, K_NO_WAIT);
    
    k_thread_name_set(tid, name);
    
    return tid;
}
```

### Scheduler-Specific Patterns

#### For EDF: Focus on Deadlines

```c
void edf_task(void *p1, void *p2, void *p3)
{
    while (1) {
        // EDF only cares about deadlines
        k_thread_deadline_set(k_current_get(), 
                             k_ms_to_cyc_ceil32(100));
        
        k_sleep(K_MSEC(100));
        process_data();
    }
}
```

#### For Weighted EDF: Set Task Importance

```c
void critical_task(void *p1, void *p2, void *p3)
{
    k_thread_deadline_set(k_current_get(), k_ms_to_cyc_ceil32(100));
    k_thread_weight_set(k_current_get(), 10);  // High importance
    
    while (1) {
        k_sleep(K_MSEC(100));
        critical_operation();
    }
}

void monitoring_task(void *p1, void *p2, void *p3)
{
    k_thread_deadline_set(k_current_get(), k_ms_to_cyc_ceil32(100));
    k_thread_weight_set(k_current_get(), 1);   // Low importance
    
    while (1) {
        k_sleep(K_MSEC(100));
        log_status();
    }
}
```

#### For WSRT: Track Remaining Work

```c
void wsrt_task(void *p1, void *p2, void *p3)
{
    k_thread_exec_time_set(k_current_get(), k_ms_to_cyc_ceil32(50));
    k_thread_weight_set(k_current_get(), 2);
    
    while (1) {
        k_sleep(K_MSEC(100));
        
        // WSRT uses time_left which is automatically tracked
        // Just do your work normally
        process_data();
    }
}
```

#### For RMS: Set Period (via exec_time proxy)

```c
void rms_task_short_period(void *p1, void *p2, void *p3)
{
    k_thread_exec_time_set(k_current_get(), k_ms_to_cyc_ceil32(50));
    
    while (1) {
        k_sleep(K_MSEC(50));  // Short period = high priority in RMS
        fast_control_loop();
    }
}

void rms_task_long_period(void *p1, void *p2, void *p3)
{
    k_thread_exec_time_set(k_current_get(), k_ms_to_cyc_ceil32(200));
    
    while (1) {
        k_sleep(K_MSEC(200));  // Long period = low priority in RMS
        slow_monitoring();
    }
}
```

#### For LLF: Monitor Laxity

```c
void llf_task(void *p1, void *p2, void *p3)
{
    k_thread_deadline_set(k_current_get(), k_ms_to_cyc_ceil32(100));
    k_thread_exec_time_set(k_current_get(), k_ms_to_cyc_ceil32(50));
    
    while (1) {
        k_sleep(K_MSEC(100));
        
        // LLF automatically calculates laxity and adjusts priority
        // You can detect potential misses by checking response time
        uint64_t start = k_uptime_get();
        process_data();
        uint64_t duration = k_uptime_get() - start;
        
        if (duration > 80) {  // Getting close to deadline
            printk("Warning: Low laxity detected\n");
        }
    }
}
```

#### For PFS: Set Fair Weights

```c
void high_priority_service(void *p1, void *p2, void *p3)
{
    k_thread_weight_set(k_current_get(), 3);  // Gets 3x CPU share
    
    while (1) {
        k_sleep(K_MSEC(10));
        important_work();
    }
}

void background_task(void *p1, void *p2, void *p3)
{
    k_thread_weight_set(k_current_get(), 1);  // Gets 1x CPU share
    
    while (1) {
        k_sleep(K_MSEC(10));
        background_work();
    }
}
```

---

## Complete Examples

### Example 1: Simple Multi-Task System

```c
#include <zephyr/kernel.h>
#include <zephyr/kernel/sched_rt.h>
#include <zephyr/sys/printk.h>

#define STACK_SIZE 1024
#define NUM_TASKS 3

/* Task definitions */
typedef struct {
    const char *name;
    uint32_t period_ms;
    uint32_t exec_time_ms;
    int weight;
} task_def_t;

const task_def_t task_defs[NUM_TASKS] = {
    {"FastTask",   50,  10, 3},
    {"MediumTask", 100, 20, 2},
    {"SlowTask",   200, 30, 1},
};

/* Thread storage */
K_THREAD_STACK_ARRAY_DEFINE(stacks, NUM_TASKS, STACK_SIZE);
struct k_thread threads[NUM_TASKS];

/* Generic task function */
void periodic_task(void *p1, void *p2, void *p3)
{
    const task_def_t *def = (const task_def_t *)p1;
    uint32_t count = 0;
    
    // Configure RT parameters
    k_thread_rt_config(k_current_get(), 
                       def->period_ms, 
                       def->exec_time_ms, 
                       def->weight);
    
    printk("[%s] Started: period=%u ms, weight=%d\n",
           def->name, def->period_ms, def->weight);
    
    while (1) {
        k_sleep(K_MSEC(def->period_ms));
        
        // Simulate work
        k_busy_wait(def->exec_time_ms * 1000);
        
        count++;
        if (count % 10 == 0) {
            printk("[%s] Completed %u activations\n", def->name, count);
        }
    }
}

int main(void)
{
    printk("\n=== RT Scheduler Example ===\n");
    printk("Creating %d tasks...\n\n", NUM_TASKS);
    
    // Create all tasks at same priority (scheduler differentiates)
    for (int i = 0; i < NUM_TASKS; i++) {
        k_thread_create(&threads[i], stacks[i], STACK_SIZE,
                        periodic_task,
                        (void *)&task_defs[i], NULL, NULL,
                        5, 0, K_NO_WAIT);
        
        k_thread_name_set(&threads[i], task_defs[i].name);
    }
    
    printk("All tasks started.\n");
    return 0;
}
```

### Example 2: Deadline Miss Detection

```c
#include <zephyr/kernel.h>
#include <zephyr/kernel/sched_rt.h>
#include <zephyr/sys/printk.h>

typedef struct {
    uint32_t activations;
    uint32_t deadline_misses;
    uint64_t total_response;
} stats_t;

stats_t task_stats = {0};

void monitored_task(void *p1, void *p2, void *p3)
{
    const uint32_t period_ms = 100;
    const uint32_t deadline_ms = 100;
    const uint32_t exec_ms = 30;
    
    uint64_t next_release = k_uptime_get() + period_ms;
    
    k_thread_rt_config(k_current_get(), period_ms, exec_ms, 2);
    
    while (1) {
        // Wait for release
        int64_t sleep = next_release - k_uptime_get();
        if (sleep > 0) {
            k_sleep(K_MSEC(sleep));
        }
        
        uint64_t start = k_uptime_get();
        uint64_t abs_deadline = next_release + deadline_ms;
        
        task_stats.activations++;
        
        // Set deadline for this period
        k_thread_deadline_set(k_current_get(), 
                             k_ms_to_cyc_ceil32(deadline_ms));
        
        // Do work
        k_busy_wait(exec_ms * 1000);
        
        uint64_t end = k_uptime_get();
        uint32_t response = (uint32_t)(end - start);
        task_stats.total_response += response;
        
        // Check deadline
        bool missed = (end > abs_deadline);
        if (missed) {
            task_stats.deadline_misses++;
            int32_t lateness = (int32_t)(end - abs_deadline);
            printk("⚠ Deadline MISS #%u (lateness: %d ms)\n",
                   task_stats.deadline_misses, lateness);
        }
        
        // Report statistics
        if (task_stats.activations % 20 == 0) {
            uint32_t avg = task_stats.total_response / task_stats.activations;
            float miss_rate = 100.0f * task_stats.deadline_misses / 
                             task_stats.activations;
            
            printk("\n=== Statistics ===\n");
            printk("Activations: %u\n", task_stats.activations);
            printk("Deadline Misses: %u (%.1f%%)\n", 
                   task_stats.deadline_misses, miss_rate);
            printk("Avg Response: %u ms\n", avg);
            printk("==================\n\n");
        }
        
        next_release += period_ms;
    }
}
```

### Example 3: Mixed-Criticality System

```c
#include <zephyr/kernel.h>
#include <zephyr/kernel/sched_rt.h>

/* Safety-critical control task */
void safety_critical_task(void *p1, void *p2, void *p3)
{
    k_thread_rt_config(k_current_get(), 
                       20,   // 20ms period (50Hz)
                       5,    // 5ms execution
                       10);  // Highest weight
    
    while (1) {
        k_sleep(K_MSEC(20));
        
        // Critical safety checks and control
        safety_check();
        control_actuators();
    }
}

/* Important monitoring task */
void monitoring_task(void *p1, void *p2, void *p3)
{
    k_thread_rt_config(k_current_get(), 
                       100,  // 100ms period (10Hz)
                       20,   // 20ms execution
                       5);   // Medium weight
    
    while (1) {
        k_sleep(K_MSEC(100));
        
        // Monitor system health
        check_sensors();
        update_display();
    }
}

/* Low-priority logging task */
void logging_task(void *p1, void *p2, void *p3)
{
    k_thread_rt_config(k_current_get(), 
                       1000, // 1s period (1Hz)
                       50,   // 50ms execution
                       1);   // Lowest weight
    
    while (1) {
        k_sleep(K_MSEC(1000));
        
        // Log data to storage
        write_logs();
    }
}
```

### Example 4: Algorithm Comparison

This example shows how to test different schedulers:

```c
#include <zephyr/kernel.h>
#include <zephyr/kernel/sched_rt.h>
#include <zephyr/sys/printk.h>

/* CSV output for later analysis */
#define CSV_OUTPUT 1

void test_task(void *p1, void *p2, void *p3)
{
    uint32_t task_id = (uint32_t)(uintptr_t)p1;
    uint32_t period_ms = (uint32_t)(uintptr_t)p2;
    uint32_t exec_ms = (uint32_t)(uintptr_t)p3;
    
    uint32_t activations = 0;
    uint64_t next_release = k_uptime_get() + 500; // Synchronized start
    
    k_thread_rt_config(k_current_get(), period_ms, exec_ms, 1);
    
    while (activations < 50) {  // Run 50 activations
        int64_t sleep = next_release - k_uptime_get();
        if (sleep > 0) {
            k_sleep(K_MSEC(sleep));
        }
        
        uint64_t start = k_uptime_get();
        k_busy_wait(exec_ms * 1000);
        uint64_t end = k_uptime_get();
        
        activations++;
        uint32_t response = (uint32_t)(end - start);
        bool missed = (response > period_ms);
        
#if CSV_OUTPUT
        printk("CSV,%llu,%u,%u,%u,%d\n",
               start, task_id, activations, response, missed ? 1 : 0);
#endif
        
        next_release += period_ms;
    }
    
    printk("Task %u completed\n", task_id);
}

int main(void)
{
#if defined(CONFIG_736_MOD_EDF)
    printk("Scheduler: Weighted EDF\n");
#elif defined(CONFIG_736_WSRT)
    printk("Scheduler: WSRT\n");
#elif defined(CONFIG_736_RMS)
    printk("Scheduler: RMS\n");
#elif defined(CONFIG_736_LLF)
    printk("Scheduler: LLF\n");
#elif defined(CONFIG_736_PFS)
    printk("Scheduler: PFS\n");
#else
    printk("Scheduler: EDF\n");
#endif
    
    // Create test tasks
    // (Create thread code here)
    
    return 0;
}
```

---

## Testing and Evaluation

### Using the Built-in Test Framework

The repository includes comprehensive test applications:

#### Simple Evaluation

```bash
cd app/simple_eval_step1

# Run single test
west build -b native_sim
west build -t run > results/output.txt

# Run all combinations (6 schedulers × 4 workloads)
bash scripts/run_all_tests.sh

# Generate graphs
python3 scripts/generate_graphs.py
```

#### Advanced Evaluation

```bash
cd app/advanced_eval

# Includes jitter analysis and 100 activations per task
bash scripts/run_all_tests.sh
python3 scripts/generate_graphs.py
```

### Creating Custom Tests

#### Test Configuration Template

```c
// test_config.h
#define NUM_TASKS 4
#define MAX_ACTIVATIONS 50

typedef struct {
    const char *name;
    uint32_t period_ms;
    uint32_t exec_time_ms;
    uint32_t deadline_ms;
    int weight;
} task_config_t;

// Define your workload
task_config_t workload[] = {
    {"T1", 100, 20, 100, 2},
    {"T2", 200, 40, 200, 2},
    {"T3", 300, 60, 300, 1},
    {"T4", 500, 80, 500, 1},
};
```

#### Collecting Metrics

```c
typedef struct {
    uint32_t activations;
    uint32_t deadline_misses;
    uint64_t total_response_time;
    uint64_t sum_response_squared;  // For jitter
    uint32_t min_response;
    uint32_t max_response;
} metrics_t;

void calculate_statistics(metrics_t *m)
{
    uint32_t avg = m->total_response_time / m->activations;
    
    // Calculate standard deviation (jitter)
    uint64_t variance = (m->sum_response_squared / m->activations) - 
                        (avg * avg);
    uint32_t std_dev = (uint32_t)sqrt(variance);
    
    float miss_rate = 100.0f * m->deadline_misses / m->activations;
    
    printk("Avg Response: %u ms\n", avg);
    printk("Std Dev (jitter): %u ms\n", std_dev);
    printk("Min/Max: %u/%u ms\n", m->min_response, m->max_response);
    printk("Miss Rate: %.2f%%\n", miss_rate);
}
```

### Comparing Schedulers

Create a comparison script:

```bash
#!/bin/bash
# compare_schedulers.sh

SCHEDULERS=("EDF" "WEIGHTED_EDF" "WSRT" "RMS" "LLF" "PFS")

for sched in "${SCHEDULERS[@]}"; do
    echo "Testing $sched..."
    
    # Update prj.conf for this scheduler
    # Build and run
    # Collect results
done

# Generate comparison graphs
python3 analyze_results.py
```

---

## Troubleshooting

### Common Issues and Solutions

#### Issue: Tasks Not Being Scheduled

**Symptoms:**
- Tasks created but never execute
- No output from task code

**Solutions:**
```c
// 1. Check thread priority (use same priority for scheduler to differentiate)
k_thread_create(&thread, stack, STACK_SIZE, task_func,
                NULL, NULL, NULL,
                5,  // Same priority for all RT tasks
                0, K_NO_WAIT);

// 2. Ensure scheduler is configured
// Check prj.conf has CONFIG_SCHED_DEADLINE=y or scheduler option

// 3. Verify thread starts immediately
k_thread_create(..., K_NO_WAIT);  // Not K_FOREVER
```

#### Issue: High Deadline Miss Rate

**Symptoms:**
- Many deadlines missed
- System appears overloaded

**Solutions:**
```c
// 1. Check total utilization
// Sum of (exec_time / period) should be < 1.0

// 2. Verify execution time estimates
uint64_t start = k_uptime_get();
do_work();
uint64_t actual = k_uptime_get() - start;
printk("Actual exec time: %llu ms\n", actual);

// 3. Reduce workload or increase periods
```

#### Issue: WSRT/LLF Not Working

**Symptoms:**
- Behaves like EDF
- Runtime tracking not happening

**Solutions:**
```kconfig
# Ensure all required configs are set
CONFIG_736_WSRT=y  # or CONFIG_736_LLF=y
CONFIG_SCHED_THREAD_USAGE=y
CONFIG_THREAD_RUNTIME_STATS=y
CONFIG_736_TIME_LEFT=y

# Not just:
# CONFIG_SCHED_DEADLINE=y  # This gives plain EDF
```

#### Issue: Weights Have No Effect

**Symptoms:**
- Setting weights doesn't change behavior
- All tasks treated equally

**Solutions:**
```c
// 1. Check you're using a weighted scheduler
#if !defined(CONFIG_736_MOD_EDF) && !defined(CONFIG_736_WSRT) && \
    !defined(CONFIG_736_PFS)
#error "Current scheduler doesn't use weights"
#endif

// 2. Ensure weights are set AFTER thread creation
k_thread_create(&thread, ...);
k_thread_weight_set(&thread, 5);  // Set weight after creation

// 3. Verify weights are different enough
k_thread_weight_set(&critical, 10);
k_thread_weight_set(&normal, 1);   // 10x difference
```

#### Issue: Compilation Errors

**Common errors:**

```c
// Error: k_thread_weight_set undeclared
// Solution: Include header and check config
#include <zephyr/kernel/sched_rt.h>
#if defined(CONFIG_736)
k_thread_weight_set(tid, weight);
#endif

// Error: CONFIG_736_MOD_EDF not defined
// Solution: Choose only ONE scheduler in prj.conf

// Error: runtime tracking symbols missing
// Solution: Add full WSRT/LLF configuration (see above)
```

### Debug Techniques

#### Enable Debug Output

```c
#define DEBUG_SCHED 1

#if DEBUG_SCHED
#define SCHED_LOG(fmt, ...) printk("[SCHED] " fmt, ##__VA_ARGS__)
#else
#define SCHED_LOG(fmt, ...)
#endif

void task(void *p1, void *p2, void *p3)
{
    SCHED_LOG("Task started\n");
    SCHED_LOG("Weight set to: %d\n", weight);
    // ...
}
```

#### Monitor Scheduler Behavior

```c
void monitor_task(void *p1, void *p2, void *p3)
{
    while (1) {
        k_sleep(K_MSEC(1000));
        
        // Get thread stats (if enabled)
#ifdef CONFIG_SCHED_THREAD_USAGE
        k_thread_runtime_stats_t stats;
        k_thread_runtime_stats_get(k_current_get(), &stats);
        printk("CPU usage: %llu cycles\n", stats.execution_cycles);
#endif
    }
}
```

---

## Performance Tuning

### Optimizing for Your Scheduler

#### EDF Optimization

```c
// Use accurate deadlines
uint32_t deadline_cycles = k_ms_to_cyc_ceil32(period_ms);
k_thread_deadline_set(tid, deadline_cycles);

// Minimize priority changes - keep same base priority
#define RT_PRIORITY 5
```

#### Weighted EDF Tuning

```c
// Choose weight ratios carefully
#define CRITICAL_WEIGHT    10  // Safety-critical
#define IMPORTANT_WEIGHT   5   // Important but not critical
#define NORMAL_WEIGHT      2   // Normal priority
#define BACKGROUND_WEIGHT  1   // Best-effort

// Under overload, critical tasks protected:
// effective_deadline(critical) = deadline / 10
// effective_deadline(background) = deadline / 1
```

#### WSRT Tuning

```c
// Provide accurate execution time estimates
k_thread_exec_time_set(tid, k_ms_to_cyc_ceil32(measured_exec_ms));

// Balance weights for response time optimization
// Higher weight = more CPU time when competing
```

#### RMS Tuning

```c
// Assign exec_time based on period (shorter period = higher priority)
// For 50ms period task:
k_thread_exec_time_set(tid, k_ms_to_cyc_ceil32(50));

// For 200ms period task:
k_thread_exec_time_set(tid, k_ms_to_cyc_ceil32(200));

// Verify utilization bound (69% for RMS)
float utilization = sum(exec_time / period);
if (utilization > 0.69) {
    printk("Warning: May not be schedulable under RMS\n");
}
```

#### LLF Tuning

```c
// Provide accurate exec time and deadlines
k_thread_exec_time_set(tid, k_ms_to_cyc_ceil32(exec_ms));
k_thread_deadline_set(tid, k_ms_to_cyc_ceil32(deadline_ms));

// Monitor for thrashing
static uint32_t last_context_switches = 0;
uint32_t current = get_context_switch_count();
if (current - last_context_switches > THRESHOLD) {
    printk("Warning: Possible thrashing\n");
}
last_context_switches = current;
```

#### PFS Tuning

```c
// Set weights based on desired CPU share
// Total CPU share = sum of all weights
// Task CPU share = weight / total_weight

// Example: Want 50%, 30%, 20% distribution
k_thread_weight_set(task1, 5);  // 5/10 = 50%
k_thread_weight_set(task2, 3);  // 3/10 = 30%
k_thread_weight_set(task3, 2);  // 2/10 = 20%
```

### System-Level Optimization

#### Reduce Context Switch Overhead

```kconfig
# Use larger time slices if supported
CONFIG_TIMESLICE_SIZE=10

# Minimize tick rate if precision not critical
CONFIG_SYS_CLOCK_TICKS_PER_SEC=1000
```

#### Memory Optimization

```kconfig
# Minimize stack sizes
CONFIG_MAIN_STACK_SIZE=1024
CONFIG_IDLE_STACK_SIZE=256

# Per-thread stacks
#define TASK_STACK_SIZE 512  // Adjust based on actual usage
```

#### Timing Precision

```kconfig
# Enable tickless for better accuracy
CONFIG_TICKLESS_KERNEL=y

# Increase timer resolution
CONFIG_SYS_CLOCK_TICKS_PER_SEC=10000
```

---

## Advanced Topics

### Runtime Scheduler Statistics

```c
#ifdef CONFIG_SCHED_THREAD_USAGE
void print_thread_stats(k_tid_t tid)
{
    k_thread_runtime_stats_t stats;
    k_thread_runtime_stats_get(tid, &stats);
    
    printk("Thread stats:\n");
    printk("  Execution cycles: %llu\n", stats.execution_cycles);
    printk("  Total cycles: %llu\n", stats.total_cycles);
    
    uint64_t cpu_percent = (stats.execution_cycles * 100) / stats.total_cycles;
    printk("  CPU usage: %llu%%\n", cpu_percent);
}
#endif
```

### Handling Overload Conditions

```c
void overload_detector_task(void *p1, void *p2, void *p3)
{
    uint32_t miss_count = 0;
    const uint32_t MISS_THRESHOLD = 5;
    
    while (1) {
        k_sleep(K_MSEC(1000));
        
        // Check system deadline miss rate
        if (get_total_deadline_misses() > miss_count + MISS_THRESHOLD) {
            printk("⚠ OVERLOAD DETECTED\n");
            
            // Take corrective action
#ifdef CONFIG_736_MOD_EDF
            // Increase weights of critical tasks
            increase_critical_task_weights();
#endif
            
            // Or shed non-critical load
            suspend_background_tasks();
        }
        
        miss_count = get_total_deadline_misses();
    }
}
```

### Multi-Core Considerations

```c
// Pin RT tasks to specific cores if needed
#ifdef CONFIG_SMP
#ifdef CONFIG_SCHED_CPU_MASK
void pin_to_core(k_tid_t tid, int core_id)
{
    k_thread_cpu_mask_clear(tid);
    k_thread_cpu_mask_enable(tid, core_id);
}
#endif
#endif
```

### Mixing Schedulers (Future Enhancement)

Currently, you select one scheduler for all threads at the same priority level.
For future enhancement to mix schedulers:

```c
// Hypothetical future API
k_thread_set_scheduler(tid, SCHED_WEIGHTED_EDF);
```

---

## Summary

This guide covered:

- **Choosing the right scheduler** for your application  
- **Configuration** for each scheduler type  
- **Programming patterns** and best practices  
- **Complete examples** from simple to advanced  
- **Testing and evaluation** using built-in tools  
- **Troubleshooting** common issues  
- **Performance tuning** for optimal results  

### Quick Decision Guide

```
Hard real-time → EDF or Weighted EDF
Predictable static → RMS
Minimize response time → WSRT
Early miss detection → LLF
Fairness & starvation prevention → PFS
```

### Next Steps

1. Choose your scheduler based on requirements
2. Configure `prj.conf` appropriately
3. Use the example code as a template
4. Test with the evaluation framework
5. Tune parameters based on results

### Further Reading

- `ALGORITHM_COMPARISON.md` - Detailed algorithm comparison
- `MODULAR_DESIGN.md` - Implementation architecture
- `NEW_ALGORITHMS.md` - LLF & PFS specifics
- `doc/kernel/services/scheduling/rt_schedulers.rst` - API reference
- `samples/scheduler_example/` - Working example

---

**Questions or issues?** Check the troubleshooting section or review the example applications in `app/simple_eval_step1/` and `app/advanced_eval/`.
