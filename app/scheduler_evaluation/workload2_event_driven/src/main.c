/*
 * Workload 2: Event-Driven Communication System
 * 
 * Simulates a network/communication system with:
 * - Critical interrupt handler (highest priority, sporadic)
 * - High priority packet processing (sporadic arrivals)
 * - Medium priority protocol handler (variable execution)
 * - Low priority background transmission (bulk data)
 * 
 * Tests: interrupt latency, priority inversion, sporadic task handling
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/timing/timing.h>
#include <zephyr/random/random.h>

/* Thread stack sizes */
#define IRQ_HANDLER_STACK_SIZE 1024
#define PACKET_PROC_STACK_SIZE 2048
#define PROTOCOL_STACK_SIZE 2048
#define BULK_TX_STACK_SIZE 1024

/* Thread priorities */
#define IRQ_HANDLER_PRIORITY 0    /* Critical - highest */
#define PACKET_PROC_PRIORITY 2    /* High */
#define PROTOCOL_PRIORITY 4       /* Medium */
#define BULK_TX_PRIORITY 6        /* Low */

/* Simulated execution times in microseconds */
#define IRQ_HANDLER_EXEC_US 500      /* <1ms - very fast */
#define PACKET_PROC_EXEC_US 3000     /* 3ms */
#define PROTOCOL_EXEC_US_MIN 2000    /* 2-8ms variable */
#define PROTOCOL_EXEC_US_MAX 8000
#define BULK_TX_EXEC_US 5000         /* 5ms */

/* Deadlines in microseconds */
#define IRQ_HANDLER_DEADLINE_US 1000    /* 1ms hard deadline */
#define PACKET_PROC_DEADLINE_US 5000    /* 5ms deadline */

/* Event arrival parameters (Poisson-like) */
#define IRQ_EVENT_MEAN_INTERVAL_MS 15    /* Average 15ms between interrupts */
#define PACKET_ARRIVAL_MEAN_INTERVAL_MS 8  /* Average 8ms between packets */

/* Test duration */
#define TEST_DURATION_MS 10000

/* Statistics */
struct event_stats {
	uint32_t events_generated;
	uint32_t events_processed;
	uint64_t total_latency_us;
	uint64_t max_latency_us;
	uint64_t min_latency_us;
	uint64_t total_response_time_us;
	uint32_t deadline_misses;
	uint32_t priority_inversions;
};

static struct event_stats irq_stats = { .min_latency_us = UINT64_MAX };
static struct event_stats packet_stats = { .min_latency_us = UINT64_MAX };
static struct event_stats protocol_stats = { .min_latency_us = UINT64_MAX };
static struct event_stats bulk_stats = { .min_latency_us = UINT64_MAX };

/* Communication queues */
#define QUEUE_SIZE 20
K_MSGQ_DEFINE(irq_queue, sizeof(uint64_t), QUEUE_SIZE, 4);
K_MSGQ_DEFINE(packet_queue, sizeof(uint64_t), QUEUE_SIZE, 4);
K_MSGQ_DEFINE(protocol_queue, sizeof(uint64_t), QUEUE_SIZE, 4);

/* Shared resources with mutex (simulates critical section) */
static struct k_mutex resource_mutex;
static uint32_t shared_resource_counter = 0;

/* Thread structures */
K_THREAD_STACK_DEFINE(irq_handler_stack, IRQ_HANDLER_STACK_SIZE);
K_THREAD_STACK_DEFINE(packet_proc_stack, PACKET_PROC_STACK_SIZE);
K_THREAD_STACK_DEFINE(protocol_stack, PROTOCOL_STACK_SIZE);
K_THREAD_STACK_DEFINE(bulk_tx_stack, BULK_TX_STACK_SIZE);

static struct k_thread irq_handler_thread;
static struct k_thread packet_proc_thread;
static struct k_thread protocol_thread;
static struct k_thread bulk_tx_thread;

/* Event generator thread structures */
K_THREAD_STACK_DEFINE(irq_gen_stack, 1024);
K_THREAD_STACK_DEFINE(packet_gen_stack, 1024);
static struct k_thread irq_gen_thread;
static struct k_thread packet_gen_thread;

/* Timing */
static uint64_t cycles_per_us;

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

/* Generate random interval using exponential distribution approximation */
static uint32_t get_random_interval(uint32_t mean_ms)
{
	/* Simple approximation: uniform distribution around mean */
	uint32_t variance = mean_ms / 2;
	uint32_t random_val = sys_rand32_get() % (2 * variance);
	uint32_t interval = (mean_ms > variance) ? (mean_ms - variance + random_val) : random_val;
	return (interval < 1) ? 1 : interval;
}

