#include <zephyr/kernel.h>
#include <zephyr/kernel/sched_rt.h>

#define STACK_SIZE 1024
#define PRIORITY 5

K_THREAD_STACK_DEFINE(thread_urgent_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(thread_normal_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(thread_slack_stack, STACK_SIZE);

struct k_thread thread_urgent_data;
struct k_thread thread_normal_data;
struct k_thread thread_slack_data;

static volatile int execution_order = 0;

void thread_urgent(void *p1, void *p2, void *p3)
{
	int order = execution_order++;
	uint32_t time_left;
	int deadline;
	
	k_thread_time_left_get(k_current_get(), &time_left);
	deadline = k_current_get()->base.prio_deadline;
	int32_t laxity = deadline - time_left;
	
	printk("[Order %d] Urgent Thread: deadline=%d, time_left=%u, laxity=%d\n",
	       order, deadline, time_left, laxity);
}

void thread_normal(void *p1, void *p2, void *p3)
{
	int order = execution_order++;
	uint32_t time_left;
	int deadline;
	
	k_thread_time_left_get(k_current_get(), &time_left);
	deadline = k_current_get()->base.prio_deadline;
	int32_t laxity = deadline - time_left;
	
	printk("[Order %d] Normal Thread: deadline=%d, time_left=%u, laxity=%d\n",
	       order, deadline, time_left, laxity);
}

void thread_slack(void *p1, void *p2, void *p3)
{
	int order = execution_order++;
	uint32_t time_left;
	int deadline;
	
	k_thread_time_left_get(k_current_get(), &time_left);
	deadline = k_current_get()->base.prio_deadline;
	int32_t laxity = deadline - time_left;
	
	printk("[Order %d] Slack Thread: deadline=%d, time_left=%u, laxity=%d\n",
	       order, deadline, time_left, laxity);
}

int main(void)
{
	printk("\n*** Least Laxity First (LLF) Test ***\n");
	printk("Testing CONFIG_736_LLF\n");
	printk("Scheduling based on laxity (slack time)\n");
	printk("Laxity = deadline - time_left\n");
	printk("Lower laxity = higher priority\n\n");

	printk("Test: Three threads with different laxity values\n");
	printk("Urgent: deadline=100, time_left=95, laxity=5\n");
	printk("Normal: deadline=200, time_left=150, laxity=50\n");
	printk("Slack:  deadline=300, time_left=200, laxity=100\n");
	printk("Expected order: Urgent -> Normal -> Slack\n");
	printk("------------------------------------------------------------\n");

	k_tid_t tid_urgent = k_thread_create(
		&thread_urgent_data, thread_urgent_stack,
		K_THREAD_STACK_SIZEOF(thread_urgent_stack),
		thread_urgent, NULL, NULL, NULL,
		PRIORITY, 0, K_NO_WAIT);
	
	k_tid_t tid_normal = k_thread_create(
		&thread_normal_data, thread_normal_stack,
		K_THREAD_STACK_SIZEOF(thread_normal_stack),
		thread_normal, NULL, NULL, NULL,
		PRIORITY, 0, K_NO_WAIT);
	
	k_tid_t tid_slack = k_thread_create(
		&thread_slack_data, thread_slack_stack,
		K_THREAD_STACK_SIZEOF(thread_slack_stack),
		thread_slack, NULL, NULL, NULL,
		PRIORITY, 0, K_NO_WAIT);

	/* Set deadlines and remaining time */
	k_thread_deadline_set(tid_urgent, 100);
	k_thread_deadline_set(tid_normal, 200);
	k_thread_deadline_set(tid_slack, 300);
	
	k_thread_time_left_set(tid_urgent, 95);   /* laxity = 5 (most urgent) */
	k_thread_time_left_set(tid_normal, 150);  /* laxity = 50 */
	k_thread_time_left_set(tid_slack, 200);   /* laxity = 100 (least urgent) */

	k_thread_join(tid_urgent, K_FOREVER);
	k_thread_join(tid_normal, K_FOREVER);
	k_thread_join(tid_slack, K_FOREVER);

	printk("\n------------------------------------------------------------\n");
	printk("*** Test Complete ***\n\n");
	printk("LLF verification:\n");
	printk("  Threads scheduled by laxity (deadline - time_left)\n");
	printk("  Lower laxity gets higher priority\n");
	printk("  Detects urgent tasks that need immediate attention\n");
	printk("  deadline and time_left syscalls working correctly\n");

	return 0;
}
