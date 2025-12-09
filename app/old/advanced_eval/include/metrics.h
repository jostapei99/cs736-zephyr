#ifndef METRICS_H
#define METRICS_H

#include <stdint.h>
#include <stdbool.h>
#include "workloads.h"

/* Extended stats structure with advanced metrics */
typedef struct {
    /* Basic timing */
    uint64_t next_release;
    uint32_t activations;
    uint32_t deadline_misses;
    
    /* Response time statistics */
    uint32_t total_response_time;
    uint64_t sum_response_time_squared;
    uint32_t min_response_time;
    uint32_t max_response_time;
    
    /* Jitter metrics */
    double response_time_variance;
    double response_time_std_dev;
    
    /* Execution time tracking */
    uint32_t total_exec_time;
    uint32_t min_exec_time;
    uint32_t max_exec_time;
    
    /* Advanced metrics */
    uint32_t preemptions;
    uint32_t context_switches;
    uint64_t total_lateness;    /* Sum of all lateness */
    uint32_t max_lateness;
    
    /* CPU cycles (if available) */
    uint64_t total_cycles;
    
} task_stats_t;

/* System-wide metrics */
typedef struct {
    uint64_t system_start_time;
    uint64_t total_runtime;
    uint32_t total_activations;
    uint32_t total_deadline_misses;
    double system_utilization;
    uint32_t total_preemptions;
} system_metrics_t;

/* Output format options */
typedef enum {
    OUTPUT_CSV,
    OUTPUT_JSON,
    OUTPUT_HUMAN,
    OUTPUT_QUIET
} output_format_t;

/* Global metrics */
extern system_metrics_t system_metrics;
extern output_format_t output_format;

/* Function declarations */
void metrics_init(void);
void metrics_update(task_stats_t *stats, uint32_t response_time, 
                   uint32_t exec_time, bool deadline_met, uint32_t lateness);
void metrics_calculate_jitter(task_stats_t *stats);
void metrics_print_task_summary(uint32_t task_id, const task_config_t *cfg, 
                               const task_stats_t *stats);
void metrics_print_system_summary(void);
void metrics_print_csv_header(void);
void metrics_print_csv_record(uint64_t timestamp, uint32_t task_id,
                              const task_stats_t *stats, uint32_t response_time,
                              bool deadline_met, int32_t lateness,
                              const task_config_t *cfg);
void metrics_print_json_record(uint64_t timestamp, uint32_t task_id,
                              const task_stats_t *stats, uint32_t response_time,
                              bool deadline_met, int32_t lateness,
                              const task_config_t *cfg);

#endif /* METRICS_H */
