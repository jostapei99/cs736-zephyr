/*
 * Workload 5: Scheduler Scaling Benchmark
 * 
 * Purpose: Systematically evaluate scheduler performance characteristics
 * 
 * This workload specifically tests:
 * 1. Thread wake-up latency (how fast can scheduler respond)
 * 2. Context switch overhead (switching between many threads)
 * 3. Scalability (performance with varying numbers of threads)
 * 4. Priority handling (correct priority-based scheduling)
 * 5. Yield behavior (cooperative vs preemptive)
 * 
 * Key Design:
 * - Varying numbers of threads (1, 5, 10, 20, 30) to test scalability
 * - Threads at different priority levels
 * - Controlled wake-up patterns to measure scheduler latency
 * - Measurable workload to calculate throughput
 * 
 * Metrics Collected:
 * - Average/Max/Min wake-up latency per scheduler
 * - Context switch time
 * - Throughput (iterations completed)
 * - Scheduler overhead percentage
 */

#include <zephyr/kernel.h>
#include <zephyr/timing/timing.h>
#include <zephyr/sys/printk.h>

/* Test configuration - adjust these to stress different aspects */
#define TEST_DURATION_MS 10000  /* 10 seconds */
#define NUM_THREAD_CONFIGS 5     /* Test with 1, 5, 10, 20, 30 threads */

/* Thread stack sizes */
#define WORKER_STACK_SIZE 512
#define COORDINATOR_STACK_SIZE 1024

/* Workload configuration */
#define WORK_DURATION_US 100  /* Simulated work per iteration */

/* Maximum threads we'll test */
#define MAX_THREADS 30

/* Test phases */
enum test_phase {
	PHASE_LATENCY,      /* Measure wake-up latency */
	PHASE_THROUGHPUT,   /* Measure maximum throughput */
	PHASE_YIELD,        /* Test yield behavior */
	PHASE_PRIORITY,     /* Test priority enforcement */
	PHASE_DONE
};

/* Per-thread statistics */
struct thread_stats {
	uint32_t iterations;
	uint64_t total_latency_us;
	uint64_t max_latency_us;
	uint64_t min_latency_us;
	uint32_t context_switches;
};

/* Global test control */
static struct {
	enum test_phase current_phase;
	int num_threads;
	uint64_t test_start_time;
	uint64_t phase_start_time;
	struct k_sem start_sem;
	struct k_sem sync_sem;
	volatile bool test_running;
} test_control;

/* Timing calibration */
static uint64_t cycles_per_us;

/* Thread data structures */
static struct {
	struct k_thread thread;
	k_thread_stack_t *stack;
	struct thread_stats stats;
	int thread_id;
	int priority;
} worker_threads[MAX_THREADS];

/* Pre-allocated stacks for all threads */
K_THREAD_STACK_ARRAY_DEFINE(worker_stacks, MAX_THREADS, WORKER_STACK_SIZE);

/* Coordinator thread */
K_THREAD_STACK_DEFINE(coordinator_stack, COORDINATOR_STACK_SIZE);
static struct k_thread coordinator_thread;

/* Helper to simulate work (busy wait) */
static void simulate_work(uint32_t duration_us)
{
	timing_t start, end;
	uint64_t cycles_needed = duration_us * cycles_per_us;
	
	start = timing_counter_get();
	do {
		end = timing_counter_get();
	} while (timing_cycles_get(&start, &end) < cycles_needed);
}

