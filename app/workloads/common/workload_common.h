/*
 * Copyright (c) 2024 CS736 Project
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file workload_common.h
 * @brief Common definitions and utilities for RT scheduler workload evaluation
 */

#ifndef WORKLOAD_COMMON_H
#define WORKLOAD_COMMON_H

#include <zephyr/kernel.h>
#include <zephyr/kernel/sched_rt.h>
#include <zephyr/sys/printk.h>
#include <stdint.h>
#include <stdbool.h>

/* Maximum number of tasks in any workload */
#define MAX_WORKLOAD_TASKS 16

/* Default test duration in milliseconds */
#ifndef TEST_DURATION_MS
#define TEST_DURATION_MS 10000
#endif

/* Stack size for workload tasks */
#define WORKLOAD_TASK_STACK_SIZE 2048

/* Enable CSV output for easy parsing */
#ifndef CSV_OUTPUT
#define CSV_OUTPUT 1
#endif

/* Enable statistics collection */
#ifndef ENABLE_RT_STATS
#define ENABLE_RT_STATS 1
#endif

/**
 * @brief Task configuration structure
 * 
 * Defines the parameters for a single task in the workload
 */
typedef struct {
	const char *name;           /* Task name for logging */
	uint32_t period_ms;         /* Period in milliseconds (0 for sporadic) */
	uint32_t exec_time_ms;      /* Expected execution time in ms */
	uint32_t deadline_ms;       /* Relative deadline (0 = implicit = period) */
	uint32_t weight;            /* Task weight/importance (1-10) */
	int priority;               /* Base priority (use -1 for deadline-based) */
	bool is_sporadic;           /* true for aperiodic tasks */
	uint32_t min_interarrival;  /* Minimum time between sporadic arrivals */
} workload_task_config_t;

/**
 * @brief Task runtime statistics
 * 
 * Collected during workload execution
 */
typedef struct {
	uint32_t task_id;
	uint32_t activations;
	uint32_t deadline_misses;
	uint32_t preemptions;
	uint64_t total_response_time_ms;
	uint32_t min_response_time_ms;
	uint32_t max_response_time_ms;
	uint64_t sum_squared_response;  /* For variance calculation */
	k_tid_t thread_id;
} workload_task_stats_t;

/**
 * @brief Overall workload statistics
 */
typedef struct {
	const char *workload_name;
	const char *scheduler_name;
	uint32_t test_duration_ms;
	uint32_t num_tasks;
	uint32_t total_activations;
	uint32_t total_deadline_misses;
	uint32_t total_context_switches;
	double avg_response_time_ms;
	double response_time_jitter_ms;
	double cpu_utilization_percent;
	uint64_t test_start_time;
	uint64_t test_end_time;
} workload_summary_t;

/**
 * @brief Get current scheduler name as string
 */
static inline const char *get_scheduler_name(void)
{
#if defined(CONFIG_736_MOD_EDF)
	return "Weighted EDF";
#elif defined(CONFIG_736_WSRT)
	return "WSRT";
#elif defined(CONFIG_736_RMS)
	return "RMS";
#elif defined(CONFIG_736_LLF)
	return "LLF";
#elif defined(CONFIG_736_PFS)
	return "PFS";
#elif defined(CONFIG_SCHED_DEADLINE)
	return "EDF";
#else
	return "Unknown";
#endif
}

/**
 * @brief Initialize task statistics
 */
static inline void init_task_stats(workload_task_stats_t *stats, uint32_t task_id)
{
	stats->task_id = task_id;
	stats->activations = 0;
	stats->deadline_misses = 0;
	stats->preemptions = 0;
	stats->total_response_time_ms = 0;
	stats->min_response_time_ms = UINT32_MAX;
	stats->max_response_time_ms = 0;
	stats->sum_squared_response = 0;
	stats->thread_id = NULL;
}

/**
 * @brief Update task statistics after a job completion
 */
