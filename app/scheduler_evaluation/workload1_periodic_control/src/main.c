/*
 * Workload 1: Periodic Control System
 * 
 * Simulates an industrial/embedded control system with:
 * - High priority periodic sensor reading (10ms period)
 * - Medium priority control computation (20ms period)
 * - Low priority actuator output (50ms period)
 * - Background logging task
 * 
 * Metrics collected: latency, tardiness, throughput, deadline misses
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/timing/timing.h>

/* Thread stack sizes */
#define STACK_SIZE 2048
#define SENSOR_STACK_SIZE 1024
#define CONTROL_STACK_SIZE 2048
#define ACTUATOR_STACK_SIZE 1024
#define LOG_STACK_SIZE 1024

/* Thread priorities (lower number = higher priority in Zephyr) */
#define SENSOR_PRIORITY 1
#define CONTROL_PRIORITY 3
#define ACTUATOR_PRIORITY 5
#define LOG_PRIORITY 7

/* Periods in milliseconds */
#define SENSOR_PERIOD_MS 10
#define CONTROL_PERIOD_MS 20
#define ACTUATOR_PERIOD_MS 50

/* Simulated execution times in microseconds */
#define SENSOR_EXEC_US 2000    /* 2ms */
#define CONTROL_EXEC_US 5000   /* 5ms */
#define ACTUATOR_EXEC_US 3000  /* 3ms */

/* Test duration */
#define TEST_DURATION_MS 10000  /* 10 seconds */

/* Statistics structures */
struct thread_stats {
	uint32_t executions;
	uint32_t deadline_misses;
	uint64_t total_latency_us;
	uint64_t max_latency_us;
	uint64_t total_response_time_us;
	uint32_t preemptions;
};

static struct thread_stats sensor_stats = {0};
static struct thread_stats control_stats = {0};
static struct thread_stats actuator_stats = {0};
static struct thread_stats log_stats = {0};

/* Shared data (protected by mutex) */
static struct k_mutex data_mutex;
static int32_t sensor_data = 0;
static int32_t control_output = 0;

/* Thread stacks */
K_THREAD_STACK_DEFINE(sensor_stack, SENSOR_STACK_SIZE);
K_THREAD_STACK_DEFINE(control_stack, CONTROL_STACK_SIZE);
K_THREAD_STACK_DEFINE(actuator_stack, ACTUATOR_STACK_SIZE);
K_THREAD_STACK_DEFINE(log_stack, LOG_STACK_SIZE);

/* Thread structures */
static struct k_thread sensor_thread;
static struct k_thread control_thread;
static struct k_thread actuator_thread;
static struct k_thread log_thread;

/* Timing variables */
static uint64_t total_cycles;
static uint64_t cycles_per_us;

/* Helper function to simulate work (busy wait) */
static void simulate_work(uint32_t duration_us)
{
	timing_t start, end;
	uint64_t cycles_needed = duration_us * cycles_per_us;
	
	start = timing_counter_get();
	while (1) {
		end = timing_counter_get();
		uint64_t cycles = timing_cycles_get(&start, &end);
		if (cycles >= cycles_needed) {
			break;
		}
	}
}

/* High priority sensor thread - reads sensor every 10ms */
static void sensor_thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	int64_t period_ticks = k_ms_to_ticks_ceil64(SENSOR_PERIOD_MS);
	int64_t next_wakeup = k_uptime_ticks();
	
	printk("Sensor thread started (Priority: %d, Period: %dms)\n", 
	       SENSOR_PRIORITY, SENSOR_PERIOD_MS);

	while (1) {
		timing_t start_time = timing_counter_get();
		int64_t actual_wakeup = k_uptime_ticks();
		
		/* Calculate latency (how late we woke up) */
		int64_t latency_ticks = actual_wakeup - next_wakeup;
		if (latency_ticks < 0) latency_ticks = 0;
		
		uint64_t latency_us = k_ticks_to_us_ceil64(latency_ticks);
		sensor_stats.total_latency_us += latency_us;
		if (latency_us > sensor_stats.max_latency_us) {
			sensor_stats.max_latency_us = latency_us;
		}
		
		/* Simulate sensor reading */
		simulate_work(SENSOR_EXEC_US);
		
		/* Update shared sensor data */
		k_mutex_lock(&data_mutex, K_FOREVER);
		sensor_data = (sensor_data + 1) % 1000;
		k_mutex_unlock(&data_mutex);
		
		/* Calculate response time */
		timing_t end_time = timing_counter_get();
		uint64_t response_cycles = timing_cycles_get(&start_time, &end_time);
		uint64_t response_us = response_cycles / cycles_per_us;
		sensor_stats.total_response_time_us += response_us;
		
		/* Check for deadline miss (response time > period) */
		if (response_us > (SENSOR_PERIOD_MS * 1000)) {
			sensor_stats.deadline_misses++;
		}
		
		sensor_stats.executions++;
		
		/* Sleep until next period */
		next_wakeup += period_ticks;
		k_sleep(K_TIMEOUT_ABS_TICKS(next_wakeup));
	}
}