/* Critical interrupt handler thread */
static void irq_handler_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	printk("IRQ Handler thread started (Priority: %d, Deadline: %d us)\n",
	       IRQ_HANDLER_PRIORITY, IRQ_HANDLER_DEADLINE_US);

	while (1) {
		uint64_t timestamp;
		
		/* Wait for interrupt event */
		if (k_msgq_get(&irq_queue, &timestamp, K_FOREVER) == 0) {
			timing_t start_time = timing_counter_get();
			
			/* Calculate latency */
			timing_t current = timing_counter_get();
			uint64_t latency_us = (current - timestamp) / cycles_per_us;
			
			irq_stats.total_latency_us += latency_us;
			if (latency_us > irq_stats.max_latency_us) {
				irq_stats.max_latency_us = latency_us;
			}
			if (latency_us < irq_stats.min_latency_us) {
				irq_stats.min_latency_us = latency_us;
			}
			
			/* Critical interrupt processing */
			simulate_work(IRQ_HANDLER_EXEC_US);
			
			/* Access shared resource briefly */
			k_mutex_lock(&resource_mutex, K_FOREVER);
			shared_resource_counter++;
			k_mutex_unlock(&resource_mutex);
			
			/* Send packet to next stage */
			uint64_t packet_ts = timing_counter_get();
			k_msgq_put(&packet_queue, &packet_ts, K_NO_WAIT);
			
			/* Calculate response time */
			timing_t end_time = timing_counter_get();
			uint64_t response_us = timing_cycles_get(&start_time, &end_time) / cycles_per_us;
			irq_stats.total_response_time_us += response_us;
			
			/* Check deadline */
			if (response_us > IRQ_HANDLER_DEADLINE_US) {
				irq_stats.deadline_misses++;
			}
			
			irq_stats.events_processed++;
		}
	}
}

/* High priority packet processing thread */
static void packet_proc_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	printk("Packet Processor thread started (Priority: %d, Deadline: %d us)\n",
	       PACKET_PROC_PRIORITY, PACKET_PROC_DEADLINE_US);

	while (1) {
		uint64_t timestamp;
		
		if (k_msgq_get(&packet_queue, &timestamp, K_FOREVER) == 0) {
			timing_t start_time = timing_counter_get();
			
			/* Calculate latency */
			uint64_t latency_us = (start_time - timestamp) / cycles_per_us;
			packet_stats.total_latency_us += latency_us;
			if (latency_us > packet_stats.max_latency_us) {
				packet_stats.max_latency_us = latency_us;
			}
			if (latency_us < packet_stats.min_latency_us) {
				packet_stats.min_latency_us = latency_us;
			}
			
			/* Process packet */
			simulate_work(PACKET_PROC_EXEC_US);
			
			/* Send to protocol handler */
			uint64_t proto_ts = timing_counter_get();
			k_msgq_put(&protocol_queue, &proto_ts, K_NO_WAIT);
			
			/* Calculate response time */
			timing_t end_time = timing_counter_get();
			uint64_t response_us = timing_cycles_get(&start_time, &end_time) / cycles_per_us;
			packet_stats.total_response_time_us += response_us;
			
			if (response_us > PACKET_PROC_DEADLINE_US) {
				packet_stats.deadline_misses++;
			}
			
			packet_stats.events_processed++;
		}
	}
}

/* Medium priority protocol handler - variable execution time */
static void protocol_handler_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	printk("Protocol Handler thread started (Priority: %d)\n", PROTOCOL_PRIORITY);

	while (1) {
		uint64_t timestamp;
		
		if (k_msgq_get(&protocol_queue, &timestamp, K_FOREVER) == 0) {
			timing_t start_time = timing_counter_get();
			
			uint64_t latency_us = (start_time - timestamp) / cycles_per_us;
			protocol_stats.total_latency_us += latency_us;
			if (latency_us > protocol_stats.max_latency_us) {
				protocol_stats.max_latency_us = latency_us;
			}
			if (latency_us < protocol_stats.min_latency_us) {
				protocol_stats.min_latency_us = latency_us;
			}
			
			/* Variable processing time */
			uint32_t exec_time = PROTOCOL_EXEC_US_MIN + 
			                     (sys_rand32_get() % (PROTOCOL_EXEC_US_MAX - PROTOCOL_EXEC_US_MIN));
			
			/* May need shared resource - potential priority inversion */
			k_mutex_lock(&resource_mutex, K_FOREVER);
			simulate_work(exec_time / 2);
			shared_resource_counter++;
			k_mutex_unlock(&resource_mutex);
			
			simulate_work(exec_time / 2);
			
			timing_t end_time = timing_counter_get();
			uint64_t response_us = timing_cycles_get(&start_time, &end_time) / cycles_per_us;
			protocol_stats.total_response_time_us += response_us;
			protocol_stats.events_processed++;
		}
	}
}

