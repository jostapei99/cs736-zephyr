/*
 * Example: Using the Modular RT Scheduler API
 *
 * This example demonstrates how to use the improved modular scheduler API
 * to create periodic real-time tasks with different scheduling algorithms.
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel/sched_rt.h>
#include <zephyr/sys/printk.h>

#define STACK_SIZE 1024
#define NUM_TASKS 3

/* Task parameters */
struct task_params {
	uint32_t period_ms;
	uint32_t exec_time_ms;
	int weight;
	const char *name;
};

const struct task_params task_configs[NUM_TASKS] = {
	{.period_ms = 100, .exec_time_ms = 20, .weight = 3, .name = "HighPrio"},
	{.period_ms = 200, .exec_time_ms = 40, .weight = 2, .name = "MedPrio"},
	{.period_ms = 500, .exec_time_ms = 50, .weight = 1, .name = "LowPrio"},
};

/* Thread storage */
K_THREAD_STACK_ARRAY_DEFINE(task_stacks, NUM_TASKS, STACK_SIZE);
struct k_thread task_threads[NUM_TASKS];

/* Periodic task implementation */
void periodic_task(void *p1, void *p2, void *p3)
{
	const struct task_params *params = p1;
	uint64_t next_release = k_uptime_get() + params->period_ms;

	/* Configure RT scheduling parameters using the modular API */
	k_thread_rt_config(k_current_get(), 
			   params->period_ms,
			   params->exec_time_ms,
			   params->weight);

	printk("[%s] Configured: period=%u ms, exec=%u ms, weight=%d\n",
	       params->name, params->period_ms, params->exec_time_ms, params->weight);

	while (1) {
		/* Wait for next release */
		int64_t sleep_time = next_release - k_uptime_get();
		if (sleep_time > 0) {
			k_sleep(K_MSEC(sleep_time));
		}

		/* Update deadline for next period */
		k_thread_deadline_set(k_current_get(), 
				      k_ms_to_cyc_ceil32(params->period_ms));

		/* Simulate work */
		uint64_t start = k_uptime_get();
		k_busy_wait(params->exec_time_ms * 1000);
		uint64_t end = k_uptime_get();

		printk("[%s] Executed: response_time=%llu ms\n",
		       params->name, end - start);

		next_release += params->period_ms;
	}
}

int main(void)
{
	printk("\n=== Modular RT Scheduler Example ===\n");
	
#if defined(CONFIG_736_MOD_EDF)
	printk("Scheduler: Weighted EDF\n");
#elif defined(CONFIG_736_WSRT)
	printk("Scheduler: WSRT (Weighted Shortest Remaining Time)\n");
#elif defined(CONFIG_736_RMS)
	printk("Scheduler: RMS (Rate Monotonic)\n");
#elif defined(CONFIG_SCHED_DEADLINE)
	printk("Scheduler: Standard EDF\n");
#else
	printk("Scheduler: Priority-based\n");
#endif

	printk("Creating %d periodic tasks...\n\n", NUM_TASKS);

	/* Create all tasks at the same priority level (5) */
	for (int i = 0; i < NUM_TASKS; i++) {
		k_thread_create(&task_threads[i], task_stacks[i], STACK_SIZE,
				periodic_task,
				(void *)&task_configs[i], NULL, NULL,
				5,  /* Same priority for all - scheduler differentiates */
				0, K_NO_WAIT);
		
		k_thread_name_set(&task_threads[i], task_configs[i].name);
	}

	printk("Tasks started. Monitoring execution...\n\n");
	return 0;
}
