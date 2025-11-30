/*
 * Workload 3: Mixed Criticality System
 * 
 * Simulates a mixed-criticality system (e.g., avionics, medical devices) with:
 * - Safety-critical monitoring (must NEVER miss deadline)
 * - High priority mission function (occasional overruns acceptable)
 * - Medium priority user interface (soft real-time)
 * - Low priority diagnostics (best effort)
 * 
 * Tests: overload handling, criticality levels, mode changes, graceful degradation
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/timing/timing.h>
#include <zephyr/random/random.h>

/* Thread stack sizes */
#define SAFETY_STACK_SIZE 1024
#define MISSION_STACK_SIZE 2048
#define UI_STACK_SIZE 2048
#define DIAG_STACK_SIZE 1024

/* Thread priorities */
#define SAFETY_PRIORITY 0     /* Highest - safety critical */
#define MISSION_PRIORITY 2    /* High - mission critical */
#define UI_PRIORITY 4         /* Medium - user interface */
#define DIAG_PRIORITY 6       /* Low - diagnostics */

/* Periods in milliseconds */
#define SAFETY_PERIOD_MS 10
#define MISSION_PERIOD_MS 20
#define UI_PERIOD_MS 100

/* Execution times in microseconds */
#define SAFETY_EXEC_US 1000      /* 1ms - very predictable */
#define MISSION_EXEC_US_NORMAL 5000    /* 5ms normal */
#define MISSION_EXEC_US_OVERLOAD 15000  /* 15ms when overloaded */
#define UI_EXEC_US 8000          /* 8ms */
#define DIAG_EXEC_US 10000       /* 10ms */

/* Test configuration */
#define TEST_DURATION_MS 15000
#define OVERLOAD_START_MS 5000    /* Start overload at 5 seconds */
#define OVERLOAD_END_MS 10000     /* End overload at 10 seconds */
#define MODE_CHANGE_AT_MS 7500    /* Trigger mode change */

/* System modes */
enum system_mode {
	MODE_NORMAL,
	MODE_DEGRADED,    /* Under overload, shed low-priority tasks */
	MODE_CRITICAL     /* Only safety-critical tasks */
};

static enum system_mode current_mode = MODE_NORMAL;
static bool system_overloaded = false;

/* Statistics */
struct task_stats {
	uint32_t activations;
	uint32_t completions;
	uint32_t deadline_misses;
	uint32_t preemptions;
	uint64_t total_latency_us;
	uint64_t max_latency_us;
	uint64_t total_response_time_us;
	uint64_t max_response_time_us;
	uint32_t shed_count;  /* Times task was shed due to mode change */
};

static struct task_stats safety_stats = {0};
static struct task_stats mission_stats = {0};
static struct task_stats ui_stats = {0};
static struct task_stats diag_stats = {0};

/* Mode change statistics */
static uint32_t mode_changes = 0;
static uint32_t total_deadline_misses = 0;

/* Thread structures */
K_THREAD_STACK_DEFINE(safety_stack, SAFETY_STACK_SIZE);
K_THREAD_STACK_DEFINE(mission_stack, MISSION_STACK_SIZE);
K_THREAD_STACK_DEFINE(ui_stack, UI_STACK_SIZE);
K_THREAD_STACK_DEFINE(diag_stack, DIAG_STACK_SIZE);

static struct k_thread safety_thread;
static struct k_thread mission_thread;
static struct k_thread ui_thread;
static struct k_thread diag_thread;

/* Synchronization */
static struct k_mutex mode_mutex;
static struct k_sem mode_change_sem;

/* Timing */
static uint64_t cycles_per_us;
static int64_t test_start_time;

/* Thread stacks and data */
K_THREAD_STACK_DEFINE(mode_mgr_stack, 1024);
static struct k_thread mode_mgr_thread;

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

/* Check if task should be shed based on current mode */
static bool should_shed_task(int priority)
{
	k_mutex_lock(&mode_mutex, K_FOREVER);
	enum system_mode mode = current_mode;
	k_mutex_unlock(&mode_mutex);
	
	switch (mode) {
	case MODE_NORMAL:
		return false;
	case MODE_DEGRADED:
		/* Shed diagnostics in degraded mode */
		return (priority >= DIAG_PRIORITY);
	case MODE_CRITICAL:
		/* Only safety-critical tasks in critical mode */
		return (priority > SAFETY_PRIORITY);
	default:
		return false;
	}
}