/* Medium priority control thread - processes control logic every 20ms */
static void control_thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	int64_t period_ticks = k_ms_to_ticks_ceil64(CONTROL_PERIOD_MS);
	int64_t next_wakeup = k_uptime_ticks();
	
	printk("Control thread started (Priority: %d, Period: %dms)\n", 
	       CONTROL_PRIORITY, CONTROL_PERIOD_MS);

	while (1) {
		timing_t start_time = timing_counter_get();
		int64_t actual_wakeup = k_uptime_ticks();
		
		int64_t latency_ticks = actual_wakeup - next_wakeup;
		if (latency_ticks < 0) latency_ticks = 0;
		
		uint64_t latency_us = k_ticks_to_us_ceil64(latency_ticks);
		control_stats.total_latency_us += latency_us;
		if (latency_us > control_stats.max_latency_us) {
			control_stats.max_latency_us = latency_us;
		}
		
		/* Read sensor data and compute control output */
		k_mutex_lock(&data_mutex, K_FOREVER);
		int32_t current_sensor = sensor_data;
		k_mutex_unlock(&data_mutex);
		
		/* Simulate control computation (PID controller, etc.) */
		simulate_work(CONTROL_EXEC_US);
		
		/* Update control output */
		k_mutex_lock(&data_mutex, K_FOREVER);
		control_output = current_sensor * 2;  /* Simple computation */
		k_mutex_unlock(&data_mutex);
		
		timing_t end_time = timing_counter_get();
		uint64_t response_cycles = timing_cycles_get(&start_time, &end_time);
		uint64_t response_us = response_cycles / cycles_per_us;
		control_stats.total_response_time_us += response_us;
		
		if (response_us > (CONTROL_PERIOD_MS * 1000)) {
			control_stats.deadline_misses++;
		}
		
		control_stats.executions++;
		
		next_wakeup += period_ticks;
		k_sleep(K_TIMEOUT_ABS_TICKS(next_wakeup));
	}
}

/* Low priority actuator thread - outputs to actuators every 50ms */
static void actuator_thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	int64_t period_ticks = k_ms_to_ticks_ceil64(ACTUATOR_PERIOD_MS);
	int64_t next_wakeup = k_uptime_ticks();
	
	printk("Actuator thread started (Priority: %d, Period: %dms)\n", 
	       ACTUATOR_PRIORITY, ACTUATOR_PERIOD_MS);

	while (1) {
		timing_t start_time = timing_counter_get();
		int64_t actual_wakeup = k_uptime_ticks();
		
		int64_t latency_ticks = actual_wakeup - next_wakeup;
		if (latency_ticks < 0) latency_ticks = 0;
		
		uint64_t latency_us = k_ticks_to_us_ceil64(latency_ticks);
		actuator_stats.total_latency_us += latency_us;
		if (latency_us > actuator_stats.max_latency_us) {
			actuator_stats.max_latency_us = latency_us;
		}
		
		/* Read control output - value not used but shows data flow */
		k_mutex_lock(&data_mutex, K_FOREVER);
		(void)control_output;  /* Suppress unused variable warning */
		k_mutex_unlock(&data_mutex);
		
		/* Simulate actuator control */
		simulate_work(ACTUATOR_EXEC_US);
		
		timing_t end_time = timing_counter_get();
		uint64_t response_cycles = timing_cycles_get(&start_time, &end_time);
		uint64_t response_us = response_cycles / cycles_per_us;
		actuator_stats.total_response_time_us += response_us;
		
		if (response_us > (ACTUATOR_PERIOD_MS * 1000)) {
			actuator_stats.deadline_misses++;
		}
		
		actuator_stats.executions++;
		
		next_wakeup += period_ticks;
		k_sleep(K_TIMEOUT_ABS_TICKS(next_wakeup));
	}
}

/* Background logging thread - best effort */
static void log_thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	printk("Background logging thread started (Priority: %d)\n", LOG_PRIORITY);

	while (1) {
		timing_t start_time = timing_counter_get();
		
		/* Simulate logging activity */
		k_sleep(K_MSEC(100));
		simulate_work(1000);  /* 1ms of work */
		
		timing_t end_time = timing_counter_get();
		uint64_t response_cycles = timing_cycles_get(&start_time, &end_time);
		uint64_t response_us = response_cycles / cycles_per_us;
		log_stats.total_response_time_us += response_us;
		log_stats.executions++;
		
		k_yield();
	}
}

