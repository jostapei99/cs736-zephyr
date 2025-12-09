/*
 * Real-Time Statistics Test Application
 * 
 * Comprehensive testing of RT statistics collection framework
 * Tests all custom scheduling algorithms with LATENCY-CRITICAL workloads
 * 
 * This version emphasizes:
 * - Tight deadline constraints
 * - Variable execution times (simulating I/O, network latency)
 * - Interference and contention scenarios
 * - Deadline miss detection under pressure
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel/sched_rt.h>
#include <zephyr/sys/printk.h>
#include <zephyr/random/random.h>

/* Configuration - Enhanced for latency testing */
#define NUM_TEST_THREADS 5
#define TEST_ITERATIONS 20
#define BASE_WORKLOAD_US 2000     /* Base workload in microseconds */
#define WORKLOAD_VARIANCE 1000    /* Variance in microseconds */
#define STACK_SIZE 2048
#define BASE_PRIORITY 5

/* Latency simulation modes */
#define LATENCY_MODE_NONE 0       /* Predictable execution */
#define LATENCY_MODE_LIGHT 1      /* 10-20% variance */
#define LATENCY_MODE_MODERATE 2   /* 30-50% variance */
#define LATENCY_MODE_HEAVY 3      /* 50-100% variance (I/O simulation) */

static int current_latency_mode = LATENCY_MODE_MODERATE;

/* Test thread parameters - TIGHTER deadlines to stress schedulers */
struct thread_params {
	const char *name;
	uint32_t deadline;     /* in milliseconds - TIGHT! */
	uint32_t period;       /* in milliseconds */
	uint32_t exec_time;    /* expected execution time in milliseconds */
	uint32_t weight;       /* thread weight */
	int latency_sensitivity; /* 0=tolerant, 1=moderate, 2=critical */
};

/* Thread data - More threads with tighter, overlapping deadlines */
static struct thread_params test_params[NUM_TEST_THREADS] = {
	/* Name,      Deadline, Period, Exec, Weight, Latency */
	{"HighPri-A",     25,     50,    8,     5,      2}, /* CRITICAL: Very tight deadline */
	{"MedPri-B",      40,     80,    12,    3,      1}, /* Moderate: Some slack */
	{"LowPri-C",      70,    120,    15,    2,      0}, /* Tolerant: Loose deadline */
	{"Burst-D",       30,     60,    10,    4,      2}, /* CRITICAL: Bursty workload */
	{"Background-E",  90,    150,    18,    1,      0}, /* Low priority background */
};

/* Thread stacks and structures */
static K_THREAD_STACK_ARRAY_DEFINE(test_stacks, NUM_TEST_THREADS, STACK_SIZE);
static struct k_thread test_threads[NUM_TEST_THREADS];
static k_tid_t test_tids[NUM_TEST_THREADS];

/* Synchronization */
static struct k_sem completion_sem;
static struct k_sem start_sem;
static volatile int active_threads = 0;

/* Statistics collection */
static struct k_thread_rt_stats final_stats[NUM_TEST_THREADS];

/* Latency tracking */
static struct {
	uint32_t max_latency;
	uint32_t min_latency;
	uint64_t total_latency;
	uint32_t samples;
} latency_stats[NUM_TEST_THREADS];

/* Simulated workload with variable latency using busy-wait */
static void do_work_variable(uint32_t base_us, int variance_percent)
{
	/* Calculate actual workload with variance */
	int32_t variance_us = (base_us * variance_percent) / 100;
	int32_t actual_us = base_us;
	
	if (variance_us > 0) {
		/* Add random variance to simulate unpredictable latency */
		int32_t random_var = (int32_t)(sys_rand32_get() % (2 * variance_us));
		actual_us += (random_var - variance_us);
	}
	
	if (actual_us < (int32_t)base_us / 2) {
		actual_us = base_us / 2;
	}
	
	/* Use busy-wait to create actual measurable latency */
	k_busy_wait(actual_us);
}

/* Get variance percentage based on latency mode and thread sensitivity */
static int get_latency_variance(int sensitivity)
{
	switch (current_latency_mode) {
	case LATENCY_MODE_NONE:
		return 0;
	case LATENCY_MODE_LIGHT:
		return sensitivity == 2 ? 15 : 10;  /* Critical tasks: 15%, others: 10% */
	case LATENCY_MODE_MODERATE:
		return sensitivity == 2 ? 40 : (sensitivity == 1 ? 30 : 20);
	case LATENCY_MODE_HEAVY:
		return sensitivity == 2 ? 80 : (sensitivity == 1 ? 60 : 40);
	default:
		return 30;
	}
}

