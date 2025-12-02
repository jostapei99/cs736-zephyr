/*
 * Workload 6: Overload Stress Test
 * 
 * Purpose: Deliberately create system overload to force deadline misses
 * 
 * This workload creates a system with >100% CPU utilization to test:
 * - How schedulers handle overload
 * - Which tasks miss deadlines under stress
 * - Scheduler fairness during overload
 * - Recovery behavior after overload
 * 
 * Design:
 * - Phase 1: Normal load (schedulable)
 * - Phase 2: Moderate overload (85-95% util)
 * - Phase 3: Heavy overload (>100% util) - DEADLINE MISSES EXPECTED
 * - Phase 4: Recovery (return to normal)
 * 
 * Task Set (designed to exceed 100% utilization):
 * - Critical Task: 10ms period, 8ms execution (80% util) - Priority 1
 * - Important Task: 20ms period, 12ms execution (60% util) - Priority 3
 * - Regular Task: 50ms period, 15ms execution (30% util) - Priority 5
 * - Background Task: 100ms period, 10ms execution (10% util) - Priority 7
 * 
 * Total utilization in Phase 3: 180% - GUARANTEED DEADLINE MISSES!
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/random/random.h>

/* Test configuration */
#define TEST_DURATION_MS 20000  /* 20 seconds total */
#define PHASE_DURATION_MS 5000  /* 5 seconds per phase */

/* Thread stack sizes */
#define CRITICAL_STACK_SIZE 1024
#define IMPORTANT_STACK_SIZE 1024
#define REGULAR_STACK_SIZE 1024
#define BACKGROUND_STACK_SIZE 1024

/* Thread priorities */
#define CRITICAL_PRIORITY 1
#define IMPORTANT_PRIORITY 3
#define REGULAR_PRIORITY 5
#define BACKGROUND_PRIORITY 7

/* Task periods (ms) */
#define CRITICAL_PERIOD_MS 10
#define IMPORTANT_PERIOD_MS 20
#define REGULAR_PERIOD_MS 50
#define BACKGROUND_PERIOD_MS 100

/* Execution times per phase (microseconds) */
/* Phase 0: CRITICAL TASK OVERLOADED - execution > deadline! */
#define CRITICAL_EXEC_PHASE0_US 11000   /* 110% util - WILL MISS EVERY TIME! */
#define IMPORTANT_EXEC_PHASE0_US 19000  /* 95% util - WILL MISS! */
#define REGULAR_EXEC_PHASE0_US 45000    /* 90% util - WILL MISS! */
#define BACKGROUND_EXEC_PHASE0_US 10000 /* 10% util */

/* Phase 1: EXTREME OVERLOAD - all tasks exceed deadlines */
#define CRITICAL_EXEC_PHASE1_US 15000   /* 150% util - massive miss */
#define IMPORTANT_EXEC_PHASE1_US 25000  /* 125% util - massive miss */
#define REGULAR_EXEC_PHASE1_US 60000    /* 120% util - massive miss */
#define BACKGROUND_EXEC_PHASE1_US 50000 /* 50% util - will miss */

/* Phase 2: Moderate overload */
#define CRITICAL_EXEC_PHASE2_US 8000    /* 80% util - might miss */
#define IMPORTANT_EXEC_PHASE2_US 18000  /* 90% util - will miss */
#define REGULAR_EXEC_PHASE2_US 45000    /* 90% util - will miss */
#define BACKGROUND_EXEC_PHASE2_US 20000 /* 20% util */

/* Phase 3: Normal (to show recovery) */
#define CRITICAL_EXEC_PHASE3_US 2000    /* 20% util */
#define IMPORTANT_EXEC_PHASE3_US 4000   /* 20% util */
#define REGULAR_EXEC_PHASE3_US 3000     /* 6% util */
#define BACKGROUND_EXEC_PHASE3_US 2000  /* 2% util */

/* Test phases */
enum test_phase {
	PHASE_NORMAL = 0,
	PHASE_MODERATE,
	PHASE_OVERLOAD,
	PHASE_RECOVERY,
	NUM_PHASES
};

static const char *phase_names[] = {
	"PHASE 0: Task Exec > Deadline",
	"PHASE 1: EXTREME OVERLOAD",
	"PHASE 2: Moderate Overload",
	"PHASE 3: Normal (Recovery)"
};

/* Global phase control */
static volatile enum test_phase current_phase = PHASE_NORMAL;
static int64_t phase_start_time;

