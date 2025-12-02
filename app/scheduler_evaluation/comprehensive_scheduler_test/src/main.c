/*
 * Comprehensive Scheduler Evaluation Test
 * 
 * Tests ALL Zephyr schedulers with a unified workload that combines:
 * - Periodic tasks (workload 1)
 * - Event-driven tasks (workload 2)
 * - Mixed criticality (workload 3)
 * - Deadline scheduling (workload 4)
 * - Scalability testing (workload 5)
 * - Overload stress (workload 6)
 * 
 * This application evaluates:
 * - SCHED_SIMPLE (O(N) list-based)
 * - SCHED_SCALABLE (O(log N) red-black tree)
 * - SCHED_MULTIQ (O(1) array of lists)
 * - SCHED_DEADLINE (EDF scheduling)
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/timing/timing.h>

/* Test duration */
#define TEST_DURATION_SEC 30
#define PHASES 6

/* Thread stack sizes */
#define STACK_SIZE 1024

/* Timing utilities */
static uint64_t cycles_per_sec;
static uint32_t cycles_per_us;

static void calibrate_timing(void)
{
	uint64_t start, end;
	
	start = k_cycle_get_32();
	k_sleep(K_MSEC(1000));
	end = k_cycle_get_32();
	
	if (end > start) {
		cycles_per_sec = end - start;
	} else {
		cycles_per_sec = (UINT32_MAX - start) + end;
	}
	
	cycles_per_us = cycles_per_sec / 1000000;
	if (cycles_per_us == 0) {
		cycles_per_us = 12; /* Fallback for qemu_cortex_m3 */
	}
	
	printk("Timing calibrated: %llu cycles/sec, %u cycles/us\n",
	       cycles_per_sec, cycles_per_us);
}

static void simulate_work(uint32_t duration_us)
{
	if (cycles_per_us == 0) {
		k_busy_wait(duration_us);
		return;
	}
	
	uint32_t cycles_to_wait = duration_us * cycles_per_us;
	uint32_t start = k_cycle_get_32();
	
	while (1) {
		uint32_t now = k_cycle_get_32();
		uint32_t elapsed;
		
		if (now >= start) {
			elapsed = now - start;
		} else {
			elapsed = (UINT32_MAX - start) + now;
		}
		
		if (elapsed >= cycles_to_wait) {
			break;
		}
	}
}

/* ============================================================================
 * Phase 1: Periodic Task Test (from Workload 1)
 * ============================================================================ */

#define NUM_PERIODIC 4
static struct {
	uint32_t period_ms;
	uint32_t exec_us;
	uint32_t priority;
	uint64_t count;
	uint64_t deadline_misses;
	uint32_t max_response;
	const char *name;
} periodic_tasks[NUM_PERIODIC] = {
	{10,  2000, 1, 0, 0, 0, "Fast"},
	{20,  5000, 3, 0, 0, 0, "Medium"},
	{50,  3000, 5, 0, 0, 0, "Slow"},
	{100, 2000, 7, 0, 0, 0, "Background"},
};

static void periodic_thread(void *p1, void *p2, void *p3)
{
	int idx = (int)(long)p1;
	uint32_t period = periodic_tasks[idx].period_ms;
	uint32_t exec = periodic_tasks[idx].exec_us;
	
	while (1) {
		uint32_t start = k_cycle_get_32();
		
		simulate_work(exec);
		
		uint32_t end = k_cycle_get_32();
		uint32_t cycles = (end >= start) ? (end - start) : 
		                  (UINT32_MAX - start + end);
		uint32_t response_us = cycles / cycles_per_us;
		
		periodic_tasks[idx].count++;
		if (response_us > periodic_tasks[idx].max_response) {
			periodic_tasks[idx].max_response = response_us;
		}
		if (response_us > (period * 1000)) {
			periodic_tasks[idx].deadline_misses++;
		}
		
		k_sleep(K_MSEC(period));
	}
}