/* Test thread entry point - Enhanced with latency awareness */
static void test_thread_entry(void *p1, void *p2, void *p3)
{
	int thread_idx = (int)(uintptr_t)p1;
	struct thread_params *params = &test_params[thread_idx];
	
	printk("[%s] Starting (deadline=%ums, period=%ums, exec=%ums, weight=%u, sensitivity=%s)\n",
	       params->name, params->deadline, params->period, 
	       params->exec_time, params->weight,
	       params->latency_sensitivity == 2 ? "CRITICAL" :
	       params->latency_sensitivity == 1 ? "MODERATE" : "TOLERANT");
	
	/* Initialize latency tracking */
	latency_stats[thread_idx].max_latency = 0;
	latency_stats[thread_idx].min_latency = UINT32_MAX;
	latency_stats[thread_idx].total_latency = 0;
	latency_stats[thread_idx].samples = 0;
	
	/* Wait for all threads to be ready */
	k_sem_take(&start_sem, K_FOREVER);
	
	/* Run test iterations */
	for (int iter = 0; iter < TEST_ITERATIONS; iter++) {
		uint64_t start_time = k_uptime_get();
		uint64_t deadline_abs = start_time + params->deadline;
		
		/* Mark activation for statistics */
#ifdef CONFIG_736_RT_STATS
		k_thread_rt_stats_activation(NULL);
#endif
		
		/* Set scheduling parameters */
		k_thread_deadline_set(k_current_get(), params->deadline);
		
#ifdef CONFIG_736_ADD_ONS
		k_thread_weight_set(k_current_get(), params->weight);
		k_thread_exec_time_set(k_current_get(), params->exec_time);
		k_thread_time_left_set(k_current_get(), params->exec_time);
#endif
		
		/* Simulate work with variable latency */
		int variance = get_latency_variance(params->latency_sensitivity);
		uint32_t work_us = params->exec_time * 1000; /* Convert ms to us */
		do_work_variable(work_us, variance);
		
		/* Additional latency for critical tasks (simulating I/O wait) */
		if (params->latency_sensitivity == 2 && (iter % 5 == 0)) {
			/* Simulate occasional I/O delay for critical tasks */
			uint32_t io_delay = 500 + (sys_rand32_get() % 2000);
			k_busy_wait(io_delay);
		}
		
		/* Measure actual execution latency */
		uint64_t completion_time = k_uptime_get();
		uint32_t actual_latency = (uint32_t)(completion_time - start_time);
		
		/* Update latency statistics */
		latency_stats[thread_idx].samples++;
		latency_stats[thread_idx].total_latency += actual_latency;
		if (actual_latency > latency_stats[thread_idx].max_latency) {
			latency_stats[thread_idx].max_latency = actual_latency;
		}
		if (actual_latency < latency_stats[thread_idx].min_latency) {
			latency_stats[thread_idx].min_latency = actual_latency;
		}
		
		/* Check for deadline miss */
		if (completion_time > deadline_abs) {
			uint32_t tardiness = (uint32_t)(completion_time - deadline_abs);
#ifdef CONFIG_736_RT_STATS
			k_thread_rt_stats_deadline_miss(NULL);
#endif
			printk("[%s] MISSED DEADLINE by %u ms (iter %d, latency=%u ms)\n",
			       params->name, tardiness, iter, actual_latency);
		}
		
		/* Sleep until next period */
		uint64_t elapsed = completion_time - start_time;
		if (elapsed < params->period) {
			k_msleep(params->period - elapsed);
		} else {
			/* Period overrun - immediate next iteration */
			printk("[%s] Period overrun! (%llu ms > %u ms)\n",
			       params->name, elapsed, params->period);
		}
	}
	
	/* Print final latency summary for this thread */
	uint32_t avg_latency = latency_stats[thread_idx].samples > 0 ?
	                       latency_stats[thread_idx].total_latency / 
	                       latency_stats[thread_idx].samples : 0;
	
	printk("[%s] Completed %d iterations - Latency: avg=%u ms, min=%u ms, max=%u ms\n",
	       params->name, TEST_ITERATIONS, avg_latency,
	       latency_stats[thread_idx].min_latency,
	       latency_stats[thread_idx].max_latency);
	
	/* Signal completion */
	if (atomic_dec(&active_threads) == 1) {
		k_sem_give(&completion_sem);
	}
}

