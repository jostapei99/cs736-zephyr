/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <stdint.h>

int main(void) {
    printk("Hello from Zephyr!\n");
    k_sleep(K_MSEC(100));
	uint32_t start_time = k_uptime_get_32();
	printk("Uptime: %u ms\n", start_time);

	while (1) {

		printk("Hello again from Zephyr! uptime: %u s\n", (k_uptime_get_32() - start_time) / 1000);
		k_sleep(K_SECONDS(1));
	}
	return 0;
}