/* Low priority bulk transmission thread */
static void bulk_tx_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	printk("Bulk TX thread started (Priority: %d)\n", BULK_TX_PRIORITY);

	while (1) {
		timing_t start_time = timing_counter_get();
		
		/* Simulate bulk data transmission */
		/* This thread holds the mutex longer - can cause priority inversion */
		k_mutex_lock(&resource_mutex, K_FOREVER);
		simulate_work(BULK_TX_EXEC_US);
		shared_resource_counter++;
		k_mutex_unlock(&resource_mutex);
		
		timing_t end_time = timing_counter_get();
		uint64_t response_us = timing_cycles_get(&start_time, &end_time) / cycles_per_us;
		bulk_stats.total_response_time_us += response_us;
		bulk_stats.events_processed++;
		
		/* Sleep a bit to allow other threads */
		k_sleep(K_MSEC(20));
	}
}

/* Event generators */
static void irq_event_generator(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	printk("IRQ Event Generator started (Mean interval: %d ms)\n", IRQ_EVENT_MEAN_INTERVAL_MS);

	while (1) {
		uint32_t interval = get_random_interval(IRQ_EVENT_MEAN_INTERVAL_MS);
		k_sleep(K_MSEC(interval));
		
		uint64_t timestamp = timing_counter_get();
		if (k_msgq_put(&irq_queue, &timestamp, K_NO_WAIT) == 0) {
			irq_stats.events_generated++;
		}
	}
}

static void packet_event_generator(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	printk("Packet Event Generator started (Mean interval: %d ms)\n", PACKET_ARRIVAL_MEAN_INTERVAL_MS);

	while (1) {
		uint32_t interval = get_random_interval(PACKET_ARRIVAL_MEAN_INTERVAL_MS);
		k_sleep(K_MSEC(interval));
		
		/* Some packets bypass IRQ handler (direct packet arrival) */
		uint64_t timestamp = timing_counter_get();
		if (k_msgq_put(&packet_queue, &timestamp, K_NO_WAIT) == 0) {
			packet_stats.events_generated++;
		}
	}
}

/* Print statistics */
static void print_statistics(void)
{
	printk("\n=== Workload 2: Event-Driven Communication System Results ===\n\n");
	
	printk("IRQ Handler (Critical Priority, Deadline: %d us):\n", IRQ_HANDLER_DEADLINE_US);
	printk("  Events Generated: %u\n", irq_stats.events_generated);
	printk("  Events Processed: %u\n", irq_stats.events_processed);
	printk("  Deadline Misses: %u\n", irq_stats.deadline_misses);
	printk("  Avg Latency: %llu us\n",
	       irq_stats.events_processed > 0 ? irq_stats.total_latency_us / irq_stats.events_processed : 0);
	printk("  Min/Max Latency: %llu / %llu us\n", 
	       irq_stats.min_latency_us == UINT64_MAX ? 0 : irq_stats.min_latency_us,
	       irq_stats.max_latency_us);
	printk("  Avg Response Time: %llu us\n",
	       irq_stats.events_processed > 0 ? irq_stats.total_response_time_us / irq_stats.events_processed : 0);
	printk("  Tardiness Rate: %.2f%%\n\n",
	       irq_stats.events_processed > 0 ? (100.0 * irq_stats.deadline_misses / irq_stats.events_processed) : 0);
	
	printk("Packet Processor (High Priority, Deadline: %d us):\n", PACKET_PROC_DEADLINE_US);
	printk("  Events Generated: %u\n", packet_stats.events_generated);
	printk("  Events Processed: %u\n", packet_stats.events_processed);
	printk("  Deadline Misses: %u\n", packet_stats.deadline_misses);
	printk("  Avg Latency: %llu us\n",
	       packet_stats.events_processed > 0 ? packet_stats.total_latency_us / packet_stats.events_processed : 0);
	printk("  Min/Max Latency: %llu / %llu us\n",
	       packet_stats.min_latency_us == UINT64_MAX ? 0 : packet_stats.min_latency_us,
	       packet_stats.max_latency_us);
	printk("  Avg Response Time: %llu us\n",
	       packet_stats.events_processed > 0 ? packet_stats.total_response_time_us / packet_stats.events_processed : 0);
	printk("  Tardiness Rate: %.2f%%\n\n",
	       packet_stats.events_processed > 0 ? (100.0 * packet_stats.deadline_misses / packet_stats.events_processed) : 0);
	
	printk("Protocol Handler (Medium Priority, Variable Execution):\n");
	printk("  Events Processed: %u\n", protocol_stats.events_processed);
	printk("  Avg Latency: %llu us\n",
	       protocol_stats.events_processed > 0 ? protocol_stats.total_latency_us / protocol_stats.events_processed : 0);
	printk("  Min/Max Latency: %llu / %llu us\n",
	       protocol_stats.min_latency_us == UINT64_MAX ? 0 : protocol_stats.min_latency_us,
	       protocol_stats.max_latency_us);
	printk("  Avg Response Time: %llu us\n\n",
	       protocol_stats.events_processed > 0 ? protocol_stats.total_response_time_us / protocol_stats.events_processed : 0);
	
	printk("Bulk Transmission (Low Priority):\n");
	printk("  Events Processed: %u\n", bulk_stats.events_processed);
	printk("  Avg Response Time: %llu us\n\n",
	       bulk_stats.events_processed > 0 ? bulk_stats.total_response_time_us / bulk_stats.events_processed : 0);
	
	uint32_t total_events = irq_stats.events_processed + packet_stats.events_processed +
	                        protocol_stats.events_processed + bulk_stats.events_processed;
	printk("Total Throughput: %u events processed in %d seconds\n",
	       total_events, TEST_DURATION_MS / 1000);
	printk("Events per second: %u\n", total_events / (TEST_DURATION_MS / 1000));
	printk("Shared Resource Accesses: %u\n", shared_resource_counter);
}