/* Safety-critical monitoring thread - MUST NEVER miss deadline */
static void safety_monitor_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	int64_t period_ticks = k_ms_to_ticks_ceil64(SAFETY_PERIOD_MS);
	int64_t next_wakeup = k_uptime_ticks();
	
	printk("Safety Monitor started (Priority: %d, Period: %dms) - MUST NEVER MISS DEADLINE\n",
	       SAFETY_PRIORITY, SAFETY_PERIOD_MS);

	while (1) {
		timing_t start_time = timing_counter_get();
		int64_t actual_wakeup = k_uptime_ticks();
		
		safety_stats.activations++;
		
		/* Check if we should be shed (should never happen for safety!) */
		if (should_shed_task(SAFETY_PRIORITY)) {
			safety_stats.shed_count++;
			printk("ERROR: Safety task attempted to be shed!\n");
		}
		
		/* Calculate latency */
		int64_t latency_ticks = actual_wakeup - next_wakeup;
		if (latency_ticks < 0) latency_ticks = 0;
		uint64_t latency_us = k_ticks_to_us_ceil64(latency_ticks);
		
		safety_stats.total_latency_us += latency_us;
		if (latency_us > safety_stats.max_latency_us) {
			safety_stats.max_latency_us = latency_us;
		}
		
		/* Perform safety monitoring - highly predictable execution */
		simulate_work(SAFETY_EXEC_US);
		
		/* Calculate response time */
		timing_t end_time = timing_counter_get();
		uint64_t response_us = timing_cycles_get(&start_time, &end_time) / cycles_per_us;
		safety_stats.total_response_time_us += response_us;
		if (response_us > safety_stats.max_response_time_us) {
			safety_stats.max_response_time_us = response_us;
		}
		
		/* Check deadline - THIS SHOULD NEVER HAPPEN */
		if (response_us > (SAFETY_PERIOD_MS * 1000)) {
			safety_stats.deadline_misses++;
			total_deadline_misses++;
			printk("CRITICAL: Safety monitor missed deadline! Response: %llu us\n", response_us);
		}
		
		safety_stats.completions++;
		
		next_wakeup += period_ticks;
		k_sleep(K_TIMEOUT_ABS_TICKS(next_wakeup));
	}
}

/* High priority mission function - occasional overruns acceptable */
static void mission_function_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	int64_t period_ticks = k_ms_to_ticks_ceil64(MISSION_PERIOD_MS);
	int64_t next_wakeup = k_uptime_ticks();
	
	printk("Mission Function started (Priority: %d, Period: %dms)\n",
	       MISSION_PRIORITY, MISSION_PERIOD_MS);

	while (1) {
		timing_t start_time = timing_counter_get();
		int64_t actual_wakeup = k_uptime_ticks();
		
		mission_stats.activations++;
		
		/* Check if we should be shed */
		if (should_shed_task(MISSION_PRIORITY)) {
			mission_stats.shed_count++;
			next_wakeup += period_ticks;
			k_sleep(K_TIMEOUT_ABS_TICKS(next_wakeup));
			continue;
		}
		
		int64_t latency_ticks = actual_wakeup - next_wakeup;
		if (latency_ticks < 0) latency_ticks = 0;
		uint64_t latency_us = k_ticks_to_us_ceil64(latency_ticks);
		
		mission_stats.total_latency_us += latency_us;
		if (latency_us > mission_stats.max_latency_us) {
			mission_stats.max_latency_us = latency_us;
		}
		
		/* Execution time varies with system load */
		uint32_t exec_time = system_overloaded ? MISSION_EXEC_US_OVERLOAD : MISSION_EXEC_US_NORMAL;
		simulate_work(exec_time);
		
		timing_t end_time = timing_counter_get();
		uint64_t response_us = timing_cycles_get(&start_time, &end_time) / cycles_per_us;
		mission_stats.total_response_time_us += response_us;
		if (response_us > mission_stats.max_response_time_us) {
			mission_stats.max_response_time_us = response_us;
		}
		
		/* Deadline miss acceptable but tracked */
		if (response_us > (MISSION_PERIOD_MS * 1000)) {
			mission_stats.deadline_misses++;
			total_deadline_misses++;
		}
		
		mission_stats.completions++;
		
		next_wakeup += period_ticks;
		k_sleep(K_TIMEOUT_ABS_TICKS(next_wakeup));
	}
}

