/*
 * Copyright (c) 2024 CS736 Project
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file task_generator.h
 * @brief Helper functions for creating and configuring workload tasks
 */

#ifndef TASK_GENERATOR_H
#define TASK_GENERATOR_H

#include "workload_common.h"

/**
 * @brief Task context passed to thread entry function
 */
typedef struct {
	uint32_t task_id;
	workload_task_config_t config;
	workload_task_stats_t *stats;
	volatile bool *stop_flag;
} task_context_t;

/**
 * @brief Configure a thread with RT parameters based on task config
 */
static inline void configure_rt_thread(k_tid_t tid, const workload_task_config_t *config)
{
	/* Set deadline */
	uint32_t deadline_ms = (config->deadline_ms == 0) ? 
			       config->period_ms : config->deadline_ms;
	if (deadline_ms > 0) {
		k_thread_deadline_set(tid, k_ms_to_cyc_ceil32(deadline_ms));
	}
	
#ifdef CONFIG_736
	/* Set execution time */
	if (config->exec_time_ms > 0) {
		k_thread_exec_time_set(tid, k_ms_to_cyc_ceil32(config->exec_time_ms));
	}
	
	/* Set weight */
	uint32_t weight = (config->weight == 0) ? 1 : config->weight;
	k_thread_weight_set(tid, weight);
#endif /* CONFIG_736 */
}

/**
 * @brief Generic periodic task entry point
 * 
 * This function implements a standard periodic task that:
 * 1. Waits for its period
 * 2. Records activation time
 * 3. Executes for configured duration
 * 4. Records completion and checks for deadline miss
 * 5. Updates statistics
 */
static void periodic_task_entry(void *arg1, void *arg2, void *arg3)
{
	task_context_t *ctx = (task_context_t *)arg1;
	workload_task_config_t *config = &ctx->config;
	workload_task_stats_t *stats = ctx->stats;
	
	/* Store thread ID for statistics */
	stats->thread_id = k_current_get();
	
	/* Configure RT parameters */
	configure_rt_thread(k_current_get(), config);
	
	/* Calculate absolute deadline/period */
	uint32_t period_ms = config->period_ms;
	uint32_t deadline_ms = (config->deadline_ms == 0) ? 
			       period_ms : config->deadline_ms;
	
	uint64_t next_release = k_uptime_get() + period_ms;
	uint32_t activation_count = 0;
	
#if ENABLE_RT_STATS && defined(CONFIG_736_RT_STATS)
	/* Reset kernel statistics */
	k_thread_rt_stats_reset(k_current_get());
#endif
	
	printk("Task %u started: period=%ums, exec=%ums, deadline=%ums, weight=%u\n",
	       ctx->task_id, period_ms, config->exec_time_ms, deadline_ms, config->weight);
	
	while (!(*ctx->stop_flag)) {
		/* Wait until next period */
		sleep_until(next_release);
		
		uint64_t activation_time = k_uptime_get();
		activation_count++;
		
#if ENABLE_RT_STATS && defined(CONFIG_736_RT_STATS)
		/* Record activation in kernel stats */
		k_thread_rt_stats_activation(k_current_get());
#endif
		
		/* Simulate task execution */
		simulate_work(config->exec_time_ms);
		
		uint64_t completion_time = k_uptime_get();
		uint32_t response_time_ms = (uint32_t)(completion_time - activation_time);
		
		/* Check for deadline miss */
		bool deadline_missed = (response_time_ms > deadline_ms);
		
#if ENABLE_RT_STATS && defined(CONFIG_736_RT_STATS)
		if (deadline_missed) {
			k_thread_rt_stats_deadline_miss(k_current_get());
		}
#endif
		
		/* Update statistics */
		update_task_stats(stats, response_time_ms, deadline_missed, false);
		
		/* Output CSV data */
		print_csv_row(activation_time, ctx->task_id, activation_count,
			      response_time_ms, deadline_missed, false);
		
		/* Calculate next release time */
		next_release += period_ms;
		
		/* Prevent drift accumulation if we're already late */
		uint64_t now = k_uptime_get();
		if (next_release < now) {
			next_release = now + period_ms;
		}
	}
	
	printk("Task %u stopped after %u activations\n", ctx->task_id, activation_count);
}

