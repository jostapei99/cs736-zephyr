/*
 * Workload 4: Multi-Rate Sporadic Workload with Deadline Scheduling
 * 
 * Simulates an automotive ECU or IoT gateway with:
 * - Fast sporadic high-priority events (1-10ms intervals)
 * - Medium sporadic events (10-50ms intervals)
 * - Slow periodic background processing
 * - Deadline-based scheduling using Zephyr's EDF support
 * 
 * Tests: EDF scheduler, sporadic server, aperiodic task handling, starvation prevention
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/timing/timing.h>
#include <zephyr/random/random.h>

/* Thread stack sizes */
#define FAST_STACK_SIZE 1024
#define MEDIUM_STACK_SIZE 2048
#define SLOW_STACK_SIZE 2048
#define DEADLINE_STACK_SIZE 2048

/* Thread priorities - all same static priority for EDF */
#define SPORADIC_PRIORITY 5

/* Execution times in microseconds */
#define FAST_EXEC_US 800          /* 0.8ms */
#define MEDIUM_EXEC_US 2500       /* 2.5ms */
#define SLOW_EXEC_US 8000         /* 8ms */
#define DEADLINE_EXEC_US 3000     /* 3ms */

/* Arrival patterns */
#define FAST_MIN_INTERVAL_MS 1
#define FAST_MAX_INTERVAL_MS 10
#define MEDIUM_MIN_INTERVAL_MS 10
#define MEDIUM_MAX_INTERVAL_MS 50
#define SLOW_PERIOD_MS 100

/* Deadlines for EDF (relative, in milliseconds) */
#define FAST_DEADLINE_MS 5
#define MEDIUM_DEADLINE_MS 15
#define SLOW_DEADLINE_MS 100
#define DEADLINE_TASK_DEADLINE_MS 10

/* Test duration */
#define TEST_DURATION_MS 10000

/* Statistics */
struct sporadic_stats {
	uint32_t arrivals;
	uint32_t completions;
	uint32_t deadline_misses;
	uint64_t total_latency_us;
	uint64_t max_latency_us;
	uint64_t total_response_time_us;
	uint64_t max_response_time_us;
	uint32_t min_interarrival_ms;
	uint32_t max_interarrival_ms;
	uint64_t total_tardiness_us;  /* Sum of (response_time - deadline) for misses */
};

static struct sporadic_stats fast_stats = { .min_interarrival_ms = UINT32_MAX };
static struct sporadic_stats medium_stats = { .min_interarrival_ms = UINT32_MAX };
static struct sporadic_stats slow_stats = { .min_interarrival_ms = UINT32_MAX };
static struct sporadic_stats deadline_stats = { .min_interarrival_ms = UINT32_MAX };

/* Event queues */
#define QUEUE_SIZE 30
K_MSGQ_DEFINE(fast_queue, sizeof(uint64_t), QUEUE_SIZE, 4);
K_MSGQ_DEFINE(medium_queue, sizeof(uint64_t), QUEUE_SIZE, 4);
K_MSGQ_DEFINE(deadline_queue, sizeof(uint64_t), QUEUE_SIZE, 4);

/* Thread structures */
K_THREAD_STACK_DEFINE(fast_stack, FAST_STACK_SIZE);
K_THREAD_STACK_DEFINE(medium_stack, MEDIUM_STACK_SIZE);
K_THREAD_STACK_DEFINE(slow_stack, SLOW_STACK_SIZE);
K_THREAD_STACK_DEFINE(deadline_stack, DEADLINE_STACK_SIZE);

static struct k_thread fast_thread;
static struct k_thread medium_thread;
static struct k_thread slow_thread;
static struct k_thread deadline_thread;

/* Event generators */
K_THREAD_STACK_DEFINE(fast_gen_stack, 1024);
K_THREAD_STACK_DEFINE(medium_gen_stack, 1024);
K_THREAD_STACK_DEFINE(deadline_gen_stack, 1024);
static struct k_thread fast_gen_thread;
static struct k_thread medium_gen_thread;
static struct k_thread deadline_gen_thread;

/* Timing */
static uint64_t cycles_per_us;
static uint32_t total_events_generated = 0;
static uint32_t total_events_processed = 0;

/* Helper to simulate work */
static void simulate_work(uint32_t duration_us)
{
	timing_t start, end;
	uint64_t cycles_needed = duration_us * cycles_per_us;
	
	start = timing_counter_get();
	while (1) {
		end = timing_counter_get();
		if (timing_cycles_get(&start, &end) >= cycles_needed) {
			break;
		}
	}
}