int main(void)
{
	printk("\n=== Workload 2: Event-Driven Communication System ===\n");
	printk("Testing scheduler with sporadic events and priority inversion\n");
	printk("Duration: %d seconds\n\n", TEST_DURATION_MS / 1000);
	
	/* Initialize timing */
	timing_init();
	timing_start();
	
	timing_t start = timing_counter_get();
	k_busy_wait(1000000);
	timing_t end = timing_counter_get();
	uint64_t total_cycles = timing_cycles_get(&start, &end);
	cycles_per_us = total_cycles / 1000000;
	
	printk("Timing calibration: %llu cycles per second\n", total_cycles);
	printk("Cycles per microsecond: %llu\n\n", cycles_per_us);
	
	/* Initialize mutex */
	k_mutex_init(&resource_mutex);
	
	/* Create worker threads */
	k_thread_create(&irq_handler_thread, irq_handler_stack, IRQ_HANDLER_STACK_SIZE,
	                irq_handler_entry, NULL, NULL, NULL,
	                IRQ_HANDLER_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&irq_handler_thread, "irq_handler");
	
	k_thread_create(&packet_proc_thread, packet_proc_stack, PACKET_PROC_STACK_SIZE,
	                packet_proc_entry, NULL, NULL, NULL,
	                PACKET_PROC_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&packet_proc_thread, "packet_proc");
	
	k_thread_create(&protocol_thread, protocol_stack, PROTOCOL_STACK_SIZE,
	                protocol_handler_entry, NULL, NULL, NULL,
	                PROTOCOL_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&protocol_thread, "protocol");
	
	k_thread_create(&bulk_tx_thread, bulk_tx_stack, BULK_TX_STACK_SIZE,
	                bulk_tx_entry, NULL, NULL, NULL,
	                BULK_TX_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&bulk_tx_thread, "bulk_tx");
	
	/* Create event generator threads */
	k_thread_create(&irq_gen_thread, irq_gen_stack, 1024,
	                irq_event_generator, NULL, NULL, NULL,
	                8, 0, K_NO_WAIT);
	k_thread_name_set(&irq_gen_thread, "irq_gen");
	
	k_thread_create(&packet_gen_thread, packet_gen_stack, 1024,
	                packet_event_generator, NULL, NULL, NULL,
	                8, 0, K_NO_WAIT);
	k_thread_name_set(&packet_gen_thread, "packet_gen");
	
	/* Run test */
	k_sleep(K_MSEC(TEST_DURATION_MS));
	
	/* Print results */
	print_statistics();
	
	printk("\nTest completed.\n");
	
	return 0;
}
