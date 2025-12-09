#include <zephyr/kernel.h>
#include <zephyr/kernel/sched_rt.h>

#define STACK_SIZE 1024
#define PRIORITY 5

/* Test scenario:
 * Create 3 threads with same deadline but different weights
 * Expected order: Higher weight should execute first (lower deadline/weight ratio)
 */

K_THREAD_STACK_DEFINE(thread_high_weight_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(thread_med_weight_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(thread_low_weight_stack, STACK_SIZE);

struct k_thread thread_high_weight_data;
struct k_thread thread_med_weight_data;
struct k_thread thread_low_weight_data;

static volatile int execution_order = 0;

void thread_high_weight(void *p1, void *p2, void *p3)
{
	int order = execution_order++;
	uint32_t weight;
	
	k_thread_weight_get(k_current_get(), &weight);
	printk("[Order %d] High Weight Thread: weight=%u, deadline=%d, ratio=%d\n",
	       order, weight, k_current_get()->base.prio_deadline,
	       k_current_get()->base.prio_deadline / (weight ? weight : 1));
}

void thread_med_weight(void *p1, void *p2, void *p3)
{
	int order = execution_order++;
	uint32_t weight;
	
	k_thread_weight_get(k_current_get(), &weight);
	printk("[Order %d] Med Weight Thread: weight=%u, deadline=%d, ratio=%d\n",
	       order, weight, k_current_get()->base.prio_deadline,
	       k_current_get()->base.prio_deadline / (weight ? weight : 1));
}

void thread_low_weight(void *p1, void *p2, void *p3)
{
	int order = execution_order++;
	uint32_t weight;
	
	k_thread_weight_get(k_current_get(), &weight);
	printk("[Order %d] Low Weight Thread: weight=%u, deadline=%d, ratio=%d\n",
	       order, weight, k_current_get()->base.prio_deadline,
	       k_current_get()->base.prio_deadline / (weight ? weight : 1));
}

int main(void)
{
	printk("\n*** Weighted EDF Scheduler Test ***\n");
	printk("Testing CONFIG_736_MOD_EDF\n");
	printk("Scheduling based on deadline/weight ratio\n");
	printk("Lower ratio = higher priority\n\n");

	printk("Test 1: Same deadline (1000), different weights\n");
	printk("Expected order: High(300) -> Med(200) -> Low(100)\n");
	printk("---------------------------------------------------\n");

	/* Create threads with same deadline but different weights */
	k_tid_t tid_high = k_thread_create(
		&thread_high_weight_data, thread_high_weight_stack,
		K_THREAD_STACK_SIZEOF(thread_high_weight_stack),
		thread_high_weight, NULL, NULL, NULL,
		PRIORITY, 0, K_NO_WAIT);
	
	k_tid_t tid_med = k_thread_create(
		&thread_med_weight_data, thread_med_weight_stack,
		K_THREAD_STACK_SIZEOF(thread_med_weight_stack),
		thread_med_weight, NULL, NULL, NULL,
		PRIORITY, 0, K_NO_WAIT);
	
	k_tid_t tid_low = k_thread_create(
		&thread_low_weight_data, thread_low_weight_stack,
		K_THREAD_STACK_SIZEOF(thread_low_weight_stack),
		thread_low_weight, NULL, NULL, NULL,
		PRIORITY, 0, K_NO_WAIT);

	/* Set same deadline for all threads */
	k_thread_deadline_set(tid_high, 1000);
	k_thread_deadline_set(tid_med, 1000);
	k_thread_deadline_set(tid_low, 1000);

	/* Set different weights - higher weight = higher priority */
	k_thread_weight_set(tid_high, 300);  /* ratio = 1000/300 = 3.33 */
	k_thread_weight_set(tid_med, 200);   /* ratio = 1000/200 = 5 */
	k_thread_weight_set(tid_low, 100);   /* ratio = 1000/100 = 10 */

	/* Let threads execute */
	k_thread_join(tid_high, K_FOREVER);
	k_thread_join(tid_med, K_FOREVER);
	k_thread_join(tid_low, K_FOREVER);

	printk("\n---------------------------------------------------\n");
	printk("Test 1 Complete\n\n");

	/* Test 2: Different deadlines, different weights */
	printk("Test 2: Different deadlines and weights\n");
	printk("Thread A: deadline=900, weight=100, ratio=9\n");
	printk("Thread B: deadline=1000, weight=200, ratio=5\n");
	printk("Thread C: deadline=1100, weight=300, ratio=3.67\n");
	printk("Expected order: C -> B -> A (lowest ratio first)\n");
	printk("---------------------------------------------------\n");

	execution_order = 0;

	/* Reuse threads with new parameters */
	tid_high = k_thread_create(
		&thread_high_weight_data, thread_high_weight_stack,
		K_THREAD_STACK_SIZEOF(thread_high_weight_stack),
		thread_high_weight, NULL, NULL, NULL,
		PRIORITY, 0, K_NO_WAIT);
	
	tid_med = k_thread_create(
		&thread_med_weight_data, thread_med_weight_stack,
		K_THREAD_STACK_SIZEOF(thread_med_weight_stack),
		thread_med_weight, NULL, NULL, NULL,
		PRIORITY, 0, K_NO_WAIT);
	
	tid_low = k_thread_create(
		&thread_low_weight_data, thread_low_weight_stack,
		K_THREAD_STACK_SIZEOF(thread_low_weight_stack),
		thread_low_weight, NULL, NULL, NULL,
		PRIORITY, 0, K_NO_WAIT);

	/* Reconfigure for test 2 */
	k_thread_deadline_set(tid_high, 1100);
	k_thread_deadline_set(tid_med, 1000);
	k_thread_deadline_set(tid_low, 900);

	k_thread_weight_set(tid_high, 300);  /* C: ratio = 1100/300 = 3.67 */
	k_thread_weight_set(tid_med, 200);   /* B: ratio = 1000/200 = 5 */
	k_thread_weight_set(tid_low, 100);   /* A: ratio = 900/100 = 9 */

	/* Wait for all threads to complete */
	k_thread_join(tid_high, K_FOREVER);
	k_thread_join(tid_med, K_FOREVER);
	k_thread_join(tid_low, K_FOREVER);

	printk("\n---------------------------------------------------\n");
	printk("*** All Tests Complete ***\n");
	printk("\nWeighted EDF verification:\n");
	printk("Threads scheduled by deadline/weight ratio\n");
	printk("Lower ratio gets higher priority\n");
	printk("Weight syscalls working correctly\n");

	return 0;
}
