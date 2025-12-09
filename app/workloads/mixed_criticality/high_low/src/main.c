/*
 * Copyright (c) 2024 CS736 Project
 * SPDX-License-Identifier: Apache-2.0
 *
 * Mixed-Criticality Workload
 * 
 * This workload has tasks with different importance levels (weights).
 * Designed to evaluate how weighted schedulers differentiate task priorities.
 * 
 * Total utilization: ~75%
 * - Critical tasks (weight=10): 2 tasks, 30% utilization
 * - Important tasks (weight=5): 2 tasks, 25% utilization  
 * - Best-effort tasks (weight=1): 2 tasks, 20% utilization
 */

#include <zephyr/kernel.h>
#include <workload_common.h>
#include <task_generator.h>

#define NUM_TASKS 6
#define WORKLOAD_NAME "Mixed-Criticality (Critical + Important + Best-Effort)"

static const workload_task_config_t task_configs[NUM_TASKS] = {
	/* Critical tasks - must complete even under overload */
	{
		.name = "Critical1",
		.period_ms = 100,
		.exec_time_ms = 20,
		.deadline_ms = 0,
		.weight = 10,          /* High importance */
		.priority = -1,
		.is_sporadic = false,
		.min_interarrival = 0,
	},
	{
		.name = "Critical2",
		.period_ms = 200,
		.exec_time_ms = 20,
		.deadline_ms = 0,
		.weight = 10,
		.priority = -1,
		.is_sporadic = false,
		.min_interarrival = 0,
	},
	
	/* Important tasks - should complete under normal load */
	{
		.name = "Important1",
		.period_ms = 150,
		.exec_time_ms = 25,
		.deadline_ms = 0,
		.weight = 5,           /* Medium importance */
		.priority = -1,
		.is_sporadic = false,
		.min_interarrival = 0,
	},
	{
		.name = "Important2",
		.period_ms = 300,
		.exec_time_ms = 25,
		.deadline_ms = 0,
		.weight = 5,
		.priority = -1,
		.is_sporadic = false,
		.min_interarrival = 0,
	},
	
	/* Best-effort tasks - can miss deadlines if needed */
	{
		.name = "BestEffort1",
		.period_ms = 250,
		.exec_time_ms = 30,
		.deadline_ms = 0,
		.weight = 1,           /* Low importance */
		.priority = -1,
		.is_sporadic = false,
		.min_interarrival = 0,
	},
	{
		.name = "BestEffort2",
		.period_ms = 500,
		.exec_time_ms = 20,
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
	printk("\n");
	
	printk("Task Criticality Levels:\n");
	printk("  Critical (weight=10): Task 1, Task 2\n");
	printk("  Important (weight=5): Task 3, Task 4\n");
	printk("  Best-Effort (weight=1): Task 5, Task 6\n");
	printk("\n");
	printk("Test Duration: %u ms\n", TEST_DURATION_MS);
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
	
	/* Analyze per-criticality performance */
	printk("\nPer-Criticality Analysis:\n");
	
	uint32_t critical_misses = task_stats[0].deadline_misses + task_stats[1].deadline_misses;
	uint32_t critical_total = task_stats[0].activations + task_stats[1].activations;
	uint32_t important_misses = task_stats[2].deadline_misses + task_stats[3].deadline_misses;
	uint32_t important_total = task_stats[2].activations + task_stats[3].activations;
	uint32_t besteffort_misses = task_stats[4].deadline_misses + task_stats[5].deadline_misses;
	uint32_t besteffort_total = task_stats[4].activations + task_stats[5].activations;
	
	printk("  Critical Tasks: %u/%u misses (%.2f%%)\n", 
	       critical_misses, critical_total,
	       critical_total > 0 ? (100.0 * critical_misses / critical_total) : 0.0);
	printk("  Important Tasks: %u/%u misses (%.2f%%)\n",
	       important_misses, important_total,
	       important_total > 0 ? (100.0 * important_misses / important_total) : 0.0);
	printk("  Best-Effort Tasks: %u/%u misses (%.2f%%)\n",
	       besteffort_misses, besteffort_total,
	       besteffort_total > 0 ? (100.0 * besteffort_misses / besteffort_total) : 0.0);
	
	printk("\nWorkload completed\n");
	
	return 0;
}