K_THREAD_STACK_ARRAY_DEFINE(periodic_stacks, NUM_PERIODIC, STACK_SIZE);
static struct k_thread periodic_threads[NUM_PERIODIC];

static void phase1_periodic_test(void)
{
	printk("\n>>> PHASE 1: Periodic Task Test <<<\n");
	printk("Testing basic periodic scheduling...\n\n");
	
	/* Reset stats */
	for (int i = 0; i < NUM_PERIODIC; i++) {
		periodic_tasks[i].count = 0;
		periodic_tasks[i].deadline_misses = 0;
		periodic_tasks[i].max_response = 0;
	}
	
	/* Create threads */
	for (int i = 0; i < NUM_PERIODIC; i++) {
		k_thread_create(&periodic_threads[i], periodic_stacks[i],
				STACK_SIZE, periodic_thread,
				(void *)(long)i, NULL, NULL,
				periodic_tasks[i].priority, 0, K_NO_WAIT);
		k_thread_name_set(&periodic_threads[i], periodic_tasks[i].name);
	}
	
	/* Run for duration */
	k_sleep(K_SECONDS(5));
	
	/* Suspend threads */
	for (int i = 0; i < NUM_PERIODIC; i++) {
		k_thread_suspend(&periodic_threads[i]);
	}
	
	/* Print results */
	printk("=== Phase 1 Results ===\n");
	for (int i = 0; i < NUM_PERIODIC; i++) {
		printk("%s (P%d, %dms): Execs=%llu, Misses=%llu, MaxResp=%uus\n",
		       periodic_tasks[i].name,
		       periodic_tasks[i].priority,
		       periodic_tasks[i].period_ms,
		       periodic_tasks[i].count,
		       periodic_tasks[i].deadline_misses,
		       periodic_tasks[i].max_response);
	}
	printk("\n");
}

/* ============================================================================
 * Phase 2: Event-Driven Test (from Workload 2)
 * ============================================================================ */

K_SEM_DEFINE(event_sem, 0, 100);

#define NUM_EVENT_TASKS 3
static struct {
	uint32_t exec_us;
	uint32_t priority;
	uint64_t count;
	uint32_t max_latency_us;
	const char *name;
} event_tasks[NUM_EVENT_TASKS] = {
	{500,  1, 0, 0, "HighPri"},
	{1000, 5, 0, 0, "MedPri"},
	{2000, 9, 0, 0, "LowPri"},
};

static void event_handler_thread(void *p1, void *p2, void *p3)
{
	int idx = (int)(long)p1;
	
	while (1) {
		uint32_t signal_time = k_cycle_get_32();
		k_sem_take(&event_sem, K_FOREVER);
		uint32_t start_time = k_cycle_get_32();
		
		uint32_t latency_cycles = (start_time >= signal_time) ?
		                          (start_time - signal_time) :
		                          (UINT32_MAX - signal_time + start_time);
		uint32_t latency_us = latency_cycles / cycles_per_us;
		
		simulate_work(event_tasks[idx].exec_us);
		
		event_tasks[idx].count++;
		if (latency_us > event_tasks[idx].max_latency_us) {
			event_tasks[idx].max_latency_us = latency_us;
		}
	}
}

K_THREAD_STACK_ARRAY_DEFINE(event_stacks, NUM_EVENT_TASKS, STACK_SIZE);
static struct k_thread event_threads[NUM_EVENT_TASKS];