/* User interface thread - soft real-time */
static void ui_thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	int64_t period_ticks = k_ms_to_ticks_ceil64(UI_PERIOD_MS);
	int64_t next_wakeup = k_uptime_ticks();
	
	printk("User Interface started (Priority: %d, Period: %dms)\n",
	       UI_PRIORITY, UI_PERIOD_MS);

	while (1) {
		timing_t start_time = timing_counter_get();
		int64_t actual_wakeup = k_uptime_ticks();
		
		ui_stats.activations++;
		
		/* Check if we should be shed */
		if (should_shed_task(UI_PRIORITY)) {
			ui_stats.shed_count++;
			next_wakeup += period_ticks;
			k_sleep(K_TIMEOUT_ABS_TICKS(next_wakeup));
			continue;
		}
		
		int64_t latency_ticks = actual_wakeup - next_wakeup;
		if (latency_ticks < 0) latency_ticks = 0;
		uint64_t latency_us = k_ticks_to_us_ceil64(latency_ticks);
		
		ui_stats.total_latency_us += latency_us;
		if (latency_us > ui_stats.max_latency_us) {
			ui_stats.max_latency_us = latency_us;
		}
		
		/* UI processing */
		simulate_work(UI_EXEC_US);
		
		timing_t end_time = timing_counter_get();
		uint64_t response_us = timing_cycles_get(&start_time, &end_time) / cycles_per_us;
		ui_stats.total_response_time_us += response_us;
		if (response_us > ui_stats.max_response_time_us) {
			ui_stats.max_response_time_us = response_us;
		}
		
		/* Soft deadline */
		if (response_us > (UI_PERIOD_MS * 1000)) {
			ui_stats.deadline_misses++;
			total_deadline_misses++;
		}
		
		ui_stats.completions++;
		
		next_wakeup += period_ticks;
		k_sleep(K_TIMEOUT_ABS_TICKS(next_wakeup));
	}
}

/* Diagnostics thread - best effort, can be preempted/shed */
static void diagnostics_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	printk("Diagnostics started (Priority: %d, Best Effort)\n", DIAG_PRIORITY);

	while (1) {
		timing_t start_time = timing_counter_get();
		
		diag_stats.activations++;
		
		/* Check if we should be shed */
		if (should_shed_task(DIAG_PRIORITY)) {
			diag_stats.shed_count++;
			k_sleep(K_MSEC(500));
			continue;
		}
		
		/* Diagnostic processing */
		simulate_work(DIAG_EXEC_US);
		
		timing_t end_time = timing_counter_get();
		uint64_t response_us = timing_cycles_get(&start_time, &end_time) / cycles_per_us;
		diag_stats.total_response_time_us += response_us;
		if (response_us > diag_stats.max_response_time_us) {
			diag_stats.max_response_time_us = response_us;
		}
		
		diag_stats.completions++;
		
		/* Variable sleep */
		k_sleep(K_MSEC(100 + (sys_rand32_get() % 200)));
	}
}

/* Mode manager thread */
static void mode_manager(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (1) {
		int64_t elapsed = k_uptime_get() - test_start_time;
		
		/* Simulate overload condition */
		if (elapsed >= OVERLOAD_START_MS && elapsed < OVERLOAD_END_MS) {
			if (!system_overloaded) {
				system_overloaded = true;
				printk("\n>>> OVERLOAD CONDITION STARTED <<<\n");
			}
		} else {
			if (system_overloaded) {
				system_overloaded = false;
				printk("\n>>> OVERLOAD CONDITION ENDED <<<\n");
			}
		}
		
		/* Mode change trigger */
		if (elapsed >= MODE_CHANGE_AT_MS && elapsed < (MODE_CHANGE_AT_MS + 100)) {
			k_mutex_lock(&mode_mutex, K_FOREVER);
			if (current_mode == MODE_NORMAL) {
				current_mode = MODE_DEGRADED;
				mode_changes++;
				printk("\n>>> MODE CHANGE: NORMAL -> DEGRADED <<<\n");
			}
			k_mutex_unlock(&mode_mutex);
		}
		
		/* Monitor deadline misses and switch to critical mode if needed */
		if (safety_stats.deadline_misses > 0) {
			k_mutex_lock(&mode_mutex, K_FOREVER);
			if (current_mode != MODE_CRITICAL) {
				current_mode = MODE_CRITICAL;
				mode_changes++;
				printk("\n>>> EMERGENCY MODE CHANGE: -> CRITICAL <<<\n");
			}
			k_mutex_unlock(&mode_mutex);
		}
		
		k_sleep(K_MSEC(100));
	}
}

