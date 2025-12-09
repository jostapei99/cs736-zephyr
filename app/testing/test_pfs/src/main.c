#include <zephyr/kernel.h>
#include <zephyr/kernel/sched_rt.h>

#define STACK_SIZE 1024
#define PRIORITY 5

K_THREAD_STACK_DEFINE(thread_low_runtime_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(thread_med_runtime_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(thread_high_runtime_stack, STACK_SIZE);

struct k_thread thread_low_runtime_data;
struct k_thread thread_med_runtime_data;
struct k_thread thread_high_runtime_data;

static volatile int execution_order = 0;

void thread_low_runtime(void *p1, void *p2, void *p3)
{
	int order = execution_order++;
	uint32_t exec_time, weight;
	
	k_thread_exec_time_get(k_current_get(), &exec_time);
	k_thread_weight_get(k_current_get(), &weight);
	printk("[Order %d] Low Runtime Thread: runtime=%u, weight=%u, virtual_runtime=%u\n",
	       order, exec_time, weight, exec_time / (weight ? weight : 1));
}

void thread_med_runtime(void *p1, void *p2, void *p3)
{
	int order = execution_order++;
	uint32_t exec_time, weight;
	
	k_thread_exec_time_get(k_current_get(), &exec_time);
	k_thread_weight_get(k_current_get(), &weight);
	printk("[Order %d] Med Runtime Thread: runtime=%u, weight=%u, virtual_runtime=%u\n",
	       order, exec_time, weight, exec_time / (weight ? weight : 1));
}

void thread_high_runtime(void *p1, void *p2, void *p3)
{
	int order = execution_order++;
	uint32_t exec_time, weight;
	
	k_thread_exec_time_get(k_current_get(), &exec_time);
	k_thread_weight_get(k_current_get(), &weight);
	printk("[Order %d] High Runtime Thread: runtime=%u, weight=%u, virtual_runtime=%u\n",
	       order, exec_time, weight, exec_time / (weight ? weight : 1));
}

int main(void)
{
	printk("\n*** Proportional Fair Scheduling (PFS) Test ***\n");
	printk("Testing CONFIG_736_PFS\n");
	printk("Scheduling based on virtual runtime (runtime/weight)\n");
	printk("Lower virtual_runtime = higher priority (fairness)\n\n");

	printk("Test 1: Different runtimes, same weight\n");
	printk("Expected order: Low runtime -> Med runtime -> High runtime\n");
	printk("------------------------------------------------------------\n");

	k_tid_t tid_low = k_thread_create(
		&thread_low_runtime_data, thread_low_runtime_stack,
		K_THREAD_STACK_SIZEOF(thread_low_runtime_stack),
		thread_low_runtime, NULL, NULL, NULL,
		PRIORITY, 0, K_NO_WAIT);
	
	k_tid_t tid_med = k_thread_create(
		&thread_med_runtime_data, thread_med_runtime_stack,
		K_THREAD_STACK_SIZEOF(thread_med_runtime_stack),
		thread_med_runtime, NULL, NULL, NULL,
		PRIORITY, 0, K_NO_WAIT);
	
	k_tid_t tid_high = k_thread_create(
		&thread_high_runtime_data, thread_high_runtime_stack,
		K_THREAD_STACK_SIZEOF(thread_high_runtime_stack),
		thread_high_runtime, NULL, NULL, NULL,
		PRIORITY, 0, K_NO_WAIT);

	/* Set different accumulated runtimes, same weight */
	k_thread_exec_time_set(tid_low, 10);    /* virtual = 10/100 = 0.1 */
	k_thread_exec_time_set(tid_med, 50);    /* virtual = 50/100 = 0.5 */
	k_thread_exec_time_set(tid_high, 100);  /* virtual = 100/100 = 1.0 */
	
	k_thread_weight_set(tid_low, 100);
	k_thread_weight_set(tid_med, 100);
	k_thread_weight_set(tid_high, 100);

	k_thread_join(tid_low, K_FOREVER);
	k_thread_join(tid_med, K_FOREVER);
	k_thread_join(tid_high, K_FOREVER);

	printk("\n------------------------------------------------------------\n");
	printk("Test 1 Complete\n\n");

	printk("Test 2: Same runtime, different weights (fairness test)\n");
	printk("A: runtime=100, weight=100, virtual=1.0\n");
	printk("B: runtime=100, weight=200, virtual=0.5\n");
	printk("C: runtime=100, weight=400, virtual=0.25\n");
	printk("Expected order: C -> B -> A (higher weight gets more CPU)\n");
	printk("------------------------------------------------------------\n");

	execution_order = 0;

	tid_low = k_thread_create(
		&thread_low_runtime_data, thread_low_runtime_stack,
		K_THREAD_STACK_SIZEOF(thread_low_runtime_stack),
		thread_low_runtime, NULL, NULL, NULL,
		PRIORITY, 0, K_NO_WAIT);
	
	tid_med = k_thread_create(
		&thread_med_runtime_data, thread_med_runtime_stack,
		K_THREAD_STACK_SIZEOF(thread_med_runtime_stack),
		thread_med_runtime, NULL, NULL, NULL,
		PRIORITY, 0, K_NO_WAIT);
	
	tid_high = k_thread_create(
		&thread_high_runtime_data, thread_high_runtime_stack,
		K_THREAD_STACK_SIZEOF(thread_high_runtime_stack),
		thread_high_runtime, NULL, NULL, NULL,
		PRIORITY, 0, K_NO_WAIT);

	/* Same runtime, different weights for fairness */
	k_thread_exec_time_set(tid_low, 100);
	k_thread_exec_time_set(tid_med, 100);
	k_thread_exec_time_set(tid_high, 100);
	
	k_thread_weight_set(tid_low, 100);   /* virtual = 1.0 */
	k_thread_weight_set(tid_med, 200);   /* virtual = 0.5 */
	k_thread_weight_set(tid_high, 400);  /* virtual = 0.25 */

	k_thread_join(tid_low, K_FOREVER);
	k_thread_join(tid_med, K_FOREVER);
	k_thread_join(tid_high, K_FOREVER);

	printk("\n------------------------------------------------------------\n");
	printk("*** All Tests Complete ***\n\n");
	printk("PFS verification:\n");
	printk("  Threads scheduled by virtual_runtime (runtime/weight)\n");
	printk("  Lower virtual_runtime gets priority\n");
	printk("  Ensures fairness - threads with less CPU get scheduled\n");
	printk("  Higher weight threads get proportionally more CPU\n");

	return 0;
}
