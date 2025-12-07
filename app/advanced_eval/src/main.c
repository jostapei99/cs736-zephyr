#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "workloads.h"
#include "metrics.h"

#define DEBUG_STATEMENTS 0 /* Set to 1 to enable debug prints */

#define STACK_SIZE 2048
#define PRIORITY   5

/* Synchronization: all tasks start together */
#define FIRST_RELEASE_MS   500

/* Maximum activations before stopping (0 = run forever) */
#define MAX_ACTIVATIONS 100

/* Define workload configurations */
#if CURRENT_WORKLOAD == WORKLOAD_LIGHT
task_config_t task_configs[NUM_TASKS] = {
    {.name = "Task1", .period_ms = 100, .exec_time_ms = 20, .deadline_ms = 100, .weight = 1},
    {.name = "Task2", .period_ms = 200, .exec_time_ms = 30, .deadline_ms = 200, .weight = 1},
    {.name = "Task3", .period_ms = 300, .exec_time_ms = 40, .deadline_ms = 300, .weight = 1},
    {.name = "Task4", .period_ms = 500, .exec_time_ms = 50, .deadline_ms = 500, .weight = 1},
};
const char *workload_name = "Light Load (~50%)";

#elif CURRENT_WORKLOAD == WORKLOAD_MEDIUM
task_config_t task_configs[NUM_TASKS] = {
    {.name = "Task1", .period_ms = 100, .exec_time_ms = 30, .deadline_ms = 100, .weight = 1},
    {.name = "Task2", .period_ms = 200, .exec_time_ms = 50, .deadline_ms = 200, .weight = 1},
    {.name = "Task3", .period_ms = 300, .exec_time_ms = 60, .deadline_ms = 300, .weight = 1},
    {.name = "Task4", .period_ms = 500, .exec_time_ms = 70, .deadline_ms = 500, .weight = 1},
};
const char *workload_name = "Medium Load (~70%)";

#elif CURRENT_WORKLOAD == WORKLOAD_HEAVY
task_config_t task_configs[NUM_TASKS] = {
    {.name = "Task1", .period_ms = 100, .exec_time_ms = 40, .deadline_ms = 100, .weight = 1},
    {.name = "Task2", .period_ms = 200, .exec_time_ms = 70, .deadline_ms = 200, .weight = 1},
    {.name = "Task3", .period_ms = 300, .exec_time_ms = 80, .deadline_ms = 300, .weight = 1},
    {.name = "Task4", .period_ms = 500, .exec_time_ms = 90, .deadline_ms = 500, .weight = 1},
};
const char *workload_name = "Heavy Load (~90%)";

#elif CURRENT_WORKLOAD == WORKLOAD_OVERLOAD
task_config_t task_configs[NUM_TASKS] = {
    {.name = "Task1", .period_ms = 100, .exec_time_ms = 50, .deadline_ms = 100, .weight = 1},
    {.name = "Task2", .period_ms = 200, .exec_time_ms = 80, .deadline_ms = 200, .weight = 1},
    {.name = "Task3", .period_ms = 300, .exec_time_ms = 90, .deadline_ms = 300, .weight = 1},
    {.name = "Task4", .period_ms = 500, .exec_time_ms = 100, .deadline_ms = 500, .weight = 1},
};
const char *workload_name = "Overload (~110%)";

#elif CURRENT_WORKLOAD == WORKLOAD_CUSTOM
task_config_t task_configs[NUM_TASKS] = {
    {.name = "Task1", .period_ms = 100, .exec_time_ms = 20, .deadline_ms = 100, .weight = 1},
    {.name = "Task2", .period_ms = 200, .exec_time_ms = 30, .deadline_ms = 200, .weight = 1},
    {.name = "Task3", .period_ms = 300, .exec_time_ms = 40, .deadline_ms = 300, .weight = 1},
    {.name = "Task4", .period_ms = 500, .exec_time_ms = 50, .deadline_ms = 500, .weight = 1},
};
const char *workload_name = "Custom Workload";
#endif

/* Thread storage - array for multiple tasks */
K_THREAD_STACK_ARRAY_DEFINE(task_stacks, NUM_TASKS, STACK_SIZE);
struct k_thread task_threads[NUM_TASKS];

/* Task context - passed to each task instance */
typedef struct {
    uint32_t task_id;
    task_config_t *config;  /* Non-const to allow runtime modification */
    task_stats_t *stats;
} task_context_t;

/* Arrays for all tasks */
task_stats_t task_stats[NUM_TASKS] = {0};
task_context_t task_contexts[NUM_TASKS];

/* Global control flag */
bool tasks_running = true;

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
 * Generic periodic real-time task with enhanced metrics
 */
