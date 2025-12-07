# Workload Integration Strategies

## Option 1: Compile-Time Selection (Recommended for Testing)

Define workload profiles at the top of `main.c`:

```c
/* ============================================
 * WORKLOAD SELECTION - Change this to test different scenarios
 * ============================================ */
#define WORKLOAD_LIGHT      1
#define WORKLOAD_MEDIUM     2
#define WORKLOAD_HEAVY      3
#define WORKLOAD_OVERLOAD   4
#define WORKLOAD_WEIGHTED   5

/* SELECT WORKLOAD HERE */
#define CURRENT_WORKLOAD WORKLOAD_MEDIUM

/* ============================================
 * WORKLOAD DEFINITIONS
 * ============================================ */

#if CURRENT_WORKLOAD == WORKLOAD_LIGHT
    /* Light load: 50% total utilization - all deadlines should be met */
    #define TASK1_PERIOD_MS    100
    #define TASK1_EXEC_TIME_MS 30
    #define TASK1_DEADLINE_MS  100
    #define TASK1_WEIGHT       1
    
    #define TASK2_PERIOD_MS    200
    #define TASK2_EXEC_TIME_MS 40
    #define TASK2_DEADLINE_MS  200
    #define TASK2_WEIGHT       1

#elif CURRENT_WORKLOAD == WORKLOAD_MEDIUM
    /* Medium load: 70% total utilization */
    #define TASK1_PERIOD_MS    100
    #define TASK1_EXEC_TIME_MS 40
    #define TASK1_DEADLINE_MS  100
    #define TASK1_WEIGHT       1
    
    #define TASK2_PERIOD_MS    200
    #define TASK2_EXEC_TIME_MS 60
    #define TASK2_DEADLINE_MS  200
    #define TASK2_WEIGHT       1

#elif CURRENT_WORKLOAD == WORKLOAD_HEAVY
    /* Heavy load: 90% total utilization - tight but schedulable */
    #define TASK1_PERIOD_MS    100
    #define TASK1_EXEC_TIME_MS 50
    #define TASK1_DEADLINE_MS  100
    #define TASK1_WEIGHT       1
    
    #define TASK2_PERIOD_MS    200
    #define TASK2_EXEC_TIME_MS 80
    #define TASK2_DEADLINE_MS  200
    #define TASK2_WEIGHT       1

#elif CURRENT_WORKLOAD == WORKLOAD_OVERLOAD
    /* Overload: 110% utilization - guaranteed deadline misses */
    #define TASK1_PERIOD_MS    100
    #define TASK1_EXEC_TIME_MS 60
    #define TASK1_DEADLINE_MS  100
    #define TASK1_WEIGHT       1
    
    #define TASK2_PERIOD_MS    200
    #define TASK2_EXEC_TIME_MS 100
    #define TASK2_DEADLINE_MS  200
    #define TASK2_WEIGHT       1

#elif CURRENT_WORKLOAD == WORKLOAD_WEIGHTED
    /* Weighted workload: Test weight-based scheduling */
    #define TASK1_PERIOD_MS    100
    #define TASK1_EXEC_TIME_MS 60
    #define TASK1_DEADLINE_MS  100
    #define TASK1_WEIGHT       2  // Higher weight (more important)
    
    #define TASK2_PERIOD_MS    200
    #define TASK2_EXEC_TIME_MS 100
    #define TASK2_DEADLINE_MS  200
    #define TASK2_WEIGHT       1  // Normal weight

#else
    #error "Invalid workload selection"
#endif
```

**Usage**: Change `CURRENT_WORKLOAD` and rebuild.

---

## Option 2: Array-Based Configuration (More Flexible)

Define workloads as arrays:

```c
typedef struct {
    const char *name;
    uint32_t period_ms;
    uint32_t exec_time_ms;
    uint32_t deadline_ms;
    uint32_t weight;
} task_config_t;

typedef struct {
    const char *workload_name;
    task_config_t task1;
    task_config_t task2;
} workload_t;

/* Define all workloads */
const workload_t workloads[] = {
    {
        .workload_name = "Light Load (50%)",
        .task1 = {"Task1", 100, 30, 100, 1},
        .task2 = {"Task2", 200, 40, 200, 1}
    },
    {
        .workload_name = "Medium Load (70%)",
        .task1 = {"Task1", 100, 40, 100, 1},
        .task2 = {"Task2", 200, 60, 200, 1}
    },
    {
        .workload_name = "Heavy Load (90%)",
        .task1 = {"Task1", 100, 50, 100, 1},
        .task2 = {"Task2", 200, 80, 200, 1}
    },
    {
        .workload_name = "Overload (110%)",
        .task1 = {"Task1", 100, 60, 100, 1},
        .task2 = {"Task2", 200, 100, 200, 1}
    }
};

#define NUM_WORKLOADS (sizeof(workloads) / sizeof(workloads[0]))

/* Select workload at runtime */
#define SELECTED_WORKLOAD 2  // Index into workloads array

/* Use it in tasks */
void task1(void *a, void *b, void *c) {
    const task_config_t *config = &workloads[SELECTED_WORKLOAD].task1;
    
    while (1) {
        // Use config->period_ms, config->exec_time_ms, etc.
    }
}
```