static inline void update_task_stats(workload_task_stats_t *stats,
				     uint32_t response_time_ms,
				     bool deadline_missed,
				     bool was_preempted)
{
	stats->activations++;
	stats->total_response_time_ms += response_time_ms;
	
	if (response_time_ms < stats->min_response_time_ms) {
		stats->min_response_time_ms = response_time_ms;
	}
	if (response_time_ms > stats->max_response_time_ms) {
		stats->max_response_time_ms = response_time_ms;
	}
	
	stats->sum_squared_response += (uint64_t)response_time_ms * response_time_ms;
	
	if (deadline_missed) {
		stats->deadline_misses++;
	}
	if (was_preempted) {
		stats->preemptions++;
	}
}

/**
 * @brief Calculate average response time for a task
 */
static inline double calc_avg_response_time(workload_task_stats_t *stats)
{
	if (stats->activations == 0) {
		return 0.0;
	}
	return (double)stats->total_response_time_ms / stats->activations;
}

/**
 * @brief Calculate response time standard deviation (jitter)
 */
static inline double calc_response_time_stddev(workload_task_stats_t *stats)
{
	if (stats->activations < 2) {
		return 0.0;
	}
	
	double avg = calc_avg_response_time(stats);
	double variance = ((double)stats->sum_squared_response / stats->activations) - (avg * avg);
	
	if (variance < 0.0) {
		variance = 0.0;  /* Numerical error protection */
	}
	
	return sqrt(variance);
}

/**
 * @brief Print CSV header
 */
static inline void print_csv_header(void)
{
#if CSV_OUTPUT
	printk("timestamp_ms,task_id,activation,response_ms,missed,preempted,scheduler\n");
#endif
}

/**
 * @brief Print CSV data row
 */
static inline void print_csv_row(uint64_t timestamp_ms, uint32_t task_id,
				 uint32_t activation, uint32_t response_ms,
				 bool missed, bool preempted)
{
#if CSV_OUTPUT
	printk("%llu,%u,%u,%u,%d,%d,%s\n",
	       timestamp_ms, task_id, activation, response_ms,
	       missed ? 1 : 0, preempted ? 1 : 0,
	       get_scheduler_name());
#endif
}

/**
 * @brief Print task statistics summary
 */
static inline void print_task_summary(workload_task_stats_t *stats, const char *task_name)
{
	double avg_response = calc_avg_response_time(stats);
	double stddev = calc_response_time_stddev(stats);
	double miss_rate = (stats->activations > 0) ? 
			   (100.0 * stats->deadline_misses / stats->activations) : 0.0;
	
	printk("  %s: %u activations, %u misses (%.2f%%), "
	       "avg response: %.2fms, jitter: %.2fms, "
	       "min/max: %u/%ums\n",
	       task_name, stats->activations, stats->deadline_misses, miss_rate,
	       avg_response, stddev,
	       stats->min_response_time_ms, stats->max_response_time_ms);
}

/**
 * @brief Print overall workload summary
 */
static inline void print_workload_summary(workload_summary_t *summary,
					  workload_task_stats_t *task_stats,
					  uint32_t num_tasks)
{
	printk("\n");
	printk("================================================================================\n");
	printk("Workload: %s\n", summary->workload_name);
	printk("Scheduler: %s\n", summary->scheduler_name);
	printk("Duration: %u ms\n", summary->test_duration_ms);
	printk("Num Tasks: %u\n", num_tasks);
	printk("================================================================================\n");
	printk("\n");
	
	printk("Task Statistics:\n");
	for (uint32_t i = 0; i < num_tasks; i++) {
		char name_buf[32];
		snprintf(name_buf, sizeof(name_buf), "Task %u", i + 1);
		print_task_summary(&task_stats[i], name_buf);
	}
	
	printk("\n");
	printk("Overall Summary:\n");
	printk("  Total Activations: %u\n", summary->total_activations);
	printk("  Deadline Misses: %u (%.2f%%)\n",
	       summary->total_deadline_misses,
	       summary->total_activations > 0 ?
	       (100.0 * summary->total_deadline_misses / summary->total_activations) : 0.0);
	printk("  Avg Response Time: %.2f ms\n", summary->avg_response_time_ms);
	printk("  Response Time Jitter: %.2f ms\n", summary->response_time_jitter_ms);
	printk("  Test Duration: %llu ms\n", summary->test_end_time - summary->test_start_time);
	printk("================================================================================\n");
}

