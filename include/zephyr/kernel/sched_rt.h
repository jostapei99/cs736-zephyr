#ifndef ZEPHYR_INCLUDE_KERNEL_SCHED_RT_H_
#define ZEPHYR_INCLUDE_KERNEL_SCHED_RT_H_

#include <zephyr/kernel.h>
#include <zephyr/syscall.h>

#ifdef CONFIG_736_ADD_ONS

__syscall int k_thread_weight_set(k_tid_t tid, uint32_t weight);

__syscall int k_thread_weight_get(k_tid_t tid, uint32_t *weight);

__syscall int k_thread_exec_time_set(k_tid_t tid, uint32_t exec_time);

__syscall int k_thread_exec_time_get(k_tid_t tid, uint32_t *exec_time);

__syscall int k_thread_time_left_set(k_tid_t tid, uint32_t time_left);

__syscall int k_thread_time_left_get(k_tid_t tid, uint32_t *time_left);

#endif /* CONFIG_736_ADD_ONS */

/**
 * @brief Real-time thread statistics structure
 *
 * This structure contains per-thread RT statistics for custom schedulers.
 * Statistics are collected automatically by the scheduler and can be
 * queried via k_thread_rt_stats_get().
 */
struct k_thread_rt_stats {
	/* Event counters */
	uint32_t activations;          /**< Number of times thread was activated */
	uint32_t completions;          /**< Number of completed executions */
	uint32_t preemptions;          /**< Number of times preempted */
	uint32_t context_switches;     /**< Total context switches (in+out) */
	uint32_t deadline_misses;      /**< Number of deadline misses */
	uint32_t priority_inversions;  /**< Number of priority inversions */

	/* Timing statistics (in milliseconds) */
	uint64_t total_response_time;  /**< Cumulative response time */
	uint64_t total_waiting_time;   /**< Cumulative waiting time in ready queue */
	uint64_t total_exec_time;      /**< Cumulative execution time */

	uint32_t min_response_time;    /**< Minimum response time */
	uint32_t max_response_time;    /**< Maximum response time */
	uint32_t min_waiting_time;     /**< Minimum waiting time */
	uint32_t max_waiting_time;     /**< Maximum waiting time */

#ifdef CONFIG_736_RT_STATS_SQUARED
	uint64_t sum_response_time_sq; /**< Sum of squared response times */
	uint64_t sum_waiting_time_sq;  /**< Sum of squared waiting times */
#endif

#ifdef CONFIG_736_RT_STATS_DETAILED
	uint64_t last_activation_time;   /**< Timestamp of last activation */
	uint64_t last_ready_time;        /**< Timestamp when entered ready queue */
	uint64_t last_start_time;        /**< Timestamp when started executing */
	uint64_t last_completion_time;   /**< Timestamp of last completion */
#endif
};

#ifdef CONFIG_736_RT_STATS

/**
 * @brief Get RT statistics for a thread
 *
 * @param tid Thread ID (NULL = current thread)
 * @param stats Pointer to structure to receive statistics
 * @return 0 on success, -EINVAL if stats is NULL
 */
__syscall int k_thread_rt_stats_get(k_tid_t tid, struct k_thread_rt_stats *stats);

/**
 * @brief Reset RT statistics for a thread
 *
 * @param tid Thread ID (NULL = current thread)
 * @return 0 on success
 */
__syscall int k_thread_rt_stats_reset(k_tid_t tid);

/**
 * @brief Mark thread activation for RT statistics
 *
 * Call this when a periodic thread starts a new period/job.
 *
 * @param tid Thread ID (NULL = current thread)
 */
__syscall void k_thread_rt_stats_activation(k_tid_t tid);

/**
 * @brief Record a deadline miss for RT statistics
 *
 * Call this when a thread misses its deadline.
 *
 * @param tid Thread ID (NULL = current thread)
 */
__syscall void k_thread_rt_stats_deadline_miss(k_tid_t tid);

#endif /* CONFIG_736_RT_STATS */

#endif /* ZEPHYR_INCLUDE_KERNEL_SCHED_RT_H_ */