/* Helper: Print statistics for a thread with latency analysis */
static void print_thread_stats(int idx)
{
	struct k_thread_rt_stats *stats = &final_stats[idx];
	struct thread_params *params = &test_params[idx];
	
	printk("\n[%s] Statistics:\n", params->name);
	printk("  Activations:        %u\n", stats->activations);
	printk("  Completions:        %u\n", stats->completions);
	printk("  Preemptions:        %u\n", stats->preemptions);
	printk("  Context switches:   %u\n", stats->context_switches);
	printk("  Deadline misses:    %u (%.1f%%)\n", stats->deadline_misses,
	       stats->activations > 0 ? (stats->deadline_misses * 100.0f) / stats->activations : 0);
	printk("  Priority inversions: %u\n", stats->priority_inversions);
	
	/* Latency Analysis */
	if (latency_stats[idx].samples > 0) {
		uint32_t avg_latency = latency_stats[idx].total_latency / 
		                       latency_stats[idx].samples;
		uint32_t jitter = latency_stats[idx].max_latency - 
		                  latency_stats[idx].min_latency;
		
		printk("  Execution Latency:\n");
		printk("    Average:  %u ms\n", avg_latency);
		printk("    Min:      %u ms\n", latency_stats[idx].min_latency);
		printk("    Max:      %u ms\n", latency_stats[idx].max_latency);
		printk("    Jitter:   %u ms\n", jitter);
		printk("    Deadline: %u ms (slack: %d ms)\n", 
		       params->deadline,
		       (int)params->deadline - (int)avg_latency);
		
		/* Latency classification */
		float latency_ratio = (float)avg_latency / params->deadline;
		if (latency_ratio > 0.9f) {
			printk("    WARNING: Running at %.0f%% of deadline!\n", 
			       latency_ratio * 100);
		} else if (latency_ratio > 0.7f) {
			printk("    CAUTION: Running at %.0f%% of deadline\n",
			       latency_ratio * 100);
		}
	}
	
	if (stats->activations > 0) {
		uint64_t avg_response = stats->total_response_time / stats->activations;
		uint64_t avg_waiting = stats->total_waiting_time / stats->activations;
		
		printk("  Response time:\n");
		printk("    Total:    %llu ms\n", stats->total_response_time);
		printk("    Average:  %llu ms\n", avg_response);
		printk("    Min:      %u ms\n", stats->min_response_time);
		printk("    Max:      %u ms\n", stats->max_response_time);
		
		printk("  Waiting time:\n");
		printk("    Total:    %llu ms\n", stats->total_waiting_time);
		printk("    Average:  %llu ms\n", avg_waiting);
		printk("    Min:      %u ms\n", stats->min_waiting_time);
		printk("    Max:      %u ms\n", stats->max_waiting_time);
		
#ifdef CONFIG_736_RT_STATS_SQUARED
		/* Calculate variance and jitter */
		uint64_t avg_resp_sq = avg_response * avg_response;
		uint64_t mean_sq_resp = stats->sum_response_time_sq / stats->activations;
		
		if (mean_sq_resp >= avg_resp_sq) {
			uint64_t variance_resp = mean_sq_resp - avg_resp_sq;
			printk("  Response time variance: %llu ms²\n", variance_resp);
		}
		
		uint64_t avg_wait_sq = avg_waiting * avg_waiting;
		uint64_t mean_sq_wait = stats->sum_waiting_time_sq / stats->activations;
		
		if (mean_sq_wait >= avg_wait_sq) {
			uint64_t variance_wait = mean_sq_wait - avg_wait_sq;
			printk("  Waiting time variance: %llu ms²\n", variance_wait);
		}
#endif /* CONFIG_736_RT_STATS_SQUARED */
		
#ifdef CONFIG_736_RT_STATS_DETAILED
		printk("  Timestamps:\n");
		printk("    Last activation:  %llu ms\n", stats->last_activation_time);
		printk("    Last ready:       %llu ms\n", stats->last_ready_time);
		printk("    Last start:       %llu ms\n", stats->last_start_time);
		printk("    Last completion:  %llu ms\n", stats->last_completion_time);
#endif /* CONFIG_736_RT_STATS_DETAILED */
	}
}

