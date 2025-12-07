#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "workloads.h"

#define DEBUG_STATEMENTS 1 /* Set to 1 to enable debug prints */
#define CSV_OUTPUT 1       /* Set to 1 to enable CSV data export */

#define STACK_SIZE 2048
#define PRIORITY   5

/* Synchronization: all tasks start together */
#define FIRST_RELEASE_MS   500

/* Test Duration */
#define TEST_DURATION_MS 60000  /* Run for 60 seconds */

bool terminate_tasks = false;

/* Thread storage - array for multiple tasks */
K_THREAD_STACK_ARRAY_DEFINE(task_stacks, NUM_TASKS, STACK_SIZE);
struct k_thread task_threads[NUM_TASKS];

/* Stats structure */
typedef struct {
    uint64_t next_release;
    uint32_t activations;
    uint32_t deadline_misses;
    uint32_t total_response_time;
    uint64_t sum_response_time_squared;  /* For jitter calculation */
    uint32_t min_response_time;
    uint32_t max_response_time;
} task_stats_t;

/* Task context - passed to each task instance */
typedef struct {
    uint32_t task_id;
    const task_config_t *config;
    task_stats_t *stats;
} task_context_t;

/* Arrays for all tasks */
task_stats_t task_stats[NUM_TASKS] = {0};
task_context_t task_contexts[NUM_TASKS];

/**
 * Initialize task release time - synchronized across all tasks
 */
static inline void init_release(task_stats_t *stats, uint32_t period_ms)
{
    if (stats->next_release == 0) {
        stats->next_release = FIRST_RELEASE_MS;  /* All tasks start at same time */
    }
}

/**
 * Generic periodic real-time task
 * Can be used for any number of tasks by passing different contexts
 */
void periodic_task(void *arg1, void *arg2, void *arg3)
{
    // unused args
    ARG_UNUSED(arg2); ARG_UNUSED(arg3);

    task_context_t *ctx = (task_context_t *)arg1;
    const task_config_t *cfg = ctx->config;
    uint32_t weight = cfg->weight;
    task_stats_t *stats = ctx->stats;
    uint32_t task_id = ctx->task_id;

    /* Set thread properties */
    k_thread_weight_set(k_current_get(), weight);    // Set weight from config
    init_release(stats, cfg->period_ms);          // Use period from config

    while (!terminate_tasks) {
        uint64_t now = k_uptime_get();
        uint64_t release_time = stats->next_release;
        uint64_t abs_deadline = release_time + cfg->deadline_ms;  // Use deadline from config
        k_thread_deadline_set(k_current_get(), k_ms_to_cyc_ceil32(abs_deadline));

#if DYNAMIC_WEIGHTING
        // Adjust weight based on deadline miss rate
        if ((float)stats->deadline_misses / stats->activations > WEIGHT_ADJUSTMENT_THRESHOLD) {
            weight = weight + 1;  // Increase weight
            k_thread_weight_set(k_current_get(), weight);
        } else if (weight > 1) {
            weight = weight - 1;  // Decrease weight, minimum 1
            k_thread_weight_set(k_current_get(), weight);
        }
#endif

        /* Wait until release time */
        if (now < release_time) {
            k_sleep(K_MSEC(release_time - now));
            now = k_uptime_get();
        }

        stats->activations++;

#if DEBUG_STATEMENTS
        printk("[Task%u] Activation: %u Released at %llu ms\n", 
                task_id, stats->activations, (unsigned long long)now);
#endif

        /* Simulate CPU-intensive work */
        k_busy_wait(cfg->exec_time_ms * 1000);  // Use exec time from config

        /* Record end time */
        uint64_t end = k_uptime_get();

        /* Calculate response time in microseconds */
        uint32_t response_time_us = (uint32_t)((end - release_time) * 1000);
        
        /* Update statistics */
        stats->total_response_time += response_time_us;
        stats->sum_response_time_squared += (uint64_t)response_time_us * response_time_us;

        if (stats->activations == 1) {
            stats->min_response_time = response_time_us;
            stats->max_response_time = response_time_us;
        } else {
            if (response_time_us < stats->min_response_time)
                stats->min_response_time = response_time_us;
            if (response_time_us > stats->max_response_time)
                stats->max_response_time = response_time_us;
        }

        /* Check for deadline miss */
        bool deadline_met = (end <= abs_deadline);
        int64_t lateness = deadline_met ? 0 : (int32_t)(end - abs_deadline);

        if (!deadline_met) {
            stats->deadline_misses++;
#if DEBUG_STATEMENTS
            printk("[Task%u] *** DEADLINE MISS *** Lateness: %d ms\n", task_id, (int)lateness);
#endif
        }

#if CSV_OUTPUT
        /* CSV format: timestamp,task_id,activation,response_time,deadline_met,lateness,period,deadline,weight */
        printk("CSV,%llu,%u,%u,%u,%d,%d,%u,%u,%u\n",
               now, task_id, stats->activations, response_time_us,
               deadline_met ? 1 : 0, (int)lateness,
               cfg->period_ms, cfg->deadline_ms, weight);
#endif

#if DEBUG_STATEMENTS
    if (stats->activations % 10 == 0) {
        uint32_t avg_response = stats->total_response_time / stats->activations;
        printk("[Task%u] Stats after %u activations:\n", task_id, stats->activations);
        printk("  Min Response Time: %u us\n", stats->min_response_time);
        printk("  Max Response Time: %u us\n", stats->max_response_time);
        printk("  Avg Response Time: %u us\n", avg_response);
        printk("  Deadline Misses: %u (%.1f%%)\n", stats->deadline_misses,
                100.0 * stats->deadline_misses / stats->activations);
        printk("=========================================\n");
    }
#endif

        /* Move to next period */
        stats->next_release += cfg->period_ms;
    }
}