/**
 * @brief Calculate overall workload summary from task statistics
 */
static inline void calculate_workload_summary(workload_summary_t *summary,
					      workload_task_stats_t *task_stats,
					      uint32_t num_tasks,
					      const char *workload_name)
{
	summary->workload_name = workload_name;
	summary->scheduler_name = get_scheduler_name();
	summary->num_tasks = num_tasks;
	summary->total_activations = 0;
	summary->total_deadline_misses = 0;
	summary->total_context_switches = 0;
	
	uint64_t total_response_sum = 0;
	uint64_t total_response_sq_sum = 0;
	
	for (uint32_t i = 0; i < num_tasks; i++) {
		summary->total_activations += task_stats[i].activations;
		summary->total_deadline_misses += task_stats[i].deadline_misses;
		summary->total_context_switches += task_stats[i].preemptions;
		total_response_sum += task_stats[i].total_response_time_ms;
		total_response_sq_sum += task_stats[i].sum_squared_response;
	}
	
	if (summary->total_activations > 0) {
		summary->avg_response_time_ms = 
			(double)total_response_sum / summary->total_activations;
		
		double variance = ((double)total_response_sq_sum / summary->total_activations) -
				  (summary->avg_response_time_ms * summary->avg_response_time_ms);
		
		if (variance < 0.0) {
			variance = 0.0;
		}
		summary->response_time_jitter_ms = sqrt(variance);
	} else {
		summary->avg_response_time_ms = 0.0;
		summary->response_time_jitter_ms = 0.0;
	}
}

/**
 * @brief Busy wait to simulate task execution
 * 
 * More accurate than k_sleep for short durations
 */
static inline void simulate_work(uint32_t duration_ms)
{
	if (duration_ms == 0) {
		return;
	}
	
	uint64_t start = k_uptime_get();
	uint64_t end_target = start + duration_ms;
	
	while (k_uptime_get() < end_target) {
		/* Busy wait */
		k_busy_wait(100);  /* 100 microseconds at a time */
	}
}

/**
 * @brief Sleep until absolute time (for periodic tasks)
 */
static inline void sleep_until(uint64_t target_time_ms)
{
	int64_t now = k_uptime_get();
	int64_t sleep_time = target_time_ms - now;
	
	if (sleep_time > 0) {
		k_sleep(K_MSEC(sleep_time));
	}
}

/**
 * @brief Calculate theoretical CPU utilization from task config
 */
static inline double calc_theoretical_utilization(const workload_task_config_t *configs,
						  uint32_t num_tasks)
{
	double util = 0.0;
	
	for (uint32_t i = 0; i < num_tasks; i++) {
		if (configs[i].period_ms > 0 && !configs[i].is_sporadic) {
			util += (double)configs[i].exec_time_ms / configs[i].period_ms;
		}
	}
	
	return util * 100.0;  /* Return as percentage */
}

/**
 * @brief Validate workload configuration
 */
static inline bool validate_workload_config(const workload_task_config_t *configs,
					    uint32_t num_tasks)
{
	bool valid = true;
	
	for (uint32_t i = 0; i < num_tasks; i++) {
		if (!configs[i].is_sporadic && configs[i].period_ms == 0) {
			printk("ERROR: Task %u has period_ms = 0 but is not sporadic\n", i);
			valid = false;
		}
		
		if (configs[i].exec_time_ms == 0) {
			printk("ERROR: Task %u has exec_time_ms = 0\n", i);
			valid = false;
		}
		
		uint32_t deadline = (configs[i].deadline_ms == 0) ? 
				    configs[i].period_ms : configs[i].deadline_ms;
		
		if (!configs[i].is_sporadic && configs[i].exec_time_ms > deadline) {
			printk("WARNING: Task %u exec_time (%ums) > deadline (%ums)\n",
			       i, configs[i].exec_time_ms, deadline);
		}
		
		if (configs[i].weight == 0) {
			printk("WARNING: Task %u has weight = 0, using default weight = 1\n", i);
		}
	}
	
	return valid;
}

#endif /* WORKLOAD_COMMON_H */