/* Helper: Determine active scheduler */
static const char *get_scheduler_name(void)
{
#ifdef CONFIG_736_MOD_EDF
	return "Weighted EDF";
#elif defined(CONFIG_736_RMS)
	return "Rate Monotonic Scheduling";
#elif defined(CONFIG_736_WSRT)
	return "Weighted Shortest Remaining Time";
#elif defined(CONFIG_736_LLF)
	return "Least Laxity First";
#elif defined(CONFIG_736_PFS)
	return "Proportional Fair Scheduling";
#else
	return "Standard EDF";
#endif
}

/* Test 1: Basic statistics collection */
static int test_basic_stats(void)
{
	printk("\n====================================\n");
	printk("Test 1: Basic Statistics Collection\n");
	printk("====================================\n");
	printk("Scheduler: %s\n", get_scheduler_name());
	
	/* Initialize synchronization */
	k_sem_init(&completion_sem, 0, 1);
	k_sem_init(&start_sem, 0, NUM_TEST_THREADS);
	active_threads = NUM_TEST_THREADS;
	
	/* Create test threads */
	for (int i = 0; i < NUM_TEST_THREADS; i++) {
		test_tids[i] = k_thread_create(&test_threads[i],
		                                test_stacks[i],
		                                STACK_SIZE,
		                                test_thread_entry,
		                                (void *)(uintptr_t)i, NULL, NULL,
		                                BASE_PRIORITY, 0, K_NO_WAIT);
		
#ifdef CONFIG_736_RT_STATS
		/* Reset statistics before test */
		k_thread_rt_stats_reset(test_tids[i]);
#endif
	}
	
	printk("\nStarting test threads...\n");
	
	/* Release all threads simultaneously */
	for (int i = 0; i < NUM_TEST_THREADS; i++) {
		k_sem_give(&start_sem);
	}
	
	/* Wait for completion */
	k_sem_take(&completion_sem, K_FOREVER);
	
	printk("\nAll threads completed. Collecting statistics...\n");
	
	/* Collect final statistics */
#ifdef CONFIG_736_RT_STATS
	for (int i = 0; i < NUM_TEST_THREADS; i++) {
		int ret = k_thread_rt_stats_get(test_tids[i], &final_stats[i]);
		if (ret != 0) {
			printk("ERROR: Failed to get stats for thread %d\n", i);
			return -1;
		}
		print_thread_stats(i);
	}
#else
	printk("WARNING: CONFIG_736_RT_STATS not enabled - no statistics collected\n");
#endif
	
	printk("\n✓ Test 1 PASSED\n");
	return 0;
}

/* Test 2: Statistics reset functionality */
static int test_stats_reset(void)
{
	printk("\n====================================\n");
	printk("Test 2: Statistics Reset\n");
	printk("====================================\n");
	
#ifdef CONFIG_736_RT_STATS
	/* Verify stats exist from Test 1 */
	struct k_thread_rt_stats stats_before;
	k_thread_rt_stats_get(test_tids[0], &stats_before);
	
	if (stats_before.activations == 0) {
		printk("ERROR: No statistics from previous test\n");
		return -1;
	}
	
	printk("Before reset: %u activations\n", stats_before.activations);
	
	/* Reset statistics */
	k_thread_rt_stats_reset(test_tids[0]);
	
	/* Verify reset */
	struct k_thread_rt_stats stats_after;
	k_thread_rt_stats_get(test_tids[0], &stats_after);
	
	if (stats_after.activations != 0 ||
	    stats_after.deadline_misses != 0 ||
	    stats_after.total_response_time != 0) {
		printk("ERROR: Statistics not properly reset\n");
		printk("  activations: %u (expected 0)\n", stats_after.activations);
		return -1;
	}
	
	printk("After reset: all fields cleared\n");
	printk("✓ Test 2 PASSED\n");
#else
	printk("SKIPPED: CONFIG_736_RT_STATS not enabled\n");
#endif
	
	return 0;
}

