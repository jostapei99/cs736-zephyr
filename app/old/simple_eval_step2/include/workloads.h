#ifndef WORKLOADS_H
#define WORKLOADS_H

#include <stdint.h>

/* Workload Selection */
#define WORKLOAD_LIGHT    1
#define WORKLOAD_MEDIUM   2
#define WORKLOAD_HEAVY    3
#define WORKLOAD_OVERLOAD 4

/* SELECT WORKLOAD HERE - Change this to test different scenarios */
#ifndef CURRENT_WORKLOAD
#define CURRENT_WORKLOAD WORKLOAD_LIGHT
#endif

#define DYNAMIC_WEIGHTING_ON 1
#define DYNAMIC_WEIGHTING_OFF 0
#define WEIGHT_ADJUSTMENT_THRESHOLD 0.1     /* threshold for percentage of deadlines missed */

#ifndef DYNAMIC_WEIGHTING
#define DYNAMIC_WEIGHTING DYNAMIC_WEIGHTING_ON
#endif

/* Number of tasks in the system */
#define NUM_TASKS 4

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
    const task_config_t task_configs[NUM_TASKS] = {
        {.name = "Task1", .period_ms = 100, .exec_time_ms = 20, .deadline_ms = 100, .weight = 1},
        {.name = "Task2", .period_ms = 200, .exec_time_ms = 30, .deadline_ms = 200, .weight = 1},
        {.name = "Task3", .period_ms = 300, .exec_time_ms = 40, .deadline_ms = 300, .weight = 1},
        {.name = "Task4", .period_ms = 500, .exec_time_ms = 50, .deadline_ms = 500, .weight = 1},
    };
    const char *workload_name = "Light Load (~50%)";
#elif CURRENT_WORKLOAD == WORKLOAD_MEDIUM
    /* Medium load: ~70% total utilization */
    const task_config_t task_configs[NUM_TASKS] = {
        {.name = "Task1", .period_ms = 100, .exec_time_ms = 30, .deadline_ms = 100, .weight = 1},
        {.name = "Task2", .period_ms = 200, .exec_time_ms = 50, .deadline_ms = 200, .weight = 1},
        {.name = "Task3", .period_ms = 300, .exec_time_ms = 60, .deadline_ms = 300, .weight = 1},
        {.name = "Task4", .period_ms = 500, .exec_time_ms = 70, .deadline_ms = 500, .weight = 1},
    };
    const char *workload_name = "Medium Load (~70%)";
#elif CURRENT_WORKLOAD == WORKLOAD_HEAVY
    /* Heavy load: ~90% total utilization */
    const task_config_t task_configs[NUM_TASKS] = {
        {.name = "Task1", .period_ms = 100, .exec_time_ms = 40, .deadline_ms = 100, .weight = 1},
        {.name = "Task2", .period_ms = 200, .exec_time_ms = 70, .deadline_ms = 200, .weight = 1},
        {.name = "Task3", .period_ms = 300, .exec_time_ms = 80, .deadline_ms = 300, .weight = 1},
        {.name = "Task4", .period_ms = 500, .exec_time_ms = 90, .deadline_ms = 500, .weight = 1},
    };
    const char *workload_name = "Heavy Load (~90%)";
#elif CURRENT_WORKLOAD == WORKLOAD_OVERLOAD
    /* Overload: ~110% total utilization */
    const task_config_t task_configs[NUM_TASKS] = {
        {.name = "Task1", .period_ms = 100, .exec_time_ms = 50, .deadline_ms = 100, .weight = 1},
        {.name = "Task2", .period_ms = 200, .exec_time_ms = 80, .deadline_ms = 200, .weight = 1},
        {.name = "Task3", .period_ms = 300, .exec_time_ms = 90, .deadline_ms = 300, .weight = 1},
        {.name = "Task4", .period_ms = 500, .exec_time_ms = 100, .deadline_ms = 500, .weight = 1},
    };
    const char *workload_name = "Overload (~110%)";
#else 
    #error "Unknown CURRENT_WORKLOAD selection"
#endif /* CURRENT_WORKLOAD selection */

#endif /* WORKLOADS_H */