/* Generate random interval */
static uint32_t get_random_interval(uint32_t min_ms, uint32_t max_ms)
{
	if (min_ms >= max_ms) return min_ms;
	uint32_t range = max_ms - min_ms;
	return min_ms + (sys_rand32_get() % range);
}

/* Fast sporadic event handler */
static void fast_event_handler(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	printk("Fast Event Handler started (Deadline: %dms)\n", FAST_DEADLINE_MS);

	while (1) {
		uint64_t timestamp;
		
		if (k_msgq_get(&fast_queue, &timestamp, K_FOREVER) == 0) {
			timing_t start_time = timing_counter_get();
			
#ifdef CONFIG_SCHED_DEADLINE
			/* Set deadline for this job */
			k_thread_deadline_set(k_current_get(), 
			                      k_ms_to_cyc_ceil32(FAST_DEADLINE_MS));
#endif
			
			/* Calculate latency (queuing delay) */
			uint64_t latency_us = (start_time - timestamp) / cycles_per_us;
			fast_stats.total_latency_us += latency_us;
			if (latency_us > fast_stats.max_latency_us) {
				fast_stats.max_latency_us = latency_us;
			}
			
			/* Process event */
			simulate_work(FAST_EXEC_US);
			
			/* Calculate response time */
			timing_t end_time = timing_counter_get();
			uint64_t response_us = timing_cycles_get(&start_time, &end_time) / cycles_per_us;
			fast_stats.total_response_time_us += response_us;
			if (response_us > fast_stats.max_response_time_us) {
				fast_stats.max_response_time_us = response_us;
			}
			
			/* Check deadline */
			uint64_t deadline_us = FAST_DEADLINE_MS * 1000;
			if (response_us > deadline_us) {
				fast_stats.deadline_misses++;
				fast_stats.total_tardiness_us += (response_us - deadline_us);
			}
			
			fast_stats.completions++;
			total_events_processed++;
		}
	}
}

/* Medium sporadic event handler */
static void medium_event_handler(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	printk("Medium Event Handler started (Deadline: %dms)\n", MEDIUM_DEADLINE_MS);

	while (1) {
		uint64_t timestamp;
		
		if (k_msgq_get(&medium_queue, &timestamp, K_FOREVER) == 0) {
			timing_t start_time = timing_counter_get();
			
#ifdef CONFIG_SCHED_DEADLINE
			k_thread_deadline_set(k_current_get(),
			                      k_ms_to_cyc_ceil32(MEDIUM_DEADLINE_MS));
#endif
			
			uint64_t latency_us = (start_time - timestamp) / cycles_per_us;
			medium_stats.total_latency_us += latency_us;
			if (latency_us > medium_stats.max_latency_us) {
				medium_stats.max_latency_us = latency_us;
			}
			
			simulate_work(MEDIUM_EXEC_US);
			
			timing_t end_time = timing_counter_get();
			uint64_t response_us = timing_cycles_get(&start_time, &end_time) / cycles_per_us;
			medium_stats.total_response_time_us += response_us;
			if (response_us > medium_stats.max_response_time_us) {
				medium_stats.max_response_time_us = response_us;
			}
			
			uint64_t deadline_us = MEDIUM_DEADLINE_MS * 1000;
			if (response_us > deadline_us) {
				medium_stats.deadline_misses++;
				medium_stats.total_tardiness_us += (response_us - deadline_us);
			}
			
			medium_stats.completions++;
			total_events_processed++;
		}
	}
}

/* Slow periodic background task */
static void slow_periodic_task(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	int64_t period_ticks = k_ms_to_ticks_ceil64(SLOW_PERIOD_MS);
	int64_t next_wakeup = k_uptime_ticks();
	
	printk("Slow Periodic Task started (Period: %dms, Deadline: %dms)\n",
	       SLOW_PERIOD_MS, SLOW_DEADLINE_MS);

	while (1) {
		timing_t start_time = timing_counter_get();
		int64_t actual_wakeup = k_uptime_ticks();
		
#ifdef CONFIG_SCHED_DEADLINE
		k_thread_deadline_set(k_current_get(),
		                      k_ms_to_cyc_ceil32(SLOW_DEADLINE_MS));
#endif
		
		slow_stats.arrivals++;
		
		int64_t latency_ticks = actual_wakeup - next_wakeup;
		if (latency_ticks < 0) latency_ticks = 0;
		uint64_t latency_us = k_ticks_to_us_ceil64(latency_ticks);
		slow_stats.total_latency_us += latency_us;
		if (latency_us > slow_stats.max_latency_us) {
			slow_stats.max_latency_us = latency_us;
		}
		
		/* Background processing */
		simulate_work(SLOW_EXEC_US);
		
		timing_t end_time = timing_counter_get();
		uint64_t response_us = timing_cycles_get(&start_time, &end_time) / cycles_per_us;
		slow_stats.total_response_time_us += response_us;
		if (response_us > slow_stats.max_response_time_us) {
			slow_stats.max_response_time_us = response_us;
		}
		
		uint64_t deadline_us = SLOW_DEADLINE_MS * 1000;
		if (response_us > deadline_us) {
			slow_stats.deadline_misses++;
			slow_stats.total_tardiness_us += (response_us - deadline_us);
		}
		
		slow_stats.completions++;
		total_events_processed++;
		
		next_wakeup += period_ticks;
		k_sleep(K_TIMEOUT_ABS_TICKS(next_wakeup));
	}
}