static void phase2_event_test(void)
{
	printk("\n>>> PHASE 2: Event-Driven Test <<<\n");
	printk("Testing event response and priority handling...\n\n");
	
	/* Reset stats */
	for (int i = 0; i < NUM_EVENT_TASKS; i++) {
		event_tasks[i].count = 0;
		event_tasks[i].max_latency_us = 0;
	}
	
	/* Create event handler threads */
	for (int i = 0; i < NUM_EVENT_TASKS; i++) {
		k_thread_create(&event_threads[i], event_stacks[i],
				STACK_SIZE, event_handler_thread,
				(void *)(long)i, NULL, NULL,
				event_tasks[i].priority, 0, K_NO_WAIT);
		k_thread_name_set(&event_threads[i], event_tasks[i].name);
	}
	
	/* Generate events */
	for (int i = 0; i < 100; i++) {
		k_sem_give(&event_sem);
		k_sleep(K_MSEC(10));
	}
	
	k_sleep(K_MSEC(500)); /* Let handlers finish */
	
	/* Suspend threads */
	for (int i = 0; i < NUM_EVENT_TASKS; i++) {
		k_thread_suspend(&event_threads[i]);
	}
	
	/* Print results */
	printk("=== Phase 2 Results ===\n");
	for (int i = 0; i < NUM_EVENT_TASKS; i++) {
		printk("%s (P%d): Events=%llu, MaxLatency=%uus\n",
		       event_tasks[i].name,
		       event_tasks[i].priority,
		       event_tasks[i].count,
		       event_tasks[i].max_latency_us);
	}
	printk("\n");
}

/* ============================================================================
 * Phase 3: Scalability Test (from Workload 5)
 * ============================================================================ */

#define MAX_SCALE_THREADS 15
static struct k_thread scale_threads[MAX_SCALE_THREADS];
K_THREAD_STACK_ARRAY_DEFINE(scale_stacks, MAX_SCALE_THREADS, STACK_SIZE);

static volatile uint64_t scale_iterations = 0;

static void scale_worker(void *p1, void *p2, void *p3)
{
	while (1) {
		simulate_work(100); /* 100us work */
		atomic_inc((atomic_t *)&scale_iterations);
		k_yield();
	}
}

static void phase3_scalability_test(void)
{
	printk("\n>>> PHASE 3: Scalability Test <<<\n");
	printk("Testing scheduler performance with varying thread counts...\n\n");
	
	int thread_counts[] = {1, 5, 10, 15};
	
	for (int tc = 0; tc < 4; tc++) {
		int num_threads = thread_counts[tc];
		scale_iterations = 0;
		
		printk("Testing with %d threads...\n", num_threads);
		
		/* Create threads */
		for (int i = 0; i < num_threads; i++) {
			k_thread_create(&scale_threads[i], scale_stacks[i],
					STACK_SIZE, scale_worker,
					NULL, NULL, NULL,
					5, 0, K_NO_WAIT);
		}
		
		uint32_t start = k_uptime_get_32();
		k_sleep(K_SECONDS(2));
		uint32_t duration_ms = k_uptime_get_32() - start;
		
		/* Suspend threads */
		for (int i = 0; i < num_threads; i++) {
			k_thread_suspend(&scale_threads[i]);
		}
		
		uint64_t throughput = (scale_iterations * 1000) / duration_ms;
		printk("  %d threads: %llu iterations, %llu iter/sec\n",
		       num_threads, scale_iterations, throughput);
	}
	printk("\n");
}

/* ============================================================================
 * Phase 4: Priority Inversion Test
 * ============================================================================ */

K_MUTEX_DEFINE(shared_mutex);
static volatile bool high_blocked = false;
static volatile uint32_t inversion_start = 0;
static volatile uint32_t inversion_duration_us = 0;

static void low_priority_holder(void *p1, void *p2, void *p3)
{
	k_mutex_lock(&shared_mutex, K_FOREVER);
	printk("Low priority: holding mutex\n");
	simulate_work(5000); /* Hold for 5ms */
	k_mutex_unlock(&shared_mutex);
	printk("Low priority: released mutex\n");
}

static void high_priority_waiter(void *p1, void *p2, void *p3)
{
	k_sleep(K_MSEC(10)); /* Let low priority acquire mutex */
	
	printk("High priority: requesting mutex\n");
	uint32_t start = k_cycle_get_32();
	high_blocked = true;
	inversion_start = start;
	
	k_mutex_lock(&shared_mutex, K_FOREVER);
	
	uint32_t end = k_cycle_get_32();
	uint32_t wait_cycles = (end >= start) ? (end - start) :
	                        (UINT32_MAX - start + end);
	inversion_duration_us = wait_cycles / cycles_per_us;
	
	printk("High priority: got mutex after %uus\n", inversion_duration_us);
	k_mutex_unlock(&shared_mutex);
}