**Advantage**: Easy to add more workloads without modifying code structure.

---

## Option 3: Generic Task Function (Most Advanced)

Create a single generic task that uses configuration:

```c
typedef struct {
    const char *name;
    uint32_t task_id;
    task_config_t *config;
    task_stats_t *stats;
} task_context_t;

void generic_task(void *arg1, void *arg2, void *arg3)
{
    task_context_t *ctx = (task_context_t *)arg1;
    task_config_t *cfg = ctx->config;
    task_stats_t *stats = ctx->stats;
    
    k_thread_weight_set(k_current_get(), cfg->weight);
    init_release(stats, cfg->period_ms);
    
    while (1) {
        uint64_t now = k_uptime_get();
        uint64_t release_time = stats->next_release;
        uint64_t abs_deadline = release_time + cfg->deadline_ms;
        
        k_thread_deadline_set(k_current_get(), k_ms_to_cyc_ceil32(abs_deadline));
        
        if (now < release_time) {
            k_sleep(K_MSEC(release_time - now));
            now = k_uptime_get();
        }
        
        stats->activations++;
        
        /* Simulate work */
        k_busy_wait(cfg->exec_time_ms * 1000);
        
        uint64_t end = k_uptime_get();
        uint32_t response_time = (uint32_t)(end - now);
        
        /* Update stats... */
        
        stats->next_release += cfg->period_ms;
    }
}

/* Create tasks dynamically */
int main(void)
{
    task_config_t task1_config = {.period_ms = 100, .exec_time_ms = 50, ...};
    task_config_t task2_config = {.period_ms = 200, .exec_time_ms = 80, ...};
    
    task_context_t task1_ctx = {.name = "Task1", .config = &task1_config, ...};
    task_context_t task2_ctx = {.name = "Task2", .config = &task2_config, ...};
    
    k_thread_create(&task1_thread, task1_stack, STACK_SIZE,
                    generic_task, &task1_ctx, NULL, NULL,
                    PRIORITY, 0, K_NO_WAIT);
                    
    k_thread_create(&task2_thread, task2_stack, STACK_SIZE,
                    generic_task, &task2_ctx, NULL, NULL,
                    PRIORITY, 0, K_NO_WAIT);
}
```

**Advantage**: Can create N tasks with a loop!

---

## Option 4: Build Script with Multiple Configurations

Create separate config files and build them in a loop:

```bash
#!/bin/bash

workloads=("light" "medium" "heavy" "overload")
schedulers=("edf" "weighted_edf" "wsrt" "rms")

for workload in "${workloads[@]}"; do
    for scheduler in "${schedulers[@]}"; do
        echo "Testing $scheduler with $workload workload"
        
        # Update workload selection in source
        sed -i "s/#define CURRENT_WORKLOAD.*/#define CURRENT_WORKLOAD WORKLOAD_${workload^^}/" src/main.c
        
        # Update scheduler in prj.conf
        # ... (enable appropriate scheduler)
        
        # Build and run
        west build -b native_sim -p
        west build -t run > results/${scheduler}_${workload}.log
    done
done
```

---

## Recommended Approach for Your Use Case

**Use Option 1 (Compile-Time Selection)** because:
- Simple to implement
- Easy to understand
- Clear what's being tested
- Good for systematic evaluation

**Workflow**:
1. Change `CURRENT_WORKLOAD` in code
2. Rebuild
3. Run and save output
4. Change scheduler in `prj.conf`
5. Repeat

**Later enhancement**: Move to Option 3 (Generic Task) when you want:
- More than 2 tasks
- Automated testing of many configurations
- Runtime workload switching

---

## Example Integration for Your Current Code

Here's how to add it to your existing `main.c`:

```c
// At the top of main.c:

#define WORKLOAD_LIGHT    1
#define WORKLOAD_MEDIUM   2
#define WORKLOAD_HEAVY    3
#define WORKLOAD_OVERLOAD 4

#define CURRENT_WORKLOAD WORKLOAD_HEAVY

// Then define all configs...
// Your task functions stay the same, just reference these #defines

// In main(), print which workload is active:
printk("Testing: %s\n", 
#if CURRENT_WORKLOAD == WORKLOAD_LIGHT
    "Light Load (50%)"
#elif CURRENT_WORKLOAD == WORKLOAD_MEDIUM
    "Medium Load (70%)"
#elif CURRENT_WORKLOAD == WORKLOAD_HEAVY
    "Heavy Load (90%)"
#elif CURRENT_WORKLOAD == WORKLOAD_OVERLOAD
    "Overload (110%)"
#endif
);
```

Want me to implement Option 1 in your current code?