/* Print statistics */
static void print_statistics(void)
{
	printk("\n=== Workload 3: Mixed Criticality System Results ===\n\n");
	
	printk("System Mode Changes: %u\n", mode_changes);
	printk("Total Deadline Misses: %u\n\n", total_deadline_misses);
	
	printk("Safety Monitor (CRITICAL - Must Never Miss):\n");
	printk("  Activations: %u\n", safety_stats.activations);
	printk("  Completions: %u\n", safety_stats.completions);
	printk("  Deadline Misses: %u <-- MUST BE ZERO!\n", safety_stats.deadline_misses);
	printk("  Times Shed: %u\n", safety_stats.shed_count);
	printk("  Avg/Max Latency: %llu / %llu us\n",
	       safety_stats.completions > 0 ? safety_stats.total_latency_us / safety_stats.completions : 0,
	       safety_stats.max_latency_us);
	printk("  Avg/Max Response: %llu / %llu us\n\n",
	       safety_stats.completions > 0 ? safety_stats.total_response_time_us / safety_stats.completions : 0,
	       safety_stats.max_response_time_us);
	
	printk("Mission Function (HIGH - Occasional Miss OK):\n");
	printk("  Activations: %u\n", mission_stats.activations);
	printk("  Completions: %u\n", mission_stats.completions);
	printk("  Deadline Misses: %u\n", mission_stats.deadline_misses);
	printk("  Times Shed: %u\n", mission_stats.shed_count);
	printk("  Tardiness Rate: %.2f%%\n",
	       mission_stats.activations > 0 ? (100.0 * mission_stats.deadline_misses / mission_stats.activations) : 0);
	printk("  Avg/Max Latency: %llu / %llu us\n",
	       mission_stats.completions > 0 ? mission_stats.total_latency_us / mission_stats.completions : 0,
	       mission_stats.max_latency_us);
	printk("  Avg/Max Response: %llu / %llu us\n\n",
	       mission_stats.completions > 0 ? mission_stats.total_response_time_us / mission_stats.completions : 0,
	       mission_stats.max_response_time_us);
	
	printk("User Interface (MEDIUM - Soft Real-Time):\n");
	printk("  Activations: %u\n", ui_stats.activations);
	printk("  Completions: %u\n", ui_stats.completions);
	printk("  Deadline Misses: %u\n", ui_stats.deadline_misses);
	printk("  Times Shed: %u\n", ui_stats.shed_count);
	printk("  Avg/Max Response: %llu / %llu us\n\n",
	       ui_stats.completions > 0 ? ui_stats.total_response_time_us / ui_stats.completions : 0,
	       ui_stats.max_response_time_us);
	
	printk("Diagnostics (LOW - Best Effort):\n");
	printk("  Activations: %u\n", diag_stats.activations);
	printk("  Completions: %u\n", diag_stats.completions);
	printk("  Times Shed: %u\n", diag_stats.shed_count);
	printk("  Avg Response: %llu us\n\n",
	       diag_stats.completions > 0 ? diag_stats.total_response_time_us / diag_stats.completions : 0);
	
	uint32_t total_completions = safety_stats.completions + mission_stats.completions +
	                             ui_stats.completions + diag_stats.completions;
	printk("Total Task Completions: %u\n", total_completions);
	printk("System Schedulability: %s\n",
	       safety_stats.deadline_misses == 0 ? "SAFE" : "UNSAFE - CRITICAL TASK MISSED DEADLINE!");
}

int main(void)
{
	printk("\n=== Workload 3: Mixed Criticality System ===\n");
	printk("Testing scheduler with multiple criticality levels and mode changes\n");
	printk("Duration: %d seconds\n\n", TEST_DURATION_MS / 1000);
	
	test_start_time = k_uptime_get();
	
	/* Initialize timing */
	timing_init();
	timing_start();
	
	timing_t start = timing_counter_get();
	k_busy_wait(1000000);
	timing_t end = timing_counter_get();
	uint64_t total_cycles = timing_cycles_get(&start, &end);
	cycles_per_us = total_cycles / 1000000;
	
	printk("Timing calibration: %llu cycles/second, %llu cycles/us\n\n", total_cycles, cycles_per_us);
	
	/* Initialize synchronization */
	k_mutex_init(&mode_mutex);
	k_sem_init(&mode_change_sem, 0, 1);
	
	/* Create threads in priority order */
	k_thread_create(&safety_thread, safety_stack, SAFETY_STACK_SIZE,
	                safety_monitor_entry, NULL, NULL, NULL,
	                SAFETY_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&safety_thread, "safety");
	
	k_thread_create(&mission_thread, mission_stack, MISSION_STACK_SIZE,
	                mission_function_entry, NULL, NULL, NULL,
	                MISSION_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&mission_thread, "mission");
	
	k_thread_create(&ui_thread, ui_stack, UI_STACK_SIZE,
	                ui_thread_entry, NULL, NULL, NULL,
	                UI_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&ui_thread, "ui");
	
	k_thread_create(&diag_thread, diag_stack, DIAG_STACK_SIZE,
	                diagnostics_entry, NULL, NULL, NULL,
	                DIAG_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&diag_thread, "diagnostics");
	
	/* Create mode manager thread */
	k_thread_create(&mode_mgr_thread, mode_mgr_stack, 1024,
	                mode_manager, NULL, NULL, NULL,
	                7, 0, K_NO_WAIT);
	k_thread_name_set(&mode_mgr_thread, "mode_mgr");
	
	/* Run test */
	k_sleep(K_MSEC(TEST_DURATION_MS));
	
	/* Print results */
	print_statistics();
	
	printk("\nTest completed.\n");
	
	return 0;
}
