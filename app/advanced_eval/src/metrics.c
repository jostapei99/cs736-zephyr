#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <math.h>
#include "metrics.h"

/* Global metrics */
system_metrics_t system_metrics = {0};
output_format_t output_format = OUTPUT_CSV;

/**
 * Initialize metrics subsystem
 */
void metrics_init(void)
{
    system_metrics.system_start_time = k_uptime_get();
    system_metrics.total_runtime = 0;
    system_metrics.total_activations = 0;
    system_metrics.total_deadline_misses = 0;
    system_metrics.system_utilization = 0.0;
    system_metrics.total_preemptions = 0;
}

/**
 * Update task statistics
 */
void metrics_update(task_stats_t *stats, uint32_t response_time, 
                   uint32_t exec_time, bool deadline_met, uint32_t lateness)
{
    stats->activations++;
    
    /* Response time stats */
    stats->total_response_time += response_time;
    stats->sum_response_time_squared += (uint64_t)response_time * response_time;
    
    if (stats->activations == 1) {
        stats->min_response_time = response_time;
        stats->max_response_time = response_time;
    } else {
        if (response_time < stats->min_response_time)
            stats->min_response_time = response_time;
        if (response_time > stats->max_response_time)
            stats->max_response_time = response_time;
    }
    
    /* Execution time stats */
    stats->total_exec_time += exec_time;
    if (stats->activations == 1) {
        stats->min_exec_time = exec_time;
        stats->max_exec_time = exec_time;
    } else {
        if (exec_time < stats->min_exec_time)
            stats->min_exec_time = exec_time;
        if (exec_time > stats->max_exec_time)
            stats->max_exec_time = exec_time;
    }
    
    /* Deadline tracking */
    if (!deadline_met) {
        stats->deadline_misses++;
        stats->total_lateness += lateness;
        if (lateness > stats->max_lateness)
            stats->max_lateness = lateness;
        
        system_metrics.total_deadline_misses++;
    }
    
    /* System-wide */
    system_metrics.total_activations++;
}

/**
 * Calculate jitter metrics (variance and standard deviation)
 */
void metrics_calculate_jitter(task_stats_t *stats)
{
    if (stats->activations < 2)
        return;
    
    double mean = (double)stats->total_response_time / stats->activations;
    double sum_sq = (double)stats->sum_response_time_squared;
    
    /* Variance = E[X^2] - (E[X])^2 */
    stats->response_time_variance = (sum_sq / stats->activations) - (mean * mean);
    
    /* Standard deviation = sqrt(variance) */
    if (stats->response_time_variance > 0) {
        stats->response_time_std_dev = sqrt(stats->response_time_variance);
    } else {
        stats->response_time_std_dev = 0.0;
    }
}

/**
 * Print task summary in human-readable format
 */
void metrics_print_task_summary(uint32_t task_id, const task_config_t *cfg, 
                               const task_stats_t *stats)
{
    if (output_format == OUTPUT_QUIET)
        return;
    
    if (stats->activations == 0)
        return;
    
    uint32_t avg_response = stats->total_response_time / stats->activations;
    uint32_t avg_exec = stats->total_exec_time / stats->activations;
    double miss_rate = 100.0 * stats->deadline_misses / stats->activations;
    
    printk("\n╔════════════════════════════════════════════════════════════╗\n");
    printk("║  %s Summary (Task ID: %u)                              \n", cfg->name, task_id);
    printk("╠════════════════════════════════════════════════════════════╣\n");
    printk("║  Configuration:\n");
    printk("║    Period:      %u ms\n", cfg->period_ms);
    printk("║    Exec Time:   %u ms (target)\n", cfg->exec_time_ms);
    printk("║    Deadline:    %u ms\n", cfg->deadline_ms);
    printk("║    Weight:      %u\n", cfg->weight);
    printk("║\n");
    printk("║  Execution Statistics:\n");
    printk("║    Activations: %u\n", stats->activations);
    printk("║    Avg Exec:    %u ms (%.1f%% of target)\n", 
           avg_exec, 100.0 * avg_exec / cfg->exec_time_ms);
    printk("║    Min/Max:     %u / %u ms\n", 
           stats->min_exec_time, stats->max_exec_time);
    printk("║\n");
    printk("║  Response Time:\n");
    printk("║    Average:     %u ms\n", avg_response);
    printk("║    Min/Max:     %u / %u ms\n", 
           stats->min_response_time, stats->max_response_time);
    printk("║    Std Dev:     %.2f ms (jitter)\n", stats->response_time_std_dev);
    printk("║    Variance:    %.2f ms²\n", stats->response_time_variance);
    printk("║\n");
    printk("║  Deadline Performance:\n");
    printk("║    Misses:      %u / %u (%.2f%%)\n", 
           stats->deadline_misses, stats->activations, miss_rate);
    
    if (stats->deadline_misses > 0) {
        uint32_t avg_lateness = stats->total_lateness / stats->deadline_misses;
        printk("║    Avg Lateness: %u ms\n", avg_lateness);
        printk("║    Max Lateness: %u ms\n", stats->max_lateness);
    }
    
    printk("╚════════════════════════════════════════════════════════════╝\n");
}