/* Deadline-based sporadic task (tests EDF directly) */
static void deadline_based_task(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	printk("Deadline-Based Task started (Deadline: %dms)\n", DEADLINE_TASK_DEADLINE_MS);

	while (1) {
		uint64_t timestamp;
		
		if (k_msgq_get(&deadline_queue, &timestamp, K_FOREVER) == 0) {
			timing_t start_time = timing_counter_get();
			
#ifdef CONFIG_SCHED_DEADLINE
			/* This task explicitly uses deadline scheduling */
			k_thread_deadline_set(k_current_get(),
			                      k_ms_to_cyc_ceil32(DEADLINE_TASK_DEADLINE_MS));
#endif
			
			uint64_t latency_us = (start_time - timestamp) / cycles_per_us;
			deadline_stats.total_latency_us += latency_us;
			if (latency_us > deadline_stats.max_latency_us) {
				deadline_stats.max_latency_us = latency_us;
			}
			
			/* Variable execution time */
			uint32_t exec_time = DEADLINE_EXEC_US + (sys_rand32_get() % 2000);
			simulate_work(exec_time);
			
			timing_t end_time = timing_counter_get();
			uint64_t response_us = timing_cycles_get(&start_time, &end_time) / cycles_per_us;
			deadline_stats.total_response_time_us += response_us;
			if (response_us > deadline_stats.max_response_time_us) {
				deadline_stats.max_response_time_us = response_us;
			}
			
			uint64_t deadline_us = DEADLINE_TASK_DEADLINE_MS * 1000;
			if (response_us > deadline_us) {
				deadline_stats.deadline_misses++;
				deadline_stats.total_tardiness_us += (response_us - deadline_us);
			}
			
			deadline_stats.completions++;
			total_events_processed++;
		}
	}
}

/* Event generators */
static void fast_event_generator(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	printk("Fast Event Generator started (%d-%dms intervals)\n",
	       FAST_MIN_INTERVAL_MS, FAST_MAX_INTERVAL_MS);
	
	int64_t last_arrival = 0;

	while (1) {
		uint32_t interval = get_random_interval(FAST_MIN_INTERVAL_MS, FAST_MAX_INTERVAL_MS);
		k_sleep(K_MSEC(interval));
		
		/* Track inter-arrival time */
		int64_t current = k_uptime_get();
		if (last_arrival > 0) {
			uint32_t interarrival = (uint32_t)(current - last_arrival);
			if (interarrival < fast_stats.min_interarrival_ms) {
				fast_stats.min_interarrival_ms = interarrival;
			}
			if (interarrival > fast_stats.max_interarrival_ms) {
				fast_stats.max_interarrival_ms = interarrival;
			}
		}
		last_arrival = current;
		
		uint64_t timestamp = timing_counter_get();
		if (k_msgq_put(&fast_queue, &timestamp, K_NO_WAIT) == 0) {
			fast_stats.arrivals++;
			total_events_generated++;
		}
	}
}

static void medium_event_generator(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	printk("Medium Event Generator started (%d-%dms intervals)\n",
	       MEDIUM_MIN_INTERVAL_MS, MEDIUM_MAX_INTERVAL_MS);
	
	int64_t last_arrival = 0;

	while (1) {
		uint32_t interval = get_random_interval(MEDIUM_MIN_INTERVAL_MS, MEDIUM_MAX_INTERVAL_MS);
		k_sleep(K_MSEC(interval));
		
		int64_t current = k_uptime_get();
		if (last_arrival > 0) {
			uint32_t interarrival = (uint32_t)(current - last_arrival);
			if (interarrival < medium_stats.min_interarrival_ms) {
				medium_stats.min_interarrival_ms = interarrival;
			}
			if (interarrival > medium_stats.max_interarrival_ms) {
				medium_stats.max_interarrival_ms = interarrival;
			}
		}
		last_arrival = current;
		
		uint64_t timestamp = timing_counter_get();
		if (k_msgq_put(&medium_queue, &timestamp, K_NO_WAIT) == 0) {
			medium_stats.arrivals++;
			total_events_generated++;
		}
	}
}