/* Test 3: Validate statistics accuracy */
static int test_stats_accuracy(void)
{
	printk("\n====================================\n");
	printk("Test 3: Statistics Accuracy\n");
	printk("====================================\n");
	
#ifdef CONFIG_736_RT_STATS
	int errors = 0;
	
	for (int i = 0; i < NUM_TEST_THREADS; i++) {
		struct k_thread_rt_stats *stats = &final_stats[i];
		
		/* Verify activation count */
		if (stats->activations != TEST_ITERATIONS) {
			printk("ERROR: [%s] Expected %d activations, got %u\n",
			       test_params[i].name, TEST_ITERATIONS, stats->activations);
			errors++;
		}
		
		/* Verify timing relationships */
		if (stats->min_response_time > stats->max_response_time) {
			printk("ERROR: [%s] min_response > max_response\n",
			       test_params[i].name);
			errors++;
		}
		
		if (stats->min_waiting_time > stats->max_waiting_time) {
			printk("ERROR: [%s] min_waiting > max_waiting\n",
			       test_params[i].name);
			errors++;
		}
		
		/* Check for reasonable values */
		if (stats->activations > 0) {
			uint64_t avg_response = stats->total_response_time / stats->activations;
			if (avg_response > test_params[i].deadline * 2) {
				printk("WARNING: [%s] Average response time (%llu ms) "
				       "exceeds 2x deadline (%u ms)\n",
				       test_params[i].name, avg_response, 
				       test_params[i].deadline);
			}
		}
	}
	
	if (errors == 0) {
		printk("✓ Test 3 PASSED - All statistics accurate\n");
	} else {
		printk("✗ Test 3 FAILED - %d errors found\n", errors);
		return -1;
	}
#else
	printk("SKIPPED: CONFIG_736_RT_STATS not enabled\n");
#endif
	
	return 0;
}

/* Test 4: Scheduler comparison summary with latency analysis */
static void print_scheduler_summary(void)
{
	printk("\n====================================\n");
	printk("Scheduler Performance Summary\n");
	printk("====================================\n");
	printk("Scheduler: %s\n", get_scheduler_name());
	printk("Latency Mode: %s\n",
	       current_latency_mode == LATENCY_MODE_NONE ? "None (Predictable)" :
	       current_latency_mode == LATENCY_MODE_LIGHT ? "Light (10-20%)" :
	       current_latency_mode == LATENCY_MODE_MODERATE ? "Moderate (30-50%)" :
	       "Heavy (50-100%)");
	
#ifdef CONFIG_736_RT_STATS
	uint64_t total_response = 0;
	uint64_t total_waiting = 0;
	uint32_t total_misses = 0;
	uint32_t total_activations = 0;
	uint32_t critical_misses = 0;
	uint32_t critical_tasks = 0;
	
	for (int i = 0; i < NUM_TEST_THREADS; i++) {
		total_response += final_stats[i].total_response_time;
		total_waiting += final_stats[i].total_waiting_time;
		total_misses += final_stats[i].deadline_misses;
		total_activations += final_stats[i].activations;
		
		/* Track critical task performance */
		if (test_params[i].latency_sensitivity == 2) {
			critical_misses += final_stats[i].deadline_misses;
			critical_tasks++;
		}
	}
	
	if (total_activations > 0) {
		printk("\nAggregate Metrics:\n");
		printk("  Total activations:     %u\n", total_activations);
		printk("  Total deadline misses: %u\n", total_misses);
		printk("  Overall miss rate:     %.2f%%\n",
		       (total_misses * 100.0f) / total_activations);
		printk("  Critical miss rate:    %.2f%% (%u misses from %u critical tasks)\n",
		       critical_tasks > 0 ? (critical_misses * 100.0f) / 
		       (critical_tasks * TEST_ITERATIONS) : 0,
		       critical_misses, critical_tasks);
		printk("  Avg response time:     %llu ms\n",
		       total_response / total_activations);
		printk("  Avg waiting time:      %llu ms\n",
		       total_waiting / total_activations);
	}
	
	printk("\nPer-Thread Latency Analysis:\n");
	printk("  %-12s  %6s  %6s  %6s  %6s  %5s  %5s\n",
	       "Thread", "AvgLat", "Jitter", "Slack", "Deadln", "Miss", "Premp");
	printk("  ------------  ------  ------  ------  ------  -----  -----\n");
	
	for (int i = 0; i < NUM_TEST_THREADS; i++) {
		uint32_t avg_latency = 0;
		uint32_t jitter = 0;
		int32_t slack = 0;
		
		if (latency_stats[i].samples > 0) {
			avg_latency = latency_stats[i].total_latency / 
			              latency_stats[i].samples;
			jitter = latency_stats[i].max_latency - 
			         latency_stats[i].min_latency;
			slack = (int32_t)test_params[i].deadline - (int32_t)avg_latency;
		}
		
		const char *marker = "";
		if (test_params[i].latency_sensitivity == 2) {
			marker = " ⚠️"; /* Critical task */
		}
		
		printk("  %-12s%s %4u ms  %4u ms  %+5d  %4u ms  %5u  %5u\n",
		       test_params[i].name, marker,
		       avg_latency,
		       jitter,
		       slack,
		       test_params[i].deadline,
		       final_stats[i].deadline_misses,
		       final_stats[i].preemptions);
	}
	
	/* Scheduler effectiveness rating */
	printk("\nScheduler Effectiveness:\n");
	float miss_rate = total_activations > 0 ? 
	                  (total_misses * 100.0f) / total_activations : 0;
	float critical_miss_rate = critical_tasks > 0 ?
	                            (critical_misses * 100.0f) / 
	                            (critical_tasks * TEST_ITERATIONS) : 0;
	
	if (critical_miss_rate == 0 && miss_rate == 0) {
		printk("  ✓ EXCELLENT: All deadlines met\n");
	} else if (critical_miss_rate == 0 && miss_rate < 5) {
		printk("  ✓ GOOD: All critical deadlines met, %.1f%% total misses\n", 
		       miss_rate);
	} else if (critical_miss_rate < 5) {
		printk("  ⚠ ACCEPTABLE: %.1f%% critical misses, %.1f%% total misses\n",
		       critical_miss_rate, miss_rate);
	} else {
		printk("  ✗ POOR: %.1f%% critical misses - scheduler may be inadequate\n",
		       critical_miss_rate);
	}
#else
	printk("\nNo statistics available (CONFIG_736_RT_STATS not enabled)\n");
#endif
	
	printk("\n====================================\n");
}

