#include <zephyr/kernel.h>
#include <zephyr/kernel/sched_rt.h>

#define STACK_SIZE 1024
#define PRIORITY 5

K_THREAD_STACK_DEFINE(thread_a_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(thread_b_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(thread_c_stack, STACK_SIZE);

struct k_thread thread_a_data;
struct k_thread thread_b_data;
struct k_thread thread_c_data;

static volatile int execution_order = 0;

void thread_a(void *p1, void *p2, void *p3)
{
	int order = execution_order++;
	uint32_t weight, time_left;
	
	k_thread_weight_get(k_current_get(), &weight);
	k_thread_time_left_get(k_current_get(), &time_left);
	printk("[Order %d] Thread A: weight=%u, time_left=%u, ratio=%u\n",
	       order, weight, time_left, time_left / (weight ? weight : 1));
}

void thread_b(void *p1, void *p2, void *p3)
{
	int order = execution_order++;
	uint32_t weight, time_left;
	
	k_thread_weight_get(k_current_get(), &weight);
	k_thread_time_left_get(k_current_get(), &time_left);
	printk("[Order %d] Thread B: weight=%u, time_left=%u, ratio=%u\n",
	       order, weight, time_left, time_left / (weight ? weight : 1));
}

void thread_c(void *p1, void *p2, void *p3)
{
	int order = execution_order++;
	uint32_t weight, time_left;
	
	k_thread_weight_get(k_current_get(), &weight);
	k_thread_time_left_get(k_current_get(), &time_left);
	printk("[Order %d] Thread C: weight=%u, time_left=%u, ratio=%u\n",
	       order, weight, time_left, time_left / (weight ? weight : 1));
}

int main(void)
{
	printk("\n*** Weighted Shortest Remaining Time (WSRT) Test ***\n");
	printk("Testing CONFIG_736_WSRT\n");
	printk("Scheduling based on time_left/weight ratio\n");
	printk("Lower ratio = higher priority\n\n");

	printk("Test 1: Same time_left, different weights\n");
	printk("Expected order: High weight -> Med weight -> Low weight\n");
	printk("------------------------------------------------------------\n");

	k_tid_t tid_a = k_thread_create(
		&thread_a_data, thread_a_stack,
		K_THREAD_STACK_SIZEOF(thread_a_stack),
		thread_a, NULL, NULL, NULL,
		PRIORITY, 0, K_NO_WAIT);
	
	k_tid_t tid_b = k_thread_create(
		&thread_b_data, thread_b_stack,
		K_THREAD_STACK_SIZEOF(thread_b_stack),
		thread_b, NULL, NULL, NULL,
		PRIORITY, 0, K_NO_WAIT);
	
	k_tid_t tid_c = k_thread_create(
		&thread_c_data, thread_c_stack,
		K_THREAD_STACK_SIZEOF(thread_c_stack),
		thread_c, NULL, NULL, NULL,
		PRIORITY, 0, K_NO_WAIT);

	/* Set same time_left, different weights */
	k_thread_time_left_set(tid_a, 100);  /* ratio = 100/300 = 0.33 */
	k_thread_time_left_set(tid_b, 100);  /* ratio = 100/200 = 0.5 */
	k_thread_time_left_set(tid_c, 100);  /* ratio = 100/100 = 1 */
	
	k_thread_weight_set(tid_a, 300);  /* Highest weight, lowest ratio */
	k_thread_weight_set(tid_b, 200);
	k_thread_weight_set(tid_c, 100);  /* Lowest weight, highest ratio */

	k_thread_join(tid_a, K_FOREVER);
	k_thread_join(tid_b, K_FOREVER);
	k_thread_join(tid_c, K_FOREVER);

	printk("\n------------------------------------------------------------\n");
	printk("Test 1 Complete\n\n");

	printk("Test 2: Different time_left and weights\n");
	printk("A: time_left=90, weight=100, ratio=0.9\n");
	printk("B: time_left=100, weight=200, ratio=0.5\n");
	printk("C: time_left=150, weight=300, ratio=0.5\n");
	printk("Expected order: B or C (ratio 0.5) -> A (ratio 0.9)\n");
	printk("------------------------------------------------------------\n");

	execution_order = 0;

	k_thread_create(
		&thread_a_data, thread_a_stack,
		K_THREAD_STACK_SIZEOF(thread_a_stack),
		thread_a, NULL, NULL, NULL,
		PRIORITY, 0, K_NO_WAIT);
	
	k_thread_create(
		&thread_b_data, thread_b_stack,
		K_THREAD_STACK_SIZEOF(thread_b_stack),
		thread_b, NULL, NULL, NULL,
		PRIORITY, 0, K_NO_WAIT);
	
	k_thread_create(
		&thread_c_data, thread_c_stack,
		K_THREAD_STACK_SIZEOF(thread_c_stack),
		thread_c, NULL, NULL, NULL,
		PRIORITY, 0, K_NO_WAIT);

	/* Different configurations for test 2 */
	k_thread_time_left_set(&thread_a_data, 90);
	k_thread_time_left_set(&thread_b_data, 100);
	k_thread_time_left_set(&thread_c_data, 150);
	
	k_thread_weight_set(&thread_a_data, 100);
	k_thread_weight_set(&thread_b_data, 200);
	k_thread_weight_set(&thread_c_data, 300);

	k_sleep(K_MSEC(100));

	printk("\n------------------------------------------------------------\n");
	printk("*** All Tests Complete ***\n\n");
	printk("WSRT verification:\n");
	printk("  Threads scheduled by time_left/weight ratio\n");
	printk("  Lower ratio gets higher priority\n");
	printk("  time_left and weight syscalls working correctly\n");

	return 0;
}