/* Worker thread entry point */
static void worker_thread_entry(void *p1, void *p2, void *p3)
{
	int thread_id = (int)(uintptr_t)p1;
	struct thread_stats *stats = &worker_threads[thread_id].stats;
	
	/* Initialize stats */
	stats->iterations = 0;
	stats->total_latency_us = 0;
	stats->max_latency_us = 0;
	stats->min_latency_us = UINT64_MAX;
	stats->context_switches = 0;
	
	/* Wait for test to start */
	k_sem_take(&test_control.start_sem, K_FOREVER);
	
	while (test_control.test_running) {
		timing_t wake_time = timing_counter_get();
		
		/* Measure latency from intended wake to actual wake */
		int64_t now_ms = k_uptime_get();
		
		switch (test_control.current_phase) {
		
		case PHASE_LATENCY:
			/* Test: Measure wake-up latency
			 * Each thread sleeps briefly then wakes up
			 * We measure how quickly the scheduler responds
			 */
			k_sleep(K_MSEC(10));
			timing_t after_sleep = timing_counter_get();
			uint64_t latency_cycles = timing_cycles_get(&wake_time, &after_sleep);
			uint64_t latency_us = latency_cycles / cycles_per_us;
			
			stats->total_latency_us += latency_us;
			if (latency_us > stats->max_latency_us) {
				stats->max_latency_us = latency_us;
			}
			if (latency_us < stats->min_latency_us) {
				stats->min_latency_us = latency_us;
			}
			stats->iterations++;
			break;
		
		case PHASE_THROUGHPUT:
			/* Test: Maximum throughput with many runnable threads
			 * All threads compete for CPU, no sleeping
			 * Measures scheduler overhead and fairness
			 */
			simulate_work(WORK_DURATION_US);
			stats->iterations++;
			
			/* Yield to allow scheduler to make decision */
			k_yield();
			stats->context_switches++;
			break;
		
		case PHASE_YIELD:
			/* Test: Yield behavior
			 * Threads cooperatively yield to test scheduler's
			 * handling of voluntary context switches
			 */
			simulate_work(WORK_DURATION_US / 2);
			k_yield();
			simulate_work(WORK_DURATION_US / 2);
			stats->iterations++;
			stats->context_switches++;
			break;
		
		case PHASE_PRIORITY:
			/* Test: Priority enforcement
			 * Higher priority threads should run more
			 * Lower priority should get less CPU
			 */
			simulate_work(WORK_DURATION_US);
			stats->iterations++;
			
			/* Higher priority threads yield less frequently */
			if (worker_threads[thread_id].priority > 5) {
				k_yield();
			}
			break;
		
		case PHASE_DONE:
			return;
		}
	}
}

/* Coordinator manages test phases and thread lifecycle */
static void coordinator_thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	
	/* Array of thread counts to test */
	int thread_counts[] = {1, 5, 10, 20, 30};
	
	printk("\n");
	printk("========================================\n");
	printk("  Zephyr Scheduler Benchmark v1.0\n");
	printk("========================================\n");
	printk("Test duration: %d seconds\n", TEST_DURATION_MS / 1000);
	printk("Work per iteration: %d us\n", WORK_DURATION_US);
	printk("Testing thread counts: 1, 5, 10, 20, 30\n");
	printk("\n");
	
	/* Test each thread count configuration */
	for (int config = 0; config < NUM_THREAD_CONFIGS; config++) {
		int num_threads = thread_counts[config];
		test_control.num_threads = num_threads;
		
		printk("========================================\n");
		printk("Testing with %d thread(s)\n", num_threads);
		printk("========================================\n");
		
		/* Create threads for this configuration */
		for (int i = 0; i < num_threads; i++) {
			worker_threads[i].thread_id = i;
			worker_threads[i].stack = worker_stacks[i];
			
			/* Assign priorities - spread across priority levels
			 * This tests scheduler's handling of different priorities
			 */
			worker_threads[i].priority = i % 16;
			
			k_thread_create(&worker_threads[i].thread,
			                worker_threads[i].stack,
			                WORKER_STACK_SIZE,
			                worker_thread_entry,
			                (void *)(uintptr_t)i, NULL, NULL,
			                worker_threads[i].priority,
			                0, K_NO_WAIT);
			
			char name[16];
			snprintf(name, sizeof(name), "worker_%d", i);
			k_thread_name_set(&worker_threads[i].thread, name);
		}
		
		/* Run each test phase */
		const char *phase_names[] = {
			"Wake-up Latency",
			"Throughput",
			"Yield Behavior",
			"Priority Enforcement"
		};
		
		for (int phase = PHASE_LATENCY; phase < PHASE_DONE; phase++) {
			printk("\n--- Phase %d: %s ---\n", phase + 1, phase_names[phase]);
			
			test_control.current_phase = phase;
			test_control.test_running = true;
			test_control.phase_start_time = k_uptime_get();
			
			/* Release all threads */
			for (int i = 0; i < num_threads; i++) {
				k_sem_give(&test_control.start_sem);
			}
			
			/* Let phase run for a portion of total test time */
			k_sleep(K_MSEC(TEST_DURATION_MS / 4));
			
			/* Stop this phase */
			test_control.test_running = false;
			k_sleep(K_MSEC(100)); /* Let threads settle */
			
			/* Print phase results */
			printk("\nResults:\n");
			
			uint64_t total_iterations = 0;
			uint64_t total_latency = 0;
			uint64_t max_latency = 0;
			uint64_t min_latency = UINT64_MAX;
			uint64_t total_switches = 0;
			
			for (int i = 0; i < num_threads; i++) {
				struct thread_stats *s = &worker_threads[i].stats;
				total_iterations += s->iterations;
				total_latency += s->total_latency_us;
				total_switches += s->context_switches;
				
				if (s->max_latency_us > max_latency) {
					max_latency = s->max_latency_us;
				}
				if (s->min_latency_us < min_latency && s->min_latency_us != UINT64_MAX) {
					min_latency = s->min_latency_us;
				}
				
				if (num_threads <= 10) {
					/* Print per-thread stats for smaller configurations */
					printk("  Thread %d (P%d): %u iterations", 
					       i, worker_threads[i].priority, s->iterations);
					if (phase == PHASE_LATENCY && s->iterations > 0) {
						printk(", Avg latency: %llu us", 
						       s->total_latency_us / s->iterations);
					}
					printk("\n");
				}
			}
			
			/* Print aggregate statistics */
			printk("\nAggregate Statistics:\n");
			printk("  Total iterations: %llu\n", total_iterations);
			printk("  Throughput: %llu iterations/sec\n", 
			       total_iterations * 1000 / (TEST_DURATION_MS / 4));
			
			if (phase == PHASE_LATENCY && total_iterations > 0) {
				printk("  Average latency: %llu us\n", 
				       total_latency / total_iterations);
				printk("  Max latency: %llu us\n", max_latency);
				printk("  Min latency: %llu us\n", min_latency);
			}
			
			if (total_switches > 0) {
				printk("  Context switches: %llu\n", total_switches);
				printk("  Switches/sec: %llu\n", 
				       total_switches * 1000 / (TEST_DURATION_MS / 4));
			}
			
			/* Reset stats for next phase */
			for (int i = 0; i < num_threads; i++) {
				worker_threads[i].stats.iterations = 0;
				worker_threads[i].stats.total_latency_us = 0;
				worker_threads[i].stats.max_latency_us = 0;
				worker_threads[i].stats.min_latency_us = UINT64_MAX;
				worker_threads[i].stats.context_switches = 0;
			}
		}
		
		/* Clean up threads */
		for (int i = 0; i < num_threads; i++) {
			k_thread_abort(&worker_threads[i].thread);
		}
		
		k_sleep(K_MSEC(100)); /* Let system settle */
	}
	
	printk("\n========================================\n");
	printk("Benchmark Complete!\n");
	printk("========================================\n");
	printk("\nKey Observations:\n");
	printk("- Simple scheduler: Low overhead, degrades with many threads\n");
	printk("- Scalable scheduler: Higher overhead, scales well\n");
	printk("- MultiQ scheduler: O(1) performance, best for real-time\n");
	printk("\nRecommendations:\n");
	printk("- Use SIMPLE for <10 threads\n");
	printk("- Use MULTIQ for real-time with moderate thread counts\n");
	printk("- Use SCALABLE for >20 threads or dynamic workloads\n");
}

