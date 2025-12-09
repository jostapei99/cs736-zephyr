/*
 * Copyright (c) 2024 CS736 Project
 * SPDX-License-Identifier: Apache-2.0
 *
 * Sustained Overload Workload
 * 
 * This workload provides ~110% CPU utilization continuously.
 * Designed to test scheduler behavior under sustained overload conditions.
 * 
 * Expected: All schedulers will miss some deadlines
 * Goal: Observe which tasks suffer and how gracefully each scheduler degrades
 */

#include <zephyr/kernel.h>
#include <workload_common.h>
#include <task_generator.h>

#define NUM_TASKS 5
#define WORKLOAD_NAME "Sustained Overload (110% Utilization)"

/* Task configurations: ~110% total utilization */
static const workload_task_config_t task_configs[NUM_TASKS] = {
	{
		.name = "Task1",
		.period_ms = 100,
		.exec_time_ms = 35,
		.deadline_ms = 0,
		.weight = 2,           /* Medium importance */
		.priority = -1,
		.is_sporadic = false,
		.min_interarrival = 0,
	},
	{
		.name = "Task2",
		.period_ms = 150,
		.exec_time_ms = 45,
		.deadline_ms = 0,
		.weight = 1,
		.priority = -1,
		.is_sporadic = false,
		.min_interarrival = 0,
	},
	{
		.name = "Task3",
		.period_ms = 200,
		.exec_time_ms = 50,
		.deadline_ms = 0,
		.weight = 3,           /* Higher importance */
		.priority = -1,
		.is_sporadic = false,
		.min_interarrival = 0,
	},
	{
		.name = "Task4",
		.period_ms = 300,
		.exec_time_ms = 60,
		.deadline_ms = 0,
		.weight = 1,
		.priority = -1,
		.is_sporadic = false,
		.min_interarrival = 0,
	},
	{
		.name = "Task5",
		.period_ms = 400,
		.exec_time_ms = 70,
		.deadline_ms = 0,
		.weight = 1,
		.priority = -1,
		.is_sporadic = false,
		.min_interarrival = 0,
	},
};

static K_THREAD_STACK_ARRAY_DEFINE(task_stacks, NUM_TASKS, WORKLOAD_TASK_STACK_SIZE);
static workload_task_stats_t task_stats[NUM_TASKS];
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
	
	if (!validate_workload_config(task_configs, NUM_TASKS)) {
		printk("ERROR: Invalid workload configuration\n");
		return -1;
	}
	
	double util = calc_theoretical_utilization(task_configs, NUM_TASKS);
	printk("Theoretical CPU Utilization: %.1f%%\n", util);
	printk("Test Duration: %u ms\n", TEST_DURATION_MS);
	printk("\n");
	printk("WARNING: This workload is OVERLOADED - deadline misses expected!\n");
	printk("Goal: Observe graceful degradation and which tasks are protected\n");
	printk("\n");
	
	for (uint32_t i = 0; i < NUM_TASKS; i++) {
		init_task_stats(&task_stats[i], i + 1);
	}
	
	print_csv_header();
	
	uint64_t start_time = k_uptime_get();
	
	create_workload_tasks(task_configs, task_stats, &stop_flag, NUM_TASKS,
			      task_stacks[0], WORKLOAD_TASK_STACK_SIZE, 
			      K_PRIO_PREEMPT(5));
	
	printk("All tasks created, running for %u ms...\n\n", TEST_DURATION_MS);
	
	k_sleep(K_MSEC(TEST_DURATION_MS));
	stop_flag = true;
	k_sleep(K_MSEC(500));
	
	uint64_t end_time = k_uptime_get();
	
	workload_summary_t summary = {0};
	summary.test_start_time = start_time;
	summary.test_end_time = end_time;
	summary.test_duration_ms = TEST_DURATION_MS;
	
	calculate_workload_summary(&summary, task_stats, NUM_TASKS, WORKLOAD_NAME);
	print_workload_summary(&summary, task_stats, NUM_TASKS);
	
	/* Analyze which tasks suffered most */
	printk("\nDegradation Analysis:\n");
	for (uint32_t i = 0; i < NUM_TASKS; i++) {
		double miss_rate = (task_stats[i].activations > 0) ?
				   (100.0 * task_stats[i].deadline_misses / task_stats[i].activations) : 0.0;
		printk("  Task %u (weight=%u): %.1f%% miss rate\n",
		       i + 1, task_configs[i].weight, miss_rate);
	}
	
	printk("\nWorkload completed\n");
	
	return 0;
}