/* Per-thread statistics */
struct thread_stats {
	uint32_t executions;
	uint32_t deadline_misses;
	uint64_t total_latency_us;
	uint64_t max_latency_us;
	uint64_t total_response_time_us;
	uint64_t max_response_time_us;
	uint32_t total_tardiness_us;  /* Sum of how late we were */
	uint32_t max_tardiness_us;    /* Worst tardiness */
};

/* Statistics per phase per thread */
static struct thread_stats critical_stats[NUM_PHASES];
static struct thread_stats important_stats[NUM_PHASES];
static struct thread_stats regular_stats[NUM_PHASES];
static struct thread_stats background_stats[NUM_PHASES];

/* Thread structures */
K_THREAD_STACK_DEFINE(critical_stack, CRITICAL_STACK_SIZE);
K_THREAD_STACK_DEFINE(important_stack, IMPORTANT_STACK_SIZE);
K_THREAD_STACK_DEFINE(regular_stack, REGULAR_STACK_SIZE);
K_THREAD_STACK_DEFINE(background_stack, BACKGROUND_STACK_SIZE);

static struct k_thread critical_thread;
static struct k_thread important_thread;
static struct k_thread regular_thread;
static struct k_thread background_thread;

/* Timing calibration */
static uint64_t cycles_per_us;

/* Helper to get execution time based on phase */
static uint32_t get_exec_time_us(int task_id, enum test_phase phase)
{
	static const uint32_t exec_times[4][4] = {
		/* Critical, Important, Regular, Background */
		{CRITICAL_EXEC_PHASE0_US, IMPORTANT_EXEC_PHASE0_US, 
		 REGULAR_EXEC_PHASE0_US, BACKGROUND_EXEC_PHASE0_US},
		{CRITICAL_EXEC_PHASE1_US, IMPORTANT_EXEC_PHASE1_US,
		 REGULAR_EXEC_PHASE1_US, BACKGROUND_EXEC_PHASE1_US},
		{CRITICAL_EXEC_PHASE2_US, IMPORTANT_EXEC_PHASE2_US,
		 REGULAR_EXEC_PHASE2_US, BACKGROUND_EXEC_PHASE2_US},
		{CRITICAL_EXEC_PHASE3_US, IMPORTANT_EXEC_PHASE3_US,
		 REGULAR_EXEC_PHASE3_US, BACKGROUND_EXEC_PHASE3_US},
	};
	return exec_times[phase][task_id];
}

/* Simulate work */
static void simulate_work(uint32_t duration_us)
{
	if (cycles_per_us > 1) {
		uint32_t start_cycles = k_cycle_get_32();
		uint32_t cycles_needed = duration_us * cycles_per_us;
		
		while ((k_cycle_get_32() - start_cycles) < cycles_needed) {
			/* Busy wait */
		}
	} else {
		k_busy_wait(duration_us);
	}
}

/* Critical task - highest priority, must not miss deadlines */
static void critical_thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	int64_t period_ticks = k_ms_to_ticks_ceil64(CRITICAL_PERIOD_MS);
	int64_t next_wakeup = k_uptime_ticks();
	
	printk("[CRITICAL] Started (P%d, Period=%dms)\n", 
	       CRITICAL_PRIORITY, CRITICAL_PERIOD_MS);

	while (1) {
		enum test_phase phase = current_phase;
		uint32_t start_cycles = k_cycle_get_32();
		int64_t actual_wakeup = k_uptime_ticks();
		
		/* Calculate latency */
		int64_t latency_ticks = actual_wakeup - next_wakeup;
		if (latency_ticks < 0) latency_ticks = 0;
		uint64_t latency_us = k_ticks_to_us_ceil64(latency_ticks);
		
		critical_stats[phase].total_latency_us += latency_us;
		if (latency_us > critical_stats[phase].max_latency_us) {
			critical_stats[phase].max_latency_us = latency_us;
		}
		
		/* Do work based on current phase */
		uint32_t work_us = get_exec_time_us(0, phase);
		simulate_work(work_us);
		
		/* Calculate response time */
		uint32_t end_cycles = k_cycle_get_32();
		uint32_t elapsed_cycles = end_cycles - start_cycles;
		uint64_t response_us = elapsed_cycles / cycles_per_us;
		
		critical_stats[phase].total_response_time_us += response_us;
		if (response_us > critical_stats[phase].max_response_time_us) {
			critical_stats[phase].max_response_time_us = response_us;
		}
		
		/* Check for deadline miss */
		uint32_t deadline_us = CRITICAL_PERIOD_MS * 1000;
		if (response_us > deadline_us) {
			critical_stats[phase].deadline_misses++;
			uint32_t tardiness = response_us - deadline_us;
			critical_stats[phase].total_tardiness_us += tardiness;
			if (tardiness > critical_stats[phase].max_tardiness_us) {
				critical_stats[phase].max_tardiness_us = tardiness;
			}
			
			/* Print first few deadline misses */
			if (critical_stats[phase].deadline_misses <= 3) {
				printk("[CRITICAL] DEADLINE MISS in %s! Response=%lluus, Deadline=%uus\n",
				       phase_names[phase], response_us, deadline_us);
			}
		}
		
		critical_stats[phase].executions++;
		
		next_wakeup += period_ticks;
		k_sleep(K_TIMEOUT_ABS_TICKS(next_wakeup));
	}
}