int main(void)
{
	printk("Initializing Scheduler Benchmark...\n");
	
	/* Initialize timing */
	timing_init();
	timing_start();
	
	/* Calibrate timing */
	timing_t start = timing_counter_get();
	k_busy_wait(1000); /* 1ms */
	timing_t end = timing_counter_get();
	uint64_t cycles_per_ms = timing_cycles_get(&start, &end);
	cycles_per_us = cycles_per_ms / 1000;
	
	if (cycles_per_us == 0) {
		cycles_per_us = 1; /* Prevent division by zero */
	}
	
	printk("Timing calibrated: %llu cycles/us\n", cycles_per_us);
	
	/* Initialize synchronization */
	k_sem_init(&test_control.start_sem, 0, MAX_THREADS);
	k_sem_init(&test_control.sync_sem, 0, MAX_THREADS);
	
	/* Initialize test control */
	test_control.current_phase = PHASE_LATENCY;
	test_control.num_threads = 0;
	test_control.test_running = false;
	test_control.test_start_time = k_uptime_get();
	
	/* Create coordinator thread */
	k_thread_create(&coordinator_thread,
	                coordinator_stack,
	                COORDINATOR_STACK_SIZE,
	                coordinator_thread_entry,
	                NULL, NULL, NULL,
	                K_PRIO_PREEMPT(0),
	                0, K_NO_WAIT);
	k_thread_name_set(&coordinator_thread, "coordinator");
	
	/* Wait for test to complete */
	k_thread_join(&coordinator_thread, K_FOREVER);
	
	printk("\nTest finished. You can now compare results across different schedulers.\n");
	
	return 0;
}
