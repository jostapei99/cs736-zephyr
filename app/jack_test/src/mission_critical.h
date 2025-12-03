/*
* Mission Critical Scheduler - Common Definitions
*
* Common constants, structures, and function declarations
*/

#ifndef MISSION_CRITICAL_H
#define MISSION_CRITICAL_H

#include <zephyr/kernel.h>

// Task function declarations
void mission_control_task(void *arg1, void *arg2, void *arg3);
void navigation_task(void *arg1, void *arg2, void *arg3);
void communication_task(void *arg1, void *arg2, void *arg3);
void housekeeping_task(void *arg1, void *arg2, void *arg3);

// Critical task function declarations
void emergency_response_task(void *arg1, void *arg2, void *arg3);
void safety_monitor_task(void *arg1, void *arg2, void *arg3);
void fault_detection_task(void *arg1, void *arg2, void *arg3);

// Timing analysis function declarations
void timing_analysis_work_handler(struct k_work *work);
void timing_analysis_timer_handler(struct k_timer *timer);

// Utility function declarations
void init_critical_tasks(void);
void start_timing_analysis(void);
void log_scheduler_state(void);
void analyze_memory_usage(void);
void analyze_thread_states(void);

// Shared resources
extern struct k_mutex resource_mutex;
extern struct k_sem task_sync_sem;
extern volatile bool simulation_running;

// Task priorities (using preemptive priorities 0-15, where 0=highest)
#define CRITICAL_TASK_PRIORITY      K_PRIO_PREEMPT(2)
#define HIGH_PRIORITY_TASK         K_PRIO_PREEMPT(5) 
#define MEDIUM_PRIORITY_TASK       K_PRIO_PREEMPT(8)
#define LOW_PRIORITY_TASK          K_PRIO_PREEMPT(12)

// Critical task priorities (highest priority preemptive)
#define SAFETY_MONITOR_PRIORITY    K_PRIO_PREEMPT(3)
#define FAULT_HANDLER_PRIORITY     K_PRIO_PREEMPT(1)
#define EMERGENCY_TASK_PRIORITY    K_PRIO_PREEMPT(0)

// Stack sizes
#define TASK_STACK_SIZE 2048
#define CRITICAL_STACK_SIZE 1024

// Timing constants
#define DEADLINE_MS 100
#define SIMULATION_DURATION_SEC 30

// Thread state constants for modern Zephyr
#define THREAD_STATE_CREATED    0
#define THREAD_STATE_READY      1
#define THREAD_STATE_RUNNING    2
#define THREAD_STATE_SUSPENDED  3
#define THREAD_STATE_TERMINATED 4

#endif /* MISSION_CRITICAL_H */