/* Important task - medium-high priority */
static void important_thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	int64_t period_ticks = k_ms_to_ticks_ceil64(IMPORTANT_PERIOD_MS);
	int64_t next_wakeup = k_uptime_ticks();
	
	printk("[IMPORTANT] Started (P%d, Period=%dms)\n", 
	       IMPORTANT_PRIORITY, IMPORTANT_PERIOD_MS);

	while (1) {
		enum test_phase phase = current_phase;
		uint32_t start_cycles = k_cycle_get_32();
		int64_t actual_wakeup = k_uptime_ticks();
		
		int64_t latency_ticks = actual_wakeup - next_wakeup;
		if (latency_ticks < 0) latency_ticks = 0;
		uint64_t latency_us = k_ticks_to_us_ceil64(latency_ticks);
		
		important_stats[phase].total_latency_us += latency_us;
		if (latency_us > important_stats[phase].max_latency_us) {
			important_stats[phase].max_latency_us = latency_us;
		}
		
		uint32_t work_us = get_exec_time_us(1, phase);
		simulate_work(work_us);
		
		uint32_t end_cycles = k_cycle_get_32();
		uint32_t elapsed_cycles = end_cycles - start_cycles;
		uint64_t response_us = elapsed_cycles / cycles_per_us;
		
		important_stats[phase].total_response_time_us += response_us;
		if (response_us > important_stats[phase].max_response_time_us) {
			important_stats[phase].max_response_time_us = response_us;
		}
		
		uint32_t deadline_us = IMPORTANT_PERIOD_MS * 1000;
		if (response_us > deadline_us) {
			important_stats[phase].deadline_misses++;
			uint32_t tardiness = response_us - deadline_us;
			important_stats[phase].total_tardiness_us += tardiness;
			if (tardiness > important_stats[phase].max_tardiness_us) {
				important_stats[phase].max_tardiness_us = tardiness;
			}
			
			if (important_stats[phase].deadline_misses <= 3) {
				printk("[IMPORTANT] DEADLINE MISS in %s! Response=%lluus, Deadline=%uus\n",
				       phase_names[phase], response_us, deadline_us);
			}
		}
		
		important_stats[phase].executions++;
		
		next_wakeup += period_ticks;
		k_sleep(K_TIMEOUT_ABS_TICKS(next_wakeup));
	}
}

/* Regular task - medium priority */
static void regular_thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	int64_t period_ticks = k_ms_to_ticks_ceil64(REGULAR_PERIOD_MS);
	int64_t next_wakeup = k_uptime_ticks();
	
	printk("[REGULAR] Started (P%d, Period=%dms)\n", 
	       REGULAR_PRIORITY, REGULAR_PERIOD_MS);

	while (1) {
		enum test_phase phase = current_phase;
		uint32_t start_cycles = k_cycle_get_32();
		int64_t actual_wakeup = k_uptime_ticks();
		
		int64_t latency_ticks = actual_wakeup - next_wakeup;
		if (latency_ticks < 0) latency_ticks = 0;
		uint64_t latency_us = k_ticks_to_us_ceil64(latency_ticks);
		
		regular_stats[phase].total_latency_us += latency_us;
		if (latency_us > regular_stats[phase].max_latency_us) {
			regular_stats[phase].max_latency_us = latency_us;
		}
		
		uint32_t work_us = get_exec_time_us(2, phase);
		simulate_work(work_us);
		
		uint32_t end_cycles = k_cycle_get_32();
		uint32_t elapsed_cycles = end_cycles - start_cycles;
		uint64_t response_us = elapsed_cycles / cycles_per_us;
		
		regular_stats[phase].total_response_time_us += response_us;
		if (response_us > regular_stats[phase].max_response_time_us) {
			regular_stats[phase].max_response_time_us = response_us;
		}
		
		uint32_t deadline_us = REGULAR_PERIOD_MS * 1000;
		if (response_us > deadline_us) {
			regular_stats[phase].deadline_misses++;
			uint32_t tardiness = response_us - deadline_us;
			regular_stats[phase].total_tardiness_us += tardiness;
			if (tardiness > regular_stats[phase].max_tardiness_us) {
				regular_stats[phase].max_tardiness_us = tardiness;
			}
			
			if (regular_stats[phase].deadline_misses <= 3) {
				printk("[REGULAR] DEADLINE MISS in %s! Response=%lluus, Deadline=%uus\n",
				       phase_names[phase], response_us, deadline_us);
			}
		}
		
		regular_stats[phase].executions++;
		
		next_wakeup += period_ticks;
		k_sleep(K_TIMEOUT_ABS_TICKS(next_wakeup));
	}
}

