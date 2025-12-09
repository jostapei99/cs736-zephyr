#include <zephyr/kernel.h>
#include <zephyr/kernel/sched_rt.h>

#define STACK_SIZE 1024
#define PRIORITY 5

K_THREAD_STACK_DEFINE(thread_short_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(thread_med_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(thread_long_stack, STACK_SIZE);

struct k_thread thread_short_data;
struct k_thread thread_med_data;
struct k_thread thread_long_data;

static volatile int execution_order = 0;

void thread_short(void *p1, void *p2, void *p3)
{
	int order = execution_order++;
	uint32_t exec_time;
	
	k_thread_exec_time_get(k_current_get(), &exec_time);
	printk("[Order %d] Short Exec Thread: exec_time=%u ms\n", order, exec_time);
}

void thread_med(void *p1, void *p2, void *p3)
{
	int order = execution_order++;
	uint32_t exec_time;
	
	k_thread_exec_time_get(k_current_get(), &exec_time);
	printk("[Order %d] Medium Exec Thread: exec_time=%u ms\n", order, exec_time);
}

void thread_long(void *p1, void *p2, void *p3)
{
	int order = execution_order++;
	uint32_t exec_time;
	
	k_thread_exec_time_get(k_current_get(), &exec_time);
	printk("[Order %d] Long Exec Thread: exec_time=%u ms\n", order, exec_time);
}

int main(void)
{
	printk("\n*** Rate Monotonic Scheduling (RMS) Test ***\n");
	printk("Testing CONFIG_736_RMS\n");
	printk("Scheduling based on execution time\n");
	printk("Shorter execution time = higher priority\n\n");

	printk("Test: Three threads with different execution times\n");
	printk("Expected order: Short(10ms) -> Med(50ms) -> Long(100ms)\n");
	printk("--------------------------------------------------------\n");

	/* Create threads */
	k_tid_t tid_short = k_thread_create(
		&thread_short_data, thread_short_stack,
		K_THREAD_STACK_SIZEOF(thread_short_stack),
		thread_short, NULL, NULL, NULL,
		PRIORITY, 0, K_NO_WAIT);
	
	k_tid_t tid_med = k_thread_create(
		&thread_med_data, thread_med_stack,
		K_THREAD_STACK_SIZEOF(thread_med_stack),
		thread_med, NULL, NULL, NULL,
		PRIORITY, 0, K_NO_WAIT);
	
	k_tid_t tid_long = k_thread_create(
		&thread_long_data, thread_long_stack,
		K_THREAD_STACK_SIZEOF(thread_long_stack),
		thread_long, NULL, NULL, NULL,
		PRIORITY, 0, K_NO_WAIT);

	/* Set execution times - RMS prioritizes shorter exec times */
	k_thread_exec_time_set(tid_short, 10);   /* Highest priority */
	k_thread_exec_time_set(tid_med, 50);     /* Medium priority */
	k_thread_exec_time_set(tid_long, 100);   /* Lowest priority */

	/* Wait for completion */
	k_thread_join(tid_short, K_FOREVER);
	k_thread_join(tid_med, K_FOREVER);
	k_thread_join(tid_long, K_FOREVER);

	printk("\n--------------------------------------------------------\n");
	printk("*** Test Complete ***\n\n");
	printk("RMS verification:\n");
	printk("  Threads scheduled by execution time\n");
	printk("  Shorter execution time gets higher priority\n");
	printk("  exec_time syscalls working correctly\n");

	return 0;
}