static void deadline_event_generator(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	printk("Deadline Event Generator started (20-60ms intervals)\n");
	
	int64_t last_arrival = 0;

	while (1) {
		uint32_t interval = get_random_interval(20, 60);
		k_sleep(K_MSEC(interval));
		
		int64_t current = k_uptime_get();
		if (last_arrival > 0) {
			uint32_t interarrival = (uint32_t)(current - last_arrival);
			if (interarrival < deadline_stats.min_interarrival_ms) {
				deadline_stats.min_interarrival_ms = interarrival;
			}
			if (interarrival > deadline_stats.max_interarrival_ms) {
				deadline_stats.max_interarrival_ms = interarrival;
			}
		}
		last_arrival = current;
		
		uint64_t timestamp = timing_counter_get();
		if (k_msgq_put(&deadline_queue, &timestamp, K_NO_WAIT) == 0) {
			deadline_stats.arrivals++;
			total_events_generated++;
		}
	}
}

/* Print statistics */
static void print_statistics(void)
{
	printk("\n=== Workload 4: Multi-Rate Sporadic Workload Results ===\n\n");
	
#ifdef CONFIG_SCHED_DEADLINE
	printk("Scheduler: DEADLINE (EDF) Enabled\n\n");
#else
	printk("Scheduler: Priority-Based (EDF Not Enabled)\n\n");
#endif
	
	printk("Fast Sporadic Events (Deadline: %dms):\n", FAST_DEADLINE_MS);
	printk("  Arrivals: %u, Completions: %u\n", fast_stats.arrivals, fast_stats.completions);
	printk("  Deadline Misses: %u (%.2f%%)\n", fast_stats.deadline_misses,
	       fast_stats.completions > 0 ? 100.0 * fast_stats.deadline_misses / fast_stats.completions : 0);
	printk("  Inter-arrival: %u - %u ms\n",
	       fast_stats.min_interarrival_ms == UINT32_MAX ? 0 : fast_stats.min_interarrival_ms,
	       fast_stats.max_interarrival_ms);
	printk("  Avg/Max Latency: %llu / %llu us\n",
	       fast_stats.completions > 0 ? fast_stats.total_latency_us / fast_stats.completions : 0,
	       fast_stats.max_latency_us);
	printk("  Avg/Max Response: %llu / %llu us\n",
	       fast_stats.completions > 0 ? fast_stats.total_response_time_us / fast_stats.completions : 0,
	       fast_stats.max_response_time_us);
	printk("  Avg Tardiness: %llu us\n\n",
	       fast_stats.deadline_misses > 0 ? fast_stats.total_tardiness_us / fast_stats.deadline_misses : 0);
	
	printk("Medium Sporadic Events (Deadline: %dms):\n", MEDIUM_DEADLINE_MS);
	printk("  Arrivals: %u, Completions: %u\n", medium_stats.arrivals, medium_stats.completions);
	printk("  Deadline Misses: %u (%.2f%%)\n", medium_stats.deadline_misses,
	       medium_stats.completions > 0 ? 100.0 * medium_stats.deadline_misses / medium_stats.completions : 0);
	printk("  Inter-arrival: %u - %u ms\n",
	       medium_stats.min_interarrival_ms == UINT32_MAX ? 0 : medium_stats.min_interarrival_ms,
	       medium_stats.max_interarrival_ms);
	printk("  Avg/Max Response: %llu / %llu us\n",
	       medium_stats.completions > 0 ? medium_stats.total_response_time_us / medium_stats.completions : 0,
	       medium_stats.max_response_time_us);
	printk("  Avg Tardiness: %llu us\n\n",
	       medium_stats.deadline_misses > 0 ? medium_stats.total_tardiness_us / medium_stats.deadline_misses : 0);
	
	printk("Slow Periodic Task (Period: %dms, Deadline: %dms):\n",
	       SLOW_PERIOD_MS, SLOW_DEADLINE_MS);
	printk("  Arrivals: %u, Completions: %u\n", slow_stats.arrivals, slow_stats.completions);
	printk("  Deadline Misses: %u (%.2f%%)\n", slow_stats.deadline_misses,
	       slow_stats.completions > 0 ? 100.0 * slow_stats.deadline_misses / slow_stats.completions : 0);
	printk("  Avg/Max Response: %llu / %llu us\n",
	       slow_stats.completions > 0 ? slow_stats.total_response_time_us / slow_stats.completions : 0,
	       slow_stats.max_response_time_us);
	printk("  Avg Tardiness: %llu us\n\n",
	       slow_stats.deadline_misses > 0 ? slow_stats.total_tardiness_us / slow_stats.deadline_misses : 0);
	
	printk("Deadline-Based Task (Deadline: %dms):\n", DEADLINE_TASK_DEADLINE_MS);
	printk("  Arrivals: %u, Completions: %u\n", deadline_stats.arrivals, deadline_stats.completions);
	printk("  Deadline Misses: %u (%.2f%%)\n", deadline_stats.deadline_misses,
	       deadline_stats.completions > 0 ? 100.0 * deadline_stats.deadline_misses / deadline_stats.completions : 0);
	printk("  Avg/Max Response: %llu / %llu us\n",
	       deadline_stats.completions > 0 ? deadline_stats.total_response_time_us / deadline_stats.completions : 0,
	       deadline_stats.max_response_time_us);
	printk("  Avg Tardiness: %llu us\n\n",
	       deadline_stats.deadline_misses > 0 ? deadline_stats.total_tardiness_us / deadline_stats.deadline_misses : 0);
	
	printk("Overall Statistics:\n");
	printk("  Total Events Generated: %u\n", total_events_generated);
	printk("  Total Events Processed: %u\n", total_events_processed);
	printk("  Events per second: %u\n", total_events_processed / (TEST_DURATION_MS / 1000));
	
	uint32_t total_misses = fast_stats.deadline_misses + medium_stats.deadline_misses +
	                        slow_stats.deadline_misses + deadline_stats.deadline_misses;
	printk("  Total Deadline Misses: %u\n", total_misses);
	printk("  Overall Deadline Miss Rate: %.2f%%\n",
	       total_events_processed > 0 ? 100.0 * total_misses / total_events_processed : 0);
}

