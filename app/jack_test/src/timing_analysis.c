/*
 * Timing Analysis and Scheduler Monitoring
 * 
 * This module provides detailed timing analysis and scheduler state monitoring
 * for mission-critical applications.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/timing/timing.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel_structs.h>

LOG_MODULE_REGISTER(timing_analysis, LOG_LEVEL_DBG);

// Timing analysis structures 
struct task_timing_stats {
    const char *task_name;
    struct k_thread *thread;
    uint64_t total_runtime_ns;
    uint64_t max_execution_ns;
    uint64_t min_execution_ns;
    uint32_t context_switches;
    uint32_t deadline_misses;
    uint32_t execution_count;
};

// Timing analysis configuration
#define MAX_MONITORED_TASKS 10
#define ANALYSIS_INTERVAL_MS 5000
#define DEADLINE_THRESHOLD_NS 50000000  // 50ms

static struct task_timing_stats task_stats[MAX_MONITORED_TASKS];
static uint32_t monitored_task_count = 0;
static timing_t last_analysis_time;
static uint64_t total_context_switches = 0;

// External variables
extern volatile bool simulation_running;
extern struct k_thread mission_control_tid;
extern struct k_thread navigation_tid;
extern struct k_thread communication_tid;
extern struct k_thread housekeeping_tid;
extern struct k_thread emergency_tid;
extern struct k_thread safety_monitor_tid;
extern struct k_thread fault_detector_tid;

// Function prototypes
void timing_analysis_work_handler(struct k_work *work);
void timing_analysis_timer_handler(struct k_timer *timer);
void analyze_memory_usage(void);
void analyze_thread_states(void);

// Analysis timer and work queue
K_WORK_DEFINE(timing_analysis_work, timing_analysis_work_handler);
K_TIMER_DEFINE(analysis_timer, timing_analysis_timer_handler, NULL);

// Context switch hook for detailed monitoring
static timing_t last_context_switch_time;
static struct k_thread *last_running_thread = NULL;

void timing_analysis_timer_handler(struct k_timer *timer)
{
    ARG_UNUSED(timer);
    k_work_submit(&timing_analysis_work);
}

void timing_analysis_work_handler(struct k_work *work)
{
    ARG_UNUSED(work);

    /* Don't run analysis if simulation is not running */
    if (!simulation_running) {
        return;
    }

    timing_t current_time = timing_counter_get();
    uint64_t analysis_interval_ns = timing_cycles_to_ns(current_time - last_analysis_time);

    LOG_INF("=== TIMING ANALYSIS REPORT ===");
    LOG_INF("Analysis interval: %llu ms", analysis_interval_ns / 1000000);

    // Print detailed statistics for each monitored task
    for (uint32_t i = 0; i < monitored_task_count; i++) 
    {
        struct task_timing_stats *stats = &task_stats[i];

        /* Safety checks */
        if (!stats || !stats->thread) {
            continue;
        }

        if( stats->execution_count > 0 ) {
            uint64_t avg_execution_ns = stats->total_runtime_ns / stats->execution_count;
            
            LOG_INF("Task %u:", i);
            LOG_INF("  Executions: %u", stats->execution_count);
            LOG_INF("  Avg execution: %llu µs", avg_execution_ns / 1000);
            LOG_INF("  Max execution: %llu µs", stats->max_execution_ns / 1000);
            LOG_INF("  Min execution: %llu µs", stats->min_execution_ns / 1000);
            LOG_INF("  Context switches: %u", stats->context_switches);
            LOG_INF("  Deadline misses: %u", stats->deadline_misses);
            LOG_INF("  Priority: %d", k_thread_priority_get(stats->thread));
        }
    }

    // System-wide scheduler statistics
    LOG_INF("=== SCHEDULER STATISTICS ===");
    LOG_INF("Total context switches: %llu", total_context_switches);
    LOG_INF("Current thread: %p", k_current_get());
    LOG_INF("System uptime: %u ms", k_uptime_get_32());

    // memory usage analysis
    analyze_memory_usage();

    // thread state analysis
    analyze_thread_states();

    last_analysis_time = current_time;
}

void analyze_memory_usage(void)
{
    LOG_INF("=== MEMORY USAGE ANALYSIS ===");
    
    /* Basic system info only - avoid all memory access that could cause crashes */
    LOG_INF("System memory analysis (safe mode)");
    
    if (monitored_task_count == 0) {
        LOG_WRN("No monitored tasks for memory analysis");
        return;
    }
    
    LOG_INF("Memory analysis for %u monitored tasks (basic info only)", monitored_task_count);
    
    /* Report basic task count and avoid all stack analysis to prevent crashes */
    for (uint32_t i = 0; i < monitored_task_count; i++) {
        struct task_timing_stats *stats = &task_stats[i];
        
        /* Minimal safety checks */
        if (!stats) {
            LOG_WRN("NULL stats at index %u", i);
            continue;
        }
        
        /* Only log basic task information without accessing thread memory */
        LOG_INF("Task %u: monitored (thread ptr: %p)", i, stats->thread);
    }
    
    LOG_INF("Memory analysis complete (safe mode - no memory inspection)");
}