/* Background task - lowest priority */
static void background_thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	int64_t period_ticks = k_ms_to_ticks_ceil64(BACKGROUND_PERIOD_MS);
	int64_t next_wakeup = k_uptime_ticks();
	
	printk("[BACKGROUND] Started (P%d, Period=%dms)\n", 
	       BACKGROUND_PRIORITY, BACKGROUND_PERIOD_MS);

	while (1) {
		enum test_phase phase = current_phase;
		uint32_t start_cycles = k_cycle_get_32();
		int64_t actual_wakeup = k_uptime_ticks();
		
		int64_t latency_ticks = actual_wakeup - next_wakeup;
		if (latency_ticks < 0) latency_ticks = 0;
		uint64_t latency_us = k_ticks_to_us_ceil64(latency_ticks);
		
		background_stats[phase].total_latency_us += latency_us;
		if (latency_us > background_stats[phase].max_latency_us) {
			background_stats[phase].max_latency_us = latency_us;
		}
		
		uint32_t work_us = get_exec_time_us(3, phase);
		simulate_work(work_us);
		
		uint32_t end_cycles = k_cycle_get_32();
		uint32_t elapsed_cycles = end_cycles - start_cycles;
		uint64_t response_us = elapsed_cycles / cycles_per_us;
		
		background_stats[phase].total_response_time_us += response_us;
		if (response_us > background_stats[phase].max_response_time_us) {
			background_stats[phase].max_response_time_us = response_us;
		}
		
		uint32_t deadline_us = BACKGROUND_PERIOD_MS * 1000;
		if (response_us > deadline_us) {
			background_stats[phase].deadline_misses++;
			uint32_t tardiness = response_us - deadline_us;
			background_stats[phase].total_tardiness_us += tardiness;
			if (tardiness > background_stats[phase].max_tardiness_us) {
				background_stats[phase].max_tardiness_us = tardiness;
			}
		}
		
		background_stats[phase].executions++;
		
		next_wakeup += period_ticks;
		k_sleep(K_TIMEOUT_ABS_TICKS(next_wakeup));
	}
}