/* Main test entry point */
int main(void)
{
	printk("\n");
	printk("╔════════════════════════════════════════╗\n");
	printk("║  RT Statistics - LATENCY Test         ║\n");
	printk("╚════════════════════════════════════════╝\n");
	
	printk("\nConfiguration:\n");
	printk("  Scheduler:       %s\n", get_scheduler_name());
	printk("  Test threads:    %d (%d critical, %d tolerant)\n", 
	       NUM_TEST_THREADS,
	       2, /* Critical count */
	       2  /* Tolerant count */);
	printk("  Iterations:      %d per thread\n", TEST_ITERATIONS);
	printk("  Base workload:   %d us per execution\n", BASE_WORKLOAD_US);
	printk("  Latency mode:    %s\n",
	       current_latency_mode == LATENCY_MODE_NONE ? "None (Predictable)" :
	       current_latency_mode == LATENCY_MODE_LIGHT ? "Light (10-20% variance)" :
	       current_latency_mode == LATENCY_MODE_MODERATE ? "Moderate (30-50% variance)" :
	       "Heavy (50-100% variance - simulates I/O)");
	
#ifdef CONFIG_736_RT_STATS
	printk("  RT Stats:        ENABLED\n");
#else
	printk("  RT Stats:        DISABLED\n");
#endif
	
#ifdef CONFIG_736_RT_STATS_DETAILED
	printk("  Detailed Stats:  ENABLED\n");
#endif
	
#ifdef CONFIG_736_RT_STATS_SQUARED
	printk("  Variance Calc:   ENABLED\n");
#endif
	
	printk("\n⚠️  LATENCY-CRITICAL WORKLOAD:\n");
	printk("  - Tight, overlapping deadlines (25-90ms)\n");
	printk("  - Variable execution times (simulates I/O)\n");
	printk("  - 5 concurrent threads with contention\n");
	printk("  - Critical tasks marked for analysis\n");
	
	/* Run tests */
	int result = 0;
	
	result = test_basic_stats();
	if (result != 0) {
		printk("\n✗ FATAL: Basic stats test failed\n");
		return result;
	}
	
	result = test_stats_reset();
	if (result != 0) {
		printk("\n✗ FATAL: Stats reset test failed\n");
		return result;
	}
	
	result = test_stats_accuracy();
	if (result != 0) {
		printk("\n✗ FATAL: Stats accuracy test failed\n");
		return result;
	}
	
	/* Print summary */
	print_scheduler_summary();
	
	printk("\n");
	printk("╔════════════════════════════════════════╗\n");
	printk("║  ALL TESTS PASSED ✓                   ║\n");
	printk("╚════════════════════════════════════════╝\n");
	printk("\n");
	
	return 0;
}