void periodic_task(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);
    
    task_context_t *ctx = (task_context_t *)arg1;
    task_config_t *cfg = ctx->config;
    task_stats_t *stats = ctx->stats;
    uint32_t task_id = ctx->task_id;
    
    /* Set thread properties */
    k_thread_weight_set(k_current_get(), cfg->weight);
    init_release(stats, cfg->period_ms);

    while (tasks_running) {
        uint64_t now = k_uptime_get();
        uint64_t release_time = stats->next_release;
        uint64_t abs_deadline = release_time + cfg->deadline_ms;
        k_thread_deadline_set(k_current_get(), k_ms_to_cyc_ceil32(abs_deadline));

        /* Wait until release time */
        if (now < release_time) {
            k_sleep(K_MSEC(release_time - now));
            now = k_uptime_get();
        }

        /* Check if we've reached max activations */
        if (MAX_ACTIVATIONS > 0 && stats->activations >= MAX_ACTIVATIONS) {
            return;  /* Stop this task */
        }

#if DEBUG_STATEMENTS
        printk("[Task%u] Activation %u at %llu ms\n",
               task_id, stats->activations + 1, (unsigned long long)now);
#endif

        /* Record start of execution */
        uint64_t exec_start = k_uptime_get();
        
        /* Simulate CPU-intensive work */
        k_busy_wait(cfg->exec_time_ms * 1000);

        /* Record end times */
        uint64_t exec_end = k_uptime_get();
        uint64_t end = exec_end;
        
        uint32_t exec_time = (uint32_t)(exec_end - exec_start);
        uint32_t response_time = (uint32_t)(end - now);

        /* Check deadline */
        bool deadline_met = (end <= abs_deadline);
        int32_t lateness = deadline_met ? 0 : (int32_t)(end - abs_deadline);
        
        /* Update metrics */
        metrics_update(stats, response_time, exec_time, deadline_met, lateness);
        
        /* Calculate jitter periodically */
        if (stats->activations % 10 == 0) {
            metrics_calculate_jitter(stats);
        }

#if DEBUG_STATEMENTS
        if (!deadline_met) {
            printk("[Task%u] *** DEADLINE MISS *** (lateness: %d ms)\n",
                   task_id, lateness);
        }
        printk("[Task%u] Response time: %u ms, Exec time: %u ms\n",
               task_id, response_time, exec_time);
#endif

        /* Output based on format */
        if (output_format == OUTPUT_CSV) {
            metrics_print_csv_record(now, task_id, stats, response_time,
                                    deadline_met, lateness, cfg);
        } else if (output_format == OUTPUT_JSON) {
            metrics_print_json_record(now, task_id, stats, response_time,
                                     deadline_met, lateness, cfg);
        }

        /* Print summary every 20 activations */
        if (output_format == OUTPUT_HUMAN && stats->activations % 20 == 0) {
            metrics_calculate_jitter(stats);
            metrics_print_task_summary(task_id, cfg, stats);
        }

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
    printk("╔════════════════════════════════════════════════════════════╗\n");
    printk("║  Advanced RT Scheduler Evaluation                         ║\n");
    printk("╔════════════════════════════════════════════════════════════╗\n");
    printk("║  Workload: %-47s ║\n", workload_name);
    printk("╠════════════════════════════════════════════════════════════╣\n");
    printk("║  Configuration:                                            ║\n");
    
    /* Initialize metrics */
    metrics_init();
    
    /* Calculate total CPU utilization */
    float total_util = 0.0f;
    for (int i = 0; i < NUM_TASKS; i++) {
        const task_config_t *cfg = &task_configs[i];
        float task_util = cfg->exec_time_ms / (float)cfg->period_ms;
        total_util += task_util;
        
        printk("║  %s: P=%3ums E=%3ums D=%3ums W=%u (Util=%5.1f%%)      ║\n",
               cfg->name, cfg->period_ms, cfg->exec_time_ms,
               cfg->deadline_ms, cfg->weight, 100.0f * task_util);
    }
    
    printk("╠════════════════════════════════════════════════════════════╣\n");
    printk("║  First Release: %4dms (synchronized)                      ║\n", 
           FIRST_RELEASE_MS);
    printk("║  Total CPU Utilization: %6.1f%%                            ║\n", 
           100.0f * total_util);
    if (MAX_ACTIVATIONS > 0) {
        printk("║  Max Activations per Task: %u                              ║\n", 
               MAX_ACTIVATIONS);
    }
    printk("║  Output Format: %-43s ║\n",
           output_format == OUTPUT_CSV ? "CSV" :
           output_format == OUTPUT_JSON ? "JSON" :
           output_format == OUTPUT_HUMAN ? "Human-readable" : "Quiet");
    printk("╚════════════════════════════════════════════════════════════╝\n");
    printk("\n");
    
    printk("Shell Commands Available:\n");
    printk("  rt show    - Display current configuration\n");
    printk("  rt stats   - Show runtime statistics\n");
    printk("  rt format  - Change output format (csv|json|human|quiet)\n");
    printk("  rt set     - Modify task parameters at runtime\n");
    printk("  rt reset   - Reset statistics\n");
    printk("  rt util    - Show utilization analysis\n");
    printk("\n");

    /* Print output header */
    if (output_format == OUTPUT_CSV) {
        metrics_print_csv_header();
    }

    /* Initialize task contexts and create all tasks */
    for (int i = 0; i < NUM_TASKS; i++) {
        task_contexts[i].task_id = i + 1;  /* 1-based task IDs for display */
        task_contexts[i].config = &task_configs[i];
        task_contexts[i].stats = &task_stats[i];
        
        /* Initialize stats structure to zero */
        memset(&task_stats[i], 0, sizeof(task_stats_t));
        
        /* Create the task thread with name */
        k_tid_t tid = k_thread_create(&task_threads[i], task_stacks[i], STACK_SIZE,
                        periodic_task, &task_contexts[i], NULL, NULL,
                        PRIORITY, 0, K_NO_WAIT);
        
        /* Set thread name for debugging */
        k_thread_name_set(tid, task_configs[i].name);
    }

    printk("All %u tasks created. Waiting for first release at %dms...\n\n", 
           NUM_TASKS, FIRST_RELEASE_MS);

    /* Main thread becomes shell - don't sleep forever */
    return 0;
}