/* Print statistics */
static void print_statistics(void)
{
	printk("\n=== Workload 1: Periodic Control System Results ===\n\n");
	
	printk("Sensor Thread (High Priority, Period: %dms):\n", SENSOR_PERIOD_MS);
	printk("  Executions: %u\n", sensor_stats.executions);
	printk("  Deadline Misses: %u\n", sensor_stats.deadline_misses);
	printk("  Avg Latency: %llu us\n", 
	       sensor_stats.executions > 0 ? sensor_stats.total_latency_us / sensor_stats.executions : 0);
	printk("  Max Latency: %llu us\n", sensor_stats.max_latency_us);
	printk("  Avg Response Time: %llu us\n",
	       sensor_stats.executions > 0 ? sensor_stats.total_response_time_us / sensor_stats.executions : 0);
	printk("  Tardiness Rate: %.2f%%\n\n", 
	       sensor_stats.executions > 0 ? (100.0 * sensor_stats.deadline_misses / sensor_stats.executions) : 0);
	
	printk("Control Thread (Medium Priority, Period: %dms):\n", CONTROL_PERIOD_MS);
	printk("  Executions: %u\n", control_stats.executions);
	printk("  Deadline Misses: %u\n", control_stats.deadline_misses);
	printk("  Avg Latency: %llu us\n",
	       control_stats.executions > 0 ? control_stats.total_latency_us / control_stats.executions : 0);
	printk("  Max Latency: %llu us\n", control_stats.max_latency_us);
	printk("  Avg Response Time: %llu us\n",
	       control_stats.executions > 0 ? control_stats.total_response_time_us / control_stats.executions : 0);
	printk("  Tardiness Rate: %.2f%%\n\n",
	       control_stats.executions > 0 ? (100.0 * control_stats.deadline_misses / control_stats.executions) : 0);
	
	printk("Actuator Thread (Low Priority, Period: %dms):\n", ACTUATOR_PERIOD_MS);
	printk("  Executions: %u\n", actuator_stats.executions);
	printk("  Deadline Misses: %u\n", actuator_stats.deadline_misses);
	printk("  Avg Latency: %llu us\n",
	       actuator_stats.executions > 0 ? actuator_stats.total_latency_us / actuator_stats.executions : 0);
	printk("  Max Latency: %llu us\n", actuator_stats.max_latency_us);
	printk("  Avg Response Time: %llu us\n",
	       actuator_stats.executions > 0 ? actuator_stats.total_response_time_us / actuator_stats.executions : 0);
	printk("  Tardiness Rate: %.2f%%\n\n",
	       actuator_stats.executions > 0 ? (100.0 * actuator_stats.deadline_misses / actuator_stats.executions) : 0);
	
	printk("Background Logging Thread:\n");
	printk("  Executions: %u\n", log_stats.executions);
	printk("  Avg Response Time: %llu us\n\n",
	       log_stats.executions > 0 ? log_stats.total_response_time_us / log_stats.executions : 0);
	
	uint32_t total_executions = sensor_stats.executions + control_stats.executions + 
	                            actuator_stats.executions + log_stats.executions;
	printk("Total Throughput: %u task executions in %d seconds\n", 
	       total_executions, TEST_DURATION_MS / 1000);
	printk("Executions per second: %u\n", total_executions / (TEST_DURATION_MS / 1000));
}

int main(void)
{
	printk("\n=== Workload 1: Periodic Control System ===\n");
	printk("Testing scheduler with periodic real-time tasks\n");
	printk("Duration: %d seconds\n\n", TEST_DURATION_MS / 1000);
	
	/* Initialize timing */
	timing_init();
	timing_start();
	
	timing_t start = timing_counter_get();
	timing_t end;
	k_busy_wait(1000000);  /* 1 second */
	end = timing_counter_get();
	total_cycles = timing_cycles_get(&start, &end);
	cycles_per_us = total_cycles / 1000000;
	
	printk("Timing calibration: %llu cycles per second\n", total_cycles);
	printk("Cycles per microsecond: %llu\n\n", cycles_per_us);
	
	/* Initialize mutex */
	k_mutex_init(&data_mutex);
	
	/* Create threads */
	k_thread_create(&sensor_thread, sensor_stack, SENSOR_STACK_SIZE,
	                sensor_thread_entry, NULL, NULL, NULL,
	                SENSOR_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&sensor_thread, "sensor");
	
	k_thread_create(&control_thread, control_stack, CONTROL_STACK_SIZE,
	                control_thread_entry, NULL, NULL, NULL,
	                CONTROL_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&control_thread, "control");
	
	k_thread_create(&actuator_thread, actuator_stack, ACTUATOR_STACK_SIZE,
	                actuator_thread_entry, NULL, NULL, NULL,
	                ACTUATOR_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&actuator_thread, "actuator");
	
	k_thread_create(&log_thread, log_stack, LOG_STACK_SIZE,
	                log_thread_entry, NULL, NULL, NULL,
	                LOG_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&log_thread, "logger");
	
	/* Run test for specified duration */
	k_sleep(K_MSEC(TEST_DURATION_MS));
	
	/* Print results */
	print_statistics();
	
	printk("\nTest completed.\n");
	
	return 0;
}