int main(void)
{
	printk("\n=== Workload 4: Multi-Rate Sporadic Workload ===\n");
	printk("Testing scheduler with sporadic arrivals and deadline-based scheduling\n");
	printk("Duration: %d seconds\n\n", TEST_DURATION_MS / 1000);
	
	/* Initialize timing */
	timing_init();
	timing_start();
	
	timing_t start = timing_counter_get();
	k_busy_wait(1000000);
	timing_t end = timing_counter_get();
	uint64_t total_cycles = timing_cycles_get(&start, &end);
	cycles_per_us = total_cycles / 1000000;
	
	printk("Timing calibration: %llu cycles/second, %llu cycles/us\n\n", total_cycles, cycles_per_us);
	
	/* Create worker threads */
	k_thread_create(&fast_thread, fast_stack, FAST_STACK_SIZE,
	                fast_event_handler, NULL, NULL, NULL,
	                SPORADIC_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&fast_thread, "fast");
	
	k_thread_create(&medium_thread, medium_stack, MEDIUM_STACK_SIZE,
	                medium_event_handler, NULL, NULL, NULL,
	                SPORADIC_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&medium_thread, "medium");
	
	k_thread_create(&slow_thread, slow_stack, SLOW_STACK_SIZE,
	                slow_periodic_task, NULL, NULL, NULL,
	                SPORADIC_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&slow_thread, "slow");
	
	k_thread_create(&deadline_thread, deadline_stack, DEADLINE_STACK_SIZE,
	                deadline_based_task, NULL, NULL, NULL,
	                SPORADIC_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&deadline_thread, "deadline");
	
	/* Create event generators */
	k_thread_create(&fast_gen_thread, fast_gen_stack, 1024,
	                fast_event_generator, NULL, NULL, NULL,
	                8, 0, K_NO_WAIT);
	k_thread_name_set(&fast_gen_thread, "fast_gen");
	
	k_thread_create(&medium_gen_thread, medium_gen_stack, 1024,
	                medium_event_generator, NULL, NULL, NULL,
	                8, 0, K_NO_WAIT);
	k_thread_name_set(&medium_gen_thread, "medium_gen");
	
	k_thread_create(&deadline_gen_thread, deadline_gen_stack, 1024,
	                deadline_event_generator, NULL, NULL, NULL,
	                8, 0, K_NO_WAIT);
	k_thread_name_set(&deadline_gen_thread, "deadline_gen");
	
	/* Run test */
	k_sleep(K_MSEC(TEST_DURATION_MS));
	
	/* Print results */
	print_statistics();
	
	printk("\nTest completed.\n");
	
	return 0;
}
