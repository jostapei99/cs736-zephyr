/*
 * Copyright (c) 2024 CS736 Project
 * SPDX-License-Identifier: Apache-2.0
 *
 * Light Load Periodic Workload
 * 
 * This workload provides ~50% CPU utilization with 4 harmonic periodic tasks.
 * Designed to test basic scheduler functionality under comfortable conditions.
 * 
 * All schedulers should handle this without deadline misses.
 */

#include <zephyr/kernel.h>
#include <workload_common.h>
#include <task_generator.h>

#define NUM_TASKS 4
#define WORKLOAD_NAME "Light Load (50% Utilization)"

/* Task configurations: ~50% total utilization */
static const workload_task_config_t task_configs[NUM_TASKS] = {
	{
		.name = "Task1",
		.period_ms = 100,
		.exec_time_ms = 20,
		.deadline_ms = 0,      /* Implicit deadline = period */
		.weight = 1,
		.priority = -1,        /* Use deadline scheduling */
		.is_sporadic = false,
		.min_interarrival = 0,
	},
	{
		.name = "Task2",
		.period_ms = 200,
		.exec_time_ms = 30,
		.deadline_ms = 0,
		.weight = 1,
		.priority = -1,
		.is_sporadic = false,
		.min_interarrival = 0,
	},
	{
		.name = "Task3",
		.period_ms = 400,
		.exec_time_ms = 40,
		.deadline_ms = 0,
		.weight = 1,
		.priority = -1,
		.is_sporadic = false,
		.min_interarrival = 0,
	},
	{
		.name = "Task4",
		.period_ms = 800,
		.exec_time_ms = 60,
		.deadline_ms = 0,
		.weight = 1,
		.priority = -1,
		.is_sporadic = false,
		.min_interarrival = 0,
	},
};

/* Thread stacks */
static K_THREAD_STACK_ARRAY_DEFINE(task_stacks, NUM_TASKS, WORKLOAD_TASK_STACK_SIZE);

/* Task statistics */
static workload_task_stats_t task_stats[NUM_TASKS];

/* Stop flag for coordinated shutdown */
static volatile bool stop_flag = false;

int main(void)
{
	printk("\n");
	printk("================================================================================\n");
	printk("RT Scheduler Workload Evaluation\n");
	printk("Workload: %s\n", WORKLOAD_NAME);
	printk("Scheduler: %s\n", get_scheduler_name());
	printk("================================================================================\n");
	printk("\n");
	
	/* Validate configuration */
	if (!validate_workload_config(task_configs, NUM_TASKS)) {
		printk("ERROR: Invalid workload configuration\n");
		return -1;
	}
	
	/* Print theoretical utilization */
	double util = calc_theoretical_utilization(task_configs, NUM_TASKS);
	printk("Theoretical CPU Utilization: %.1f%%\n", util);
	printk("Test Duration: %u ms\n", TEST_DURATION_MS);
	printk("\n");
	
	/* Initialize statistics */
	for (uint32_t i = 0; i < NUM_TASKS; i++) {
		init_task_stats(&task_stats[i], i + 1);
	}
	
	/* Print CSV header */
	print_csv_header();
	
	/* Record start time */
	uint64_t start_time = k_uptime_get();
	
	/* Create and start tasks */
	create_workload_tasks(task_configs, task_stats, &stop_flag, NUM_TASKS,
			      task_stacks[0], WORKLOAD_TASK_STACK_SIZE, 
			      K_PRIO_PREEMPT(5));
	
	printk("All tasks created, running for %u ms...\n\n", TEST_DURATION_MS);
	
	/* Let tasks run for test duration */
	k_sleep(K_MSEC(TEST_DURATION_MS));
	
	/* Signal all tasks to stop */
	stop_flag = true;
	
	/* Wait a bit for tasks to finish their current jobs */
	k_sleep(K_MSEC(500));
	
	uint64_t end_time = k_uptime_get();
	
	/* Calculate and print summary */
	workload_summary_t summary = {0};
	summary.test_start_time = start_time;
	summary.test_end_time = end_time;
	summary.test_duration_ms = TEST_DURATION_MS;
	
	calculate_workload_summary(&summary, task_stats, NUM_TASKS, WORKLOAD_NAME);
	print_workload_summary(&summary, task_stats, NUM_TASKS);
	
	printk("\nWorkload completed successfully\n");
	
	return 0;
}