/**
 * @brief Generic sporadic task entry point
 * 
 * For aperiodic tasks with random arrivals
 */
static void sporadic_task_entry(void *arg1, void *arg2, void *arg3)
{
	task_context_t *ctx = (task_context_t *)arg1;
	workload_task_config_t *config = &ctx->config;
	workload_task_stats_t *stats = ctx->stats;
	
	stats->thread_id = k_current_get();
	configure_rt_thread(k_current_get(), config);
	
	uint32_t deadline_ms = (config->deadline_ms == 0) ? 
			       config->min_interarrival : config->deadline_ms;
	uint32_t activation_count = 0;
	
	printk("Sporadic Task %u started: min_interarrival=%ums, exec=%ums, deadline=%ums\n",
	       ctx->task_id, config->min_interarrival, config->exec_time_ms, deadline_ms);
	
	while (!(*ctx->stop_flag)) {
		/* Wait for random arrival time (simplified: use min_interarrival with small variation) */
		uint32_t arrival_time = config->min_interarrival;
		k_sleep(K_MSEC(arrival_time));
		
		if (*ctx->stop_flag) {
			break;
		}
		
		uint64_t activation_time = k_uptime_get();
		activation_count++;
		
		/* Execute */
		simulate_work(config->exec_time_ms);
		
		uint64_t completion_time = k_uptime_get();
		uint32_t response_time_ms = (uint32_t)(completion_time - activation_time);
		bool deadline_missed = (response_time_ms > deadline_ms);
		
		update_task_stats(stats, response_time_ms, deadline_missed, false);
		print_csv_row(activation_time, ctx->task_id, activation_count,
			      response_time_ms, deadline_missed, false);
	}
	
	printk("Sporadic Task %u stopped after %u activations\n", ctx->task_id, activation_count);
}

/**
 * @brief Create and start a task based on configuration
 * 
 * @param config Task configuration
 * @param stats Pointer to statistics structure
 * @param stop_flag Pointer to stop flag
 * @param task_id Task identifier
 * @param stack Pointer to stack memory
 * @param stack_size Size of stack
 * @param priority Thread priority
 * @return Thread ID of created task
 */
static inline k_tid_t create_workload_task(const workload_task_config_t *config,
					    workload_task_stats_t *stats,
					    volatile bool *stop_flag,
					    uint32_t task_id,
					    k_thread_stack_t *stack,
					    size_t stack_size,
					    int priority)
{
	static task_context_t contexts[MAX_WORKLOAD_TASKS];
	
	if (task_id >= MAX_WORKLOAD_TASKS) {
		printk("ERROR: task_id %u exceeds MAX_WORKLOAD_TASKS\n", task_id);
		return NULL;
	}
	
	/* Setup context */
	contexts[task_id].task_id = task_id;
	contexts[task_id].config = *config;
	contexts[task_id].stats = stats;
	contexts[task_id].stop_flag = stop_flag;
	
	/* Choose entry function based on task type */
	k_thread_entry_t entry_fn = config->is_sporadic ? 
				     sporadic_task_entry : periodic_task_entry;
	
	/* Create thread */
	static struct k_thread threads[MAX_WORKLOAD_TASKS];
	k_tid_t tid = k_thread_create(&threads[task_id], stack, stack_size,
				      entry_fn, &contexts[task_id], NULL, NULL,
				      priority, 0, K_NO_WAIT);
	
	return tid;
}

/**
 * @brief Helper to create multiple tasks from config array
 */
static inline void create_workload_tasks(const workload_task_config_t *configs,
					 workload_task_stats_t *stats,
					 volatile bool *stop_flag,
					 uint32_t num_tasks,
					 k_thread_stack_t *stacks,
					 size_t stack_size,
					 int base_priority)
{
	for (uint32_t i = 0; i < num_tasks; i++) {
		k_thread_stack_t *task_stack = (k_thread_stack_t *)
			((uint8_t *)stacks + (i * stack_size));
		
		int priority = (configs[i].priority == -1) ? 
			       base_priority : configs[i].priority;
		
		create_workload_task(&configs[i], &stats[i], stop_flag, i,
				     task_stack, stack_size, priority);
	}
}

#endif /* TASK_GENERATOR_H */