/**
 * Print system-wide summary
 */
void metrics_print_system_summary(void)
{
    if (output_format == OUTPUT_QUIET)
        return;
    
    uint64_t now = k_uptime_get();
    system_metrics.total_runtime = now - system_metrics.system_start_time;
    
    double miss_rate = 0.0;
    if (system_metrics.total_activations > 0) {
        miss_rate = 100.0 * system_metrics.total_deadline_misses / 
                    system_metrics.total_activations;
    }
    
    printk("\n");
    printk("╔════════════════════════════════════════════════════════════╗\n");
    printk("║  SYSTEM SUMMARY                                            ║\n");
    printk("╠════════════════════════════════════════════════════════════╣\n");
    printk("║  Runtime:          %llu ms\n", system_metrics.total_runtime);
    printk("║  Total Activations: %u\n", system_metrics.total_activations);
    printk("║  Deadline Misses:   %u (%.2f%%)\n", 
           system_metrics.total_deadline_misses, miss_rate);
    printk("╚════════════════════════════════════════════════════════════╝\n");
}

/**
 * Print CSV header
 */
void metrics_print_csv_header(void)
{
    if (output_format != OUTPUT_CSV)
        return;
    
    printk("CSV_HEADER,timestamp,task_id,activation,response_time,exec_time,"
           "deadline_met,lateness,period,deadline,weight,jitter\n");
}

/**
 * Print CSV record
 */
void metrics_print_csv_record(uint64_t timestamp, uint32_t task_id,
                              const task_stats_t *stats, uint32_t response_time,
                              bool deadline_met, int32_t lateness,
                              const task_config_t *cfg)
{
    if (output_format != OUTPUT_CSV)
        return;
    
    printk("CSV,%llu,%u,%u,%u,%u,%d,%d,%u,%u,%u,%.2f\n",
           timestamp, task_id, stats->activations, response_time,
           stats->total_exec_time / stats->activations,
           deadline_met ? 1 : 0, lateness,
           cfg->period_ms, cfg->deadline_ms, cfg->weight,
           stats->response_time_std_dev);
}

/**
 * Print JSON record
 */
void metrics_print_json_record(uint64_t timestamp, uint32_t task_id,
                              const task_stats_t *stats, uint32_t response_time,
                              bool deadline_met, int32_t lateness,
                              const task_config_t *cfg)
{
    if (output_format != OUTPUT_JSON)
        return;
    
    printk("{\"timestamp\":%llu,\"task_id\":%u,\"activation\":%u,"
           "\"response_time\":%u,\"exec_time\":%u,\"deadline_met\":%s,"
           "\"lateness\":%d,\"period\":%u,\"deadline\":%u,\"weight\":%u,"
           "\"jitter\":%.2f}\n",
           timestamp, task_id, stats->activations, response_time,
           stats->total_exec_time / stats->activations,
           deadline_met ? "true" : "false", lateness,
           cfg->period_ms, cfg->deadline_ms, cfg->weight,
           stats->response_time_std_dev);
}
