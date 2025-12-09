#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "../../../app/simple_eval_step1/include/workloads.h"

#define DEBUG_STATEMENTS 0 /* Set to 1 to enable debug prints */
#define CSV_OUTPUT 1       /* Set to 1 to enable CSV data export */
#define USE_KERNEL_STATS 1 /* Use kernel statistics instead of manual tracking */

#define STACK_SIZE 2048
#define PRIORITY   5

/* Synchronization: all tasks start together */
#define FIRST_RELEASE_MS   500

/* Maximum activations before stopping (0 = run forever) */
#define MAX_ACTIVATIONS 50

/* Thread storage - array for multiple tasks */
K_THREAD_STACK_ARRAY_DEFINE(task_stacks, NUM_TASKS, STACK_SIZE);
struct k_thread task_threads[NUM_TASKS];

/* Task context - passed to each task instance */
typedef struct {
    uint32_t task_id;
    const task_config_t *config;
    uint64_t next_release;
    uint32_t activations;
} task_context_t;

/* Arrays for all tasks */
task_context_t task_contexts[NUM_TASKS];

/**
 * Initialize task release time - synchronized across all tasks
 */
static inline void init_release(task_context_t *ctx)
{
    if (ctx->next_release == 0) {
        ctx->next_release = FIRST_RELEASE_MS;  /* All tasks start at same time */
    }
}

/**
 * Generic periodic real-time task with kernel statistics
 */
void periodic_task_with_stats(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);
    
    task_context_t *ctx = (task_context_t *)arg1;
    const task_config_t *cfg = ctx->config;
    uint32_t task_id = ctx->task_id;
    k_tid_t self = k_current_get();
    
    /* Set thread properties */
    k_thread_weight_set(self, cfg->weight);
    init_release(ctx);

#ifdef CONFIG_736_RT_STATS
    /* Reset statistics at start */
    k_thread_rt_stats_reset(self);
#endif

    while (1) {
        uint64_t now = k_uptime_get();
        uint64_t release_time = ctx->next_release;
        uint64_t abs_deadline = release_time + cfg->deadline_ms;
        k_thread_deadline_set(self, k_ms_to_cyc_ceil32(abs_deadline));

        /* Wait until release time */
        if (now < release_time) {
            k_sleep(K_MSEC(release_time - now));
            now = k_uptime_get();
        }

        ctx->activations++;
        
#ifdef CONFIG_736_RT_STATS
        /* Record activation in kernel statistics */
        k_thread_rt_stats_activation(self);
#endif
        
        /* Check if we've reached max activations */
        if (MAX_ACTIVATIONS > 0 && ctx->activations > MAX_ACTIVATIONS) {
            /* Print final statistics */
#ifdef CONFIG_736_RT_STATS
            struct k_thread_rt_stats stats;
            k_thread_rt_stats_get(self, &stats);
            
            printk("\n╔══════════════════════════════════════════════════╗\n");
            printk("║  Final Statistics for Task%u (%s)\n", task_id, cfg->name);
            printk("╠══════════════════════════════════════════════════╣\n");
            printk("║  Activations:        %6u\n", stats.activations);
            printk("║  Deadline Misses:    %6u (%.1f%%)\n", 
                   stats.deadline_misses,
                   k_thread_rt_stats_miss_ratio(&stats));
            printk("║  Context Switches:   %6u\n", stats.context_switches);
            printk("║  Preemptions:        %6u\n", stats.preemptions);
            printk("╠══════════════════════════════════════════════════╣\n");
            printk("║  Response Time (ms):\n");
            printk("║    Min:              %6u\n", stats.min_response_time);
            printk("║    Max:              %6u\n", stats.max_response_time);
            printk("║    Avg:              %6u\n", k_thread_rt_stats_avg_response(&stats));
#ifdef CONFIG_736_RT_STATS_SQUARED
            printk("║    Std Dev:          %6u\n", k_thread_rt_stats_response_stddev(&stats));
            printk("║    Jitter:           %6u\n", k_thread_rt_stats_response_jitter(&stats));
#endif
            printk("╚══════════════════════════════════════════════════╝\n");
#endif /* CONFIG_736_RT_STATS */
            return;  /* Stop this task */
        }

#if DEBUG_STATEMENTS
        printk("[Task%u] Activation %u at %llu ms\n",
               task_id, ctx->activations, (unsigned long long)now);
#endif

        /* Simulate CPU-intensive work */
        uint64_t work_start = k_uptime_get();
        k_busy_wait(cfg->exec_time_ms * 1000);
        uint64_t work_end = k_uptime_get();

        uint64_t end = k_uptime_get();
        uint32_t response_time = (uint32_t)(end - now);
        uint32_t actual_exec = (uint32_t)(work_end - work_start);

        /* Check deadline */
        bool deadline_met = (end <= abs_deadline);
        int32_t lateness = deadline_met ? 0 : (int32_t)(end - abs_deadline);
        
        if (!deadline_met) {
#ifdef CONFIG_736_RT_STATS
            k_thread_rt_stats_deadline_miss(self);
#endif
#if DEBUG_STATEMENTS
            printk("[Task%u] *** DEADLINE MISS *** (lateness: %d ms)\n",
                   task_id, lateness);
#endif
        }

#if CSV_OUTPUT
        /* CSV format: timestamp,task_id,activation,response_time,actual_exec,deadline_met,lateness,period,deadline,weight */
        printk("CSV,%llu,%u,%u,%u,%u,%d,%d,%u,%u,%u\n",
               now, task_id, ctx->activations, response_time, actual_exec,
               deadline_met ? 1 : 0, lateness,
               cfg->period_ms, cfg->deadline_ms, cfg->weight);
#endif

#if DEBUG_STATEMENTS
        printk("[Task%u] Response time: %u ms, Deadline: %s\n",
               task_id, response_time, deadline_met ? "MET" : "MISSED");
#endif

        /* Print summary every 10 activations */
        if (ctx->activations % 10 == 0) {
#ifdef CONFIG_736_RT_STATS
            struct k_thread_rt_stats stats;
            k_thread_rt_stats_get(self, &stats);
            
            printk("\n=== Task%u Stats after %u activations ===\n", task_id, ctx->activations);
            printk("Avg Response Time: %u ms\n", k_thread_rt_stats_avg_response(&stats));
            printk("Deadline Misses: %u (%.1f%%)\n", 
                   stats.deadline_misses,
                   k_thread_rt_stats_miss_ratio(&stats));
            printk("Context Switches: %u\n", stats.context_switches);
            printk("Preemptions: %u\n", stats.preemptions);
            printk("=========================================\n\n");
#endif
        }

        /* Move to next period */
        ctx->next_release += cfg->period_ms;
    }
}

/**
 * Main entry point
 */
int main(void)
{
    printk("\n");
    printk("═══════════════════════════════════════════════════\n");
    printk("  Real-Time Scheduler Evaluation with Statistics\n");
    printk("═══════════════════════════════════════════════════\n");
    printk("Workload: %s\n", workload_name);
#ifdef CONFIG_736_RT_STATS
    printk("Kernel Statistics: ENABLED\n");
#ifdef CONFIG_736_RT_STATS_DETAILED
    printk("  - Detailed timestamps: YES\n");
#endif
#ifdef CONFIG_736_RT_STATS_SQUARED
    printk("  - Variance/jitter: YES\n");
#endif
#else
    printk("Kernel Statistics: DISABLED (using manual tracking)\n");
#endif
    printk("Configuration:\n");
    
    /* Calculate total CPU utilization */
    float total_util = 0.0f;
    for (int i = 0; i < NUM_TASKS; i++) {
        const task_config_t *cfg = &task_configs[i];
        float task_util = cfg->exec_time_ms / (float)cfg->period_ms;
        total_util += task_util;
        
        printk("  %s: Period=%ums, Exec=%ums, Deadline=%ums, Weight=%u (Util=%.1f%%)\n",
               cfg->name, cfg->period_ms, cfg->exec_time_ms,
               cfg->deadline_ms, cfg->weight, 100.0f * task_util);
    }
    
    printk("  First Release: %dms (synchronized)\n", FIRST_RELEASE_MS);
    printk("  Total CPU Utilization: %.1f%%\n", 100.0f * total_util);
    if (MAX_ACTIVATIONS > 0) {
        printk("  Max Activations per Task: %u\n", MAX_ACTIVATIONS);
    }
    printk("═══════════════════════════════════════════════════\n\n");

#if CSV_OUTPUT
    /* Print CSV header */
    printk("CSV_HEADER,timestamp,task_id,activation,response_time,actual_exec,deadline_met,lateness,period,deadline,weight\n");
#endif

    /* Initialize task contexts and create all tasks */
    for (int i = 0; i < NUM_TASKS; i++) {
        task_contexts[i].task_id = i + 1;  /* 1-based task IDs for display */
        task_contexts[i].config = &task_configs[i];
        task_contexts[i].next_release = 0;
        task_contexts[i].activations = 0;
        
        /* Create the task thread */
        k_thread_create(&task_threads[i], task_stacks[i], STACK_SIZE,
                        periodic_task_with_stats, &task_contexts[i], NULL, NULL,
                        PRIORITY, 0, K_NO_WAIT);
    }

    printk("All %u tasks created. Waiting for first release at %dms...\n\n", 
           NUM_TASKS, FIRST_RELEASE_MS);

    /* Keep main alive */
    while (1) {
        k_sleep(K_FOREVER);
    }
    
    return 0;
}