void analyze_thread_states(void)
{
    LOG_INF("=== THREAD STATE ANALYSIS ===");
    
    for (uint32_t i = 0; i < monitored_task_count; i++) {
        struct task_timing_stats *stats = &task_stats[i];
        
        /* Safety checks */
        if (!stats || !stats->thread) {
            LOG_WRN("Invalid task stats at index %u in thread state analysis", i);
            continue;
        }
        
        /* Use only safe API calls, avoid direct memory access */
        int priority = k_thread_priority_get(stats->thread);
        
        /* Report basic thread information without accessing internal state */
        LOG_INF("Thread %u: Priority=%d (safe mode analysis)", i, priority);
    }
}

static void add_monitored_task(const char *name, struct k_thread *thread)
{
    if (monitored_task_count < MAX_MONITORED_TASKS) {
        struct task_timing_stats *stats = &task_stats[monitored_task_count];
        
        stats->task_name = name;
        stats->thread = thread;
        stats->total_runtime_ns = 0;
        stats->max_execution_ns = 0;
        stats->min_execution_ns = UINT64_MAX;
        stats->context_switches = 0;
        stats->deadline_misses = 0;
        stats->execution_count = 0;
        
        monitored_task_count++;
        
        LOG_INF("Added task '%s' to timing analysis", name);
    } else {
        LOG_WRN("Cannot add task '%s' - maximum monitored tasks reached", name);
    }
}

void record_task_execution(struct k_thread *thread, uint64_t execution_time_ns)
{
    for (uint32_t i = 0; i < monitored_task_count; i++) {
        struct task_timing_stats *stats = &task_stats[i];
        
        if (stats->thread == thread) {
            stats->total_runtime_ns += execution_time_ns;
            stats->execution_count++;
            
            if (execution_time_ns > stats->max_execution_ns) {
                stats->max_execution_ns = execution_time_ns;
            }
            
            if (execution_time_ns < stats->min_execution_ns) {
                stats->min_execution_ns = execution_time_ns;
            }
            
            if (execution_time_ns > DEADLINE_THRESHOLD_NS) {
                stats->deadline_misses++;
                LOG_WRN("Deadline miss detected for %s: %llu µs", 
                        stats->task_name, execution_time_ns / 1000);
            }
            
            break;
        }
    }
}

void record_context_switch(struct k_thread *prev_thread, struct k_thread *next_thread)
{
    ARG_UNUSED(prev_thread);
    
    total_context_switches++;
    
    /* Update context switch count for the incoming thread */
    for (uint32_t i = 0; i < monitored_task_count; i++) {
        if (task_stats[i].thread == next_thread) {
            task_stats[i].context_switches++;
            break;
        }
    }
    
    timing_t current_time = timing_counter_get();
    
    /* Calculate execution time for the previous thread */
    if (last_running_thread != NULL && last_context_switch_time != 0) {
        uint64_t execution_time = timing_cycles_to_ns(current_time - last_context_switch_time);
        record_task_execution(last_running_thread, execution_time);
    }
    
    last_context_switch_time = current_time;
    last_running_thread = next_thread;
    
    LOG_DBG("Context switch: %p -> %p (total: %llu)", 
            prev_thread, next_thread, total_context_switches);
}

void start_timing_analysis(void)
{
    LOG_INF("Starting comprehensive timing analysis...");
    
    /* Wait for threads to initialize */
    k_sleep(K_MSEC(1000));
    
    /* Initialize timing system */
    last_analysis_time = timing_counter_get();
    last_context_switch_time = last_analysis_time;
    
    /* Add all critical tasks to monitoring */
    add_monitored_task("mission_control", &mission_control_tid);
    add_monitored_task("navigation", &navigation_tid);
    add_monitored_task("communication", &communication_tid);
    add_monitored_task("housekeeping", &housekeeping_tid);
    add_monitored_task("emergency", &emergency_tid);
    add_monitored_task("safety_monitor", &safety_monitor_tid);
    add_monitored_task("fault_detector", &fault_detector_tid);
    
    /* Start periodic analysis timer with initial delay */
    k_timer_start(&analysis_timer, K_MSEC(ANALYSIS_INTERVAL_MS + 1000), K_MSEC(ANALYSIS_INTERVAL_MS));
    
    LOG_INF("Timing analysis started - reporting every %d ms", ANALYSIS_INTERVAL_MS);
    LOG_INF("Monitoring %u tasks with deadline threshold: %u µs", 
            monitored_task_count, DEADLINE_THRESHOLD_NS / 1000);
}