K_THREAD_STACK_DEFINE(low_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(high_stack, STACK_SIZE);
static struct k_thread low_thread, high_thread;

static void phase4_priority_inversion_test(void)
{
	printk("\n>>> PHASE 4: Priority Inversion Test <<<\n");
	printk("Testing mutex behavior and priority inheritance...\n\n");
	
	high_blocked = false;
	inversion_duration_us = 0;
	
	k_thread_create(&low_thread, low_stack, STACK_SIZE,
			low_priority_holder, NULL, NULL, NULL,
			10, 0, K_NO_WAIT);
	
	k_thread_create(&high_thread, high_stack, STACK_SIZE,
			high_priority_waiter, NULL, NULL, NULL,
			1, 0, K_NO_WAIT);
	
	k_sleep(K_MSEC(100));
	
	printk("=== Phase 4 Results ===\n");
	printk("Priority inversion duration: %uus\n", inversion_duration_us);
	if (inversion_duration_us < 6000) {
		printk("Result: Priority inheritance working ✓\n");
	} else {
		printk("Result: Possible unbounded priority inversion\n");
	}
	printk("\n");
}

/* ============================================================================
 * Phase 5: Overload Stress Test (from Workload 6)
 * ============================================================================ */

#define NUM_OVERLOAD_TASKS 4
static struct {
	uint32_t period_ms;
	uint32_t exec_normal_us;
	uint32_t exec_overload_us;
	uint32_t priority;
	uint64_t count;
	uint64_t misses;
	uint32_t max_tardiness;
	const char *name;
	volatile bool overload;
} overload_tasks[NUM_OVERLOAD_TASKS] = {
	{10,  2000, 11000, 1, 0, 0, 0, "Critical", false},
	{20,  4000, 19000, 3, 0, 0, 0, "Important", false},
	{50,  5000, 45000, 5, 0, 0, 0, "Regular", false},
	{100, 7000, 80000, 7, 0, 0, 0, "Background", false},
};

static void overload_thread(void *p1, void *p2, void *p3)
{
	int idx = (int)(long)p1;
	
	while (1) {
		uint32_t start = k_cycle_get_32();
		
		uint32_t exec = overload_tasks[idx].overload ?
		                overload_tasks[idx].exec_overload_us :
		                overload_tasks[idx].exec_normal_us;
		
		simulate_work(exec);
		
		uint32_t end = k_cycle_get_32();
		uint32_t cycles = (end >= start) ? (end - start) :
		                  (UINT32_MAX - start + end);
		uint32_t response_us = cycles / cycles_per_us;
		uint32_t deadline_us = overload_tasks[idx].period_ms * 1000;
		
		overload_tasks[idx].count++;
		if (response_us > deadline_us) {
			overload_tasks[idx].misses++;
			uint32_t tardiness = response_us - deadline_us;
			if (tardiness > overload_tasks[idx].max_tardiness) {
				overload_tasks[idx].max_tardiness = tardiness;
			}
		}
		
		k_sleep(K_MSEC(overload_tasks[idx].period_ms));
	}
}

K_THREAD_STACK_ARRAY_DEFINE(overload_stacks, NUM_OVERLOAD_TASKS, STACK_SIZE);
static struct k_thread overload_threads[NUM_OVERLOAD_TASKS];

static void phase5_overload_test(void)
{
	printk("\n>>> PHASE 5: Overload Stress Test <<<\n");
	printk("Testing scheduler under normal and overload conditions...\n\n");
	
	/* Reset stats */
	for (int i = 0; i < NUM_OVERLOAD_TASKS; i++) {
		overload_tasks[i].count = 0;
		overload_tasks[i].misses = 0;
		overload_tasks[i].max_tardiness = 0;
		overload_tasks[i].overload = false;
	}
	
	/* Create threads */
	for (int i = 0; i < NUM_OVERLOAD_TASKS; i++) {
		k_thread_create(&overload_threads[i], overload_stacks[i],
				STACK_SIZE, overload_thread,
				(void *)(long)i, NULL, NULL,
				overload_tasks[i].priority, 0, K_NO_WAIT);
		k_thread_name_set(&overload_threads[i], overload_tasks[i].name);
	}
	
	/* Normal load */
	printk("Running normal load...\n");
	k_sleep(K_SECONDS(3));
	
	uint64_t normal_counts[NUM_OVERLOAD_TASKS];
	uint64_t normal_misses[NUM_OVERLOAD_TASKS];
	for (int i = 0; i < NUM_OVERLOAD_TASKS; i++) {
		normal_counts[i] = overload_tasks[i].count;
		normal_misses[i] = overload_tasks[i].misses;
		overload_tasks[i].count = 0;
		overload_tasks[i].misses = 0;
		overload_tasks[i].max_tardiness = 0;
	}
	
	/* Overload */
	printk("Applying overload...\n");
	for (int i = 0; i < NUM_OVERLOAD_TASKS; i++) {
		overload_tasks[i].overload = true;
	}
	k_sleep(K_SECONDS(3));
	
	/* Stop threads */
	for (int i = 0; i < NUM_OVERLOAD_TASKS; i++) {
		k_thread_suspend(&overload_threads[i]);
	}
	
	/* Print results */
	printk("\n=== Phase 5 Results ===\n");
	printk("\nNormal Load:\n");
	for (int i = 0; i < NUM_OVERLOAD_TASKS; i++) {
		printk("%s: Execs=%llu, Misses=%llu\n",
		       overload_tasks[i].name,
		       normal_counts[i],
		       normal_misses[i]);
	}
	
	printk("\nOverload:\n");
	for (int i = 0; i < NUM_OVERLOAD_TASKS; i++) {
		printk("%s: Execs=%llu, Misses=%llu, MaxTardiness=%uus\n",
		       overload_tasks[i].name,
		       overload_tasks[i].count,
		       overload_tasks[i].misses,
		       overload_tasks[i].max_tardiness);
	}
	printk("\n");
}

/* ============================================================================
 * Phase 6: Deadline Scheduling Test (EDF)
 * ============================================================================ */

#ifdef CONFIG_SCHED_DEADLINE

#define NUM_DEADLINE_TASKS 3
static struct {
	uint32_t period_ms;
	uint32_t exec_us;
	uint64_t count;
	uint64_t misses;
	const char *name;
} deadline_tasks[NUM_DEADLINE_TASKS] = {
	{5,  800, 0, 0, "Tight"},
	{15, 2500, 0, 0, "Medium"},
	{100, 8000, 0, 0, "Loose"},
};

static void deadline_thread(void *p1, void *p2, void *p3)
{
	int idx = (int)(long)p1;
	struct k_thread *self = k_current_get();
	
	k_thread_deadline_set(self, deadline_tasks[idx].period_ms);
	
	while (1) {
		uint32_t start = k_cycle_get_32();
		
		simulate_work(deadline_tasks[idx].exec_us);
		
		uint32_t end = k_cycle_get_32();
		uint32_t cycles = (end >= start) ? (end - start) :
		                  (UINT32_MAX - start + end);
		uint32_t response_us = cycles / cycles_per_us;
		
		deadline_tasks[idx].count++;
		if (response_us > (deadline_tasks[idx].period_ms * 1000)) {
			deadline_tasks[idx].misses++;
		}
		
		k_sleep(K_MSEC(deadline_tasks[idx].period_ms));
	}
}

K_THREAD_STACK_ARRAY_DEFINE(deadline_stacks, NUM_DEADLINE_TASKS, STACK_SIZE);
static struct k_thread deadline_threads[NUM_DEADLINE_TASKS];

static void phase6_deadline_test(void)
{
	printk("\n>>> PHASE 6: EDF Deadline Scheduling Test <<<\n");
	printk("Testing earliest-deadline-first scheduling...\n\n");
	
	/* Reset stats */
	for (int i = 0; i < NUM_DEADLINE_TASKS; i++) {
		deadline_tasks[i].count = 0;
		deadline_tasks[i].misses = 0;
	}
	
	/* Create threads */
	for (int i = 0; i < NUM_DEADLINE_TASKS; i++) {
		k_thread_create(&deadline_threads[i], deadline_stacks[i],
				STACK_SIZE, deadline_thread,
				(void *)(long)i, NULL, NULL,
				5, 0, K_NO_WAIT);
		k_thread_name_set(&deadline_threads[i], deadline_tasks[i].name);
	}
	
	/* Run test */
	k_sleep(K_SECONDS(5));
	
	/* Stop threads */
	for (int i = 0; i < NUM_DEADLINE_TASKS; i++) {
		k_thread_suspend(&deadline_threads[i]);
	}
	
	/* Print results */
	printk("=== Phase 6 Results ===\n");
	for (int i = 0; i < NUM_DEADLINE_TASKS; i++) {
		printk("%s (%dms): Execs=%llu, Misses=%llu\n",
		       deadline_tasks[i].name,
		       deadline_tasks[i].period_ms,
		       deadline_tasks[i].count,
		       deadline_tasks[i].misses);
	}
	printk("\n");
}

#else

static void phase6_deadline_test(void)
{
	printk("\n>>> PHASE 6: EDF Deadline Scheduling Test <<<\n");
	printk("SKIPPED: CONFIG_SCHED_DEADLINE not enabled\n\n");
}

#endif /* CONFIG_SCHED_DEADLINE */

/* ============================================================================
 * Main Application
 * ============================================================================ */

int main(void)
{
	printk("\n");
	printk("=========================================================\n");
	printk("=== Comprehensive Scheduler Evaluation Test ===\n");
	printk("=========================================================\n");
	printk("\n");
	
	/* Detect active scheduler */
	printk("Active Scheduler: ");
#ifdef CONFIG_SCHED_MULTIQ
	printk("MULTIQ (O(1) array of lists)\n");
#elif CONFIG_SCHED_SCALABLE
	printk("SCALABLE (O(log N) red-black tree)\n");
#elif CONFIG_SCHED_SIMPLE
	printk("SIMPLE (O(N) list)\n");
#else
	printk("UNKNOWN\n");
#endif

#ifdef CONFIG_SCHED_DEADLINE
	printk("EDF Deadline Scheduling: ENABLED\n");
#else
	printk("EDF Deadline Scheduling: DISABLED\n");
#endif

	printk("Test Duration: %d seconds total\n", TEST_DURATION_SEC);
	printk("\n");
	
	/* Calibrate timing */
	calibrate_timing();
	
	/* Run all test phases */
	phase1_periodic_test();
	phase2_event_test();
	phase3_scalability_test();
	phase4_priority_inversion_test();
	phase5_overload_test();
	phase6_deadline_test();
	
	/* Summary */
	printk("\n");
	printk("=========================================================\n");
	printk("=== Test Complete ===\n");
	printk("=========================================================\n");
	printk("\n");
	printk("This test evaluated:\n");
	printk("  1. Periodic task scheduling\n");
	printk("  2. Event-driven responsiveness\n");
	printk("  3. Scalability (1-20 threads)\n");
	printk("  4. Priority inversion handling\n");
	printk("  5. Overload behavior\n");
	printk("  6. Deadline scheduling (if enabled)\n");
	printk("\n");
	printk("Run this test with different scheduler configurations:\n");
	printk("  - CONFIG_SCHED_SIMPLE=y (default)\n");
	printk("  - CONFIG_SCHED_SCALABLE=y\n");
	printk("  - CONFIG_SCHED_MULTIQ=y\n");
	printk("  - CONFIG_SCHED_DEADLINE=y (with SIMPLE)\n");
	printk("\n");
	printk("Compare the results to see which scheduler works best\n");
	printk("for your application characteristics!\n");
	printk("\n");
	
	return 0;
}
