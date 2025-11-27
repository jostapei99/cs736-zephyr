/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "zephyr/kernel/thread.h"
#include "zephyr/sys/time_units.h"
#include <stdbool.h>
#include <stdio.h>

// struct k_thread thread1;
// struct k_thread thread2;

void task(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

	k_tid_t myTid = k_current_get();

	k_thread_weight_set(myTid, 1);
	myTid->base.usage.track_usage = true;
	k_thread_exec_time_set(myTid, k_ms_to_cyc_ceil32(500));

	while (1) {
		k_thread_deadline_set(myTid, k_ms_to_cyc_ceil32(1000));
		k_yield();
		k_busy_wait(250000);
		printk("Halfway thru task1\n");
		k_busy_wait(250000);
		k_sleep(K_TIMEOUT_ABS_SEC(1));
	}
}

void task2(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

	k_tid_t myTid = k_current_get();

	k_thread_weight_set(myTid, 1);
	myTid->base.usage.track_usage = true;
	k_thread_exec_time_set(myTid, k_ms_to_cyc_ceil32(400));

	while (1) {
		k_thread_deadline_set(myTid, k_ms_to_cyc_ceil32(1));
		k_yield();
		k_busy_wait(200000);
		printk("Halfway thru task2\n");
		k_busy_wait(200000);
		k_sleep(K_TIMEOUT_ABS_SEC(1));
	}
}

// Mission control task - highest priority
K_THREAD_DEFINE(thread1, 2048,
				task, NULL, NULL, NULL,
				5, 0, 0);

// Navigation task - high priority
K_THREAD_DEFINE(thread2, 2048,
				task2, NULL, NULL, NULL,
				5, 0, 0);



int main(void)
{
	
	return 0;
}
