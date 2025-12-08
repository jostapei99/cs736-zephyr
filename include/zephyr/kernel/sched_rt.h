/*
 * Copyright (c) 2024 CS736 Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Real-Time Scheduling API Extensions
 *
 * This header provides extended real-time scheduling APIs for custom
 * scheduling algorithms including Weighted EDF, WSRT, and RMS.
 */

#ifndef ZEPHYR_INCLUDE_KERNEL_SCHED_RT_H_
#define ZEPHYR_INCLUDE_KERNEL_SCHED_RT_H_

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup sched_rt_apis Real-Time Scheduling APIs
 * @ingroup kernel_apis
 * @{
 */

#ifdef CONFIG_736

/**
 * @brief Set thread weight for weighted scheduling algorithms
 *
 * Sets the weight parameter for a thread. The weight is used by:
 * - Weighted EDF: deadline/weight ratio (higher weight = higher priority)
 * - WSRT: time_left/weight ratio (higher weight = higher priority)
 *
 * @param tid Thread ID
 * @param weight Weight value (should be > 0, typical range: 1-10)
 *
 * @note If the thread is in a run queue, it will be requeued with
 *       the new weight.
 * @note A weight of 0 is treated as 1 to avoid division by zero.
 */
__syscall void k_thread_weight_set(k_tid_t tid, int weight);

/**
 * @brief Set expected execution time for a thread
 *
 * Sets the expected execution time for a thread. This is used by:
 * - RMS: Threads with shorter exec times have higher priority
 * - WSRT: Initial value for time_left tracking
 *
 * @param tid Thread ID
 * @param exec_time Expected execution time in cycles
 *
 * @note If the thread is in a run queue, it will be requeued with
 *       the new exec_time.
 * @note For WSRT, prio_time_left is reset to exec_time when
 *       k_thread_deadline_set() is called.
 */
__syscall void k_thread_exec_time_set(k_tid_t tid, int exec_time);

/**
 * @brief Helper to set exec time in milliseconds
 *
 * @param tid Thread ID
 * @param exec_time_ms Execution time in milliseconds
 */
static inline void k_thread_exec_time_set_ms(k_tid_t tid, uint32_t exec_time_ms)
{
	k_thread_exec_time_set(tid, k_ms_to_cyc_ceil32(exec_time_ms));
}

#endif /* CONFIG_736 */

/**
 * @brief Configure a periodic real-time task
 *
 * Helper function to configure all RT parameters for a periodic task.
 * This sets up deadline, weight, and execution time in one call.
 *
 * @param tid Thread ID
 * @param period_ms Period in milliseconds
 * @param exec_time_ms Expected execution time in milliseconds
 * @param weight Task weight (importance factor)
 *
 * @note This is a convenience wrapper around individual syscalls
 */
static inline void k_thread_rt_config(k_tid_t tid, uint32_t period_ms,
				      uint32_t exec_time_ms, int weight)
{
#ifdef CONFIG_SCHED_DEADLINE
	/* Set deadline equal to period for implicit-deadline tasks */
	k_thread_deadline_set(tid, k_ms_to_cyc_ceil32(period_ms));
#endif

#ifdef CONFIG_736
	k_thread_exec_time_set(tid, k_ms_to_cyc_ceil32(exec_time_ms));
	k_thread_weight_set(tid, weight);
#else
	ARG_UNUSED(exec_time_ms);
	ARG_UNUSED(weight);
#endif
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_KERNEL_SCHED_RT_H_ */