/**
 * Main entry point
 */
int main(void)
{
    printk("\n");
    printk("===============================================\n");
    printk("  Simple Real-Time Task Evaluation - Step 2   \n");
    printk("===============================================\n\n");
    printk("Workload: %s\n", workload_name);
    printk("Configuration:\n");

    // Calculate total CPU utilization
    float total_utilization = 0.0f;
    for (int i = 0; i < NUM_TASKS; i++) {
        const task_config_t *cfg = &task_configs[i];
        float util = cfg->exec_time_ms / (float)cfg->period_ms;
        total_utilization += util;

        printk("  %s: P=%ums E=%ums D=%ums W=%u (%.1f%%)\n",
               cfg->name, cfg->period_ms, cfg->exec_time_ms,
               cfg->deadline_ms, cfg->weight, 100.0f * (double)util);
    }

    printk("Total CPU Utilization: %.1f%%\n\n", 100.0f * (double)total_utilization);
    printk("First synchronized release at %u ms\n", FIRST_RELEASE_MS);
    printk("Test duration: %u ms\n\n", TEST_DURATION_MS);
    printk("===============================================\n\n");

#if CSV_OUTPUT
    // print CSV header
    printk("CSV_HEADER,timestamp,task_id,activation,response_time,deadline_met,lateness,period,deadline,weight\n");
#endif

    for (int i = 0; i < NUM_TASKS; i++) {
        // Initialize task context
        task_contexts[i].task_id = i + 1;
        task_contexts[i].config = &task_configs[i];
        task_contexts[i].stats = &task_stats[i];

        // initialize stats structure to zero
        memset(&task_stats[i], 0, sizeof(task_stats_t));

        // Create task thread
        k_thread_create(&task_threads[i], task_stacks[i], STACK_SIZE,
                        periodic_task, &task_contexts[i], NULL, NULL,
                        PRIORITY, 0, K_NO_WAIT);
    }

    printk("All %u tasks created. Waiting for first release at %u ms and running for %u ms\n\n",
           NUM_TASKS, FIRST_RELEASE_MS, TEST_DURATION_MS);

    k_sleep(K_MSEC(TEST_DURATION_MS));
    terminate_tasks = true;

    printk("Test duration complete. Terminating tasks...\n");
    k_sleep(K_MSEC(1000));

    for(int i=0; i < NUM_TASKS; i++) {
        k_thread_join(&task_threads[i], K_FOREVER);
    }

    return 0;
}