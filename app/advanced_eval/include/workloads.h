#ifndef WORKLOADS_H
#define WORKLOADS_H

#include <stdint.h>

/* Workload Selection */
#define WORKLOAD_LIGHT    1
#define WORKLOAD_MEDIUM   2
#define WORKLOAD_HEAVY    3
#define WORKLOAD_OVERLOAD 4
#define WORKLOAD_CUSTOM   5  /* User-defined via shell */

/* Number of tasks in the system */
#define NUM_TASKS 4

/* SELECT WORKLOAD HERE - Change this to test different scenarios */
#ifndef CURRENT_WORKLOAD
#define CURRENT_WORKLOAD WORKLOAD_LIGHT
#endif

/* Workload structure */
typedef struct {
    const char *name;
    uint32_t period_ms;
    uint32_t exec_time_ms;
    uint32_t deadline_ms;
    uint32_t weight;
} task_config_t;

/* Workload Definitions */
#if CURRENT_WORKLOAD == WORKLOAD_LIGHT
    /* Light load: ~50% total utilization */
    extern task_config_t task_configs[NUM_TASKS];
    extern const char *workload_name;

#elif CURRENT_WORKLOAD == WORKLOAD_MEDIUM
    /* Medium load: ~70% total utilization */
    extern task_config_t task_configs[NUM_TASKS];
    extern const char *workload_name;

#elif CURRENT_WORKLOAD == WORKLOAD_HEAVY
    /* Heavy load: ~90% total utilization */
    extern task_config_t task_configs[NUM_TASKS];
    extern const char *workload_name;

#elif CURRENT_WORKLOAD == WORKLOAD_OVERLOAD
    /* Overload: ~110% total utilization */
    extern task_config_t task_configs[NUM_TASKS];
    extern const char *workload_name;

#elif CURRENT_WORKLOAD == WORKLOAD_CUSTOM
    /* Custom workload - modifiable at runtime */
    extern task_config_t task_configs[NUM_TASKS];
    extern const char *workload_name;
#endif

#endif /* WORKLOADS_H */