/* Print statistics for a phase */
static void print_phase_stats(enum test_phase phase)
{
	printk("\n=== %s Statistics ===\n", phase_names[phase]);
	
	/* Critical task */
	struct thread_stats *s = &critical_stats[phase];
	printk("\nCritical Task (P%d, Period=%dms):\n", CRITICAL_PRIORITY, CRITICAL_PERIOD_MS);
	printk("  Executions: %u\n", s->executions);
	printk("  Deadline Misses: %u (%.1f%%)\n", s->deadline_misses,
	       s->executions > 0 ? 100.0 * s->deadline_misses / s->executions : 0);
	if (s->executions > 0) {
		printk("  Avg Response: %llu us\n", s->total_response_time_us / s->executions);
		printk("  Max Response: %llu us\n", s->max_response_time_us);
		printk("  Avg Latency: %llu us\n", s->total_latency_us / s->executions);
	}
	if (s->deadline_misses > 0) {
		printk("  Avg Tardiness: %u us\n", s->total_tardiness_us / s->deadline_misses);
		printk("  Max Tardiness: %u us\n", s->max_tardiness_us);
	}
	
	/* Important task */
	s = &important_stats[phase];
	printk("\nImportant Task (P%d, Period=%dms):\n", IMPORTANT_PRIORITY, IMPORTANT_PERIOD_MS);
	printk("  Executions: %u\n", s->executions);
	printk("  Deadline Misses: %u (%.1f%%)\n", s->deadline_misses,
	       s->executions > 0 ? 100.0 * s->deadline_misses / s->executions : 0);
	if (s->executions > 0) {
		printk("  Avg Response: %llu us\n", s->total_response_time_us / s->executions);
		printk("  Max Response: %llu us\n", s->max_response_time_us);
	}
	if (s->deadline_misses > 0) {
		printk("  Avg Tardiness: %u us\n", s->total_tardiness_us / s->deadline_misses);
		printk("  Max Tardiness: %u us\n", s->max_tardiness_us);
	}
	
	/* Regular task */
	s = &regular_stats[phase];
	printk("\nRegular Task (P%d, Period=%dms):\n", REGULAR_PRIORITY, REGULAR_PERIOD_MS);
	printk("  Executions: %u\n", s->executions);
	printk("  Deadline Misses: %u (%.1f%%)\n", s->deadline_misses,
	       s->executions > 0 ? 100.0 * s->deadline_misses / s->executions : 0);
	if (s->executions > 0) {
		printk("  Avg Response: %llu us\n", s->total_response_time_us / s->executions);
		printk("  Max Response: %llu us\n", s->max_response_time_us);
	}
	if (s->deadline_misses > 0) {
		printk("  Avg Tardiness: %u us\n", s->total_tardiness_us / s->deadline_misses);
		printk("  Max Tardiness: %u us\n", s->max_tardiness_us);
	}
	
	/* Background task */
	s = &background_stats[phase];
	printk("\nBackground Task (P%d, Period=%dms):\n", BACKGROUND_PRIORITY, BACKGROUND_PERIOD_MS);
	printk("  Executions: %u\n", s->executions);
	printk("  Deadline Misses: %u (%.1f%%)\n", s->deadline_misses,
	       s->executions > 0 ? 100.0 * s->deadline_misses / s->executions : 0);
	if (s->executions > 0) {
		printk("  Avg Response: %llu us\n", s->total_response_time_us / s->executions);
	}
}

int main(void)
{
	printk("\n");
	printk("=========================================\n");
	printk("=== Workload 6: Overload Stress Test ===\n");
	printk("=========================================\n");
	printk("Duration: %d seconds (%d seconds per phase)\n", 
	       TEST_DURATION_MS / 1000, PHASE_DURATION_MS / 1000);
	printk("\nThis test deliberately overloads the system\n");
	printk("to force deadline misses and test scheduler behavior.\n\n");
	
	/* Timing calibration */
	uint32_t start_cycles = k_cycle_get_32();
	k_busy_wait(1000000);  /* 1 second */
	uint32_t end_cycles = k_cycle_get_32();
	uint64_t total_cycles = end_cycles - start_cycles;
	cycles_per_us = total_cycles / 1000000;
	
	if (cycles_per_us == 0) {
		cycles_per_us = 1;
	}
	
	printk("Timing: %llu cycles/sec, %llu cycles/us\n\n", total_cycles, cycles_per_us);
	
	/* Create threads */
	k_thread_create(&critical_thread, critical_stack, CRITICAL_STACK_SIZE,
	                critical_thread_entry, NULL, NULL, NULL,
	                CRITICAL_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&critical_thread, "critical");
	
	k_thread_create(&important_thread, important_stack, IMPORTANT_STACK_SIZE,
	                important_thread_entry, NULL, NULL, NULL,
	                IMPORTANT_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&important_thread, "important");
	
	k_thread_create(&regular_thread, regular_stack, REGULAR_STACK_SIZE,
	                regular_thread_entry, NULL, NULL, NULL,
	                REGULAR_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&regular_thread, "regular");
	
	k_thread_create(&background_thread, background_stack, BACKGROUND_STACK_SIZE,
	                background_thread_entry, NULL, NULL, NULL,
	                BACKGROUND_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&background_thread, "background");
	
	/* Run test phases */
	for (int phase = 0; phase < NUM_PHASES; phase++) {
		printk("\n>>> ENTERING PHASE %d: %s <<<\n", phase, phase_names[phase]);
		current_phase = phase;
		phase_start_time = k_uptime_get();
		
		k_sleep(K_MSEC(PHASE_DURATION_MS));
		
		print_phase_stats(phase);
	}
	
	printk("\n");
	printk("=========================================\n");
	printk("=== Test Complete ===\n");
	printk("=========================================\n");
	
	printk("\nKey Findings:\n");
	printk("- Normal Load: Should have 0 deadline misses\n");
	printk("- Moderate Load: May have occasional misses\n");
	printk("- OVERLOAD: WILL have deadline misses (180%% util)\n");
	printk("- Recovery: Should return to 0 misses\n");
	printk("\nHigher priority tasks should miss fewer deadlines.\n");
	printk("Compare results across different schedulers!\n\n");
	
	return 0;
}
