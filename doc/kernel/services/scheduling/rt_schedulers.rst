.. _rt_schedulers:

Real-Time Scheduling Algorithms
################################

Overview
********

Zephyr supports multiple real-time scheduling algorithms for threads at the same
priority level. These algorithms extend the base priority scheduler with
deadline-aware and weight-aware policies.

Available Algorithms
********************

Standard EDF (Earliest Deadline First)
=======================================

The baseline real-time scheduler. Threads with earlier absolute deadlines are
scheduled first within the same priority level.

**Configuration:**

.. code-block:: kconfig

   CONFIG_SCHED_DEADLINE=y

**Usage:**

.. code-block:: c

   k_thread_deadline_set(tid, k_ms_to_cyc_ceil32(deadline_ms));

Weighted EDF
============

Extends EDF with weight-based priority adjustment. Threads are scheduled based on
their ``deadline / weight`` ratio. Lower ratios mean higher priority.

**Effect:** Higher weight threads are treated as having effectively earlier deadlines.

**Configuration:**

.. code-block:: kconfig

   CONFIG_736_MOD_EDF=y

**Usage:**

.. code-block:: c

   k_thread_deadline_set(tid, k_ms_to_cyc_ceil32(100));  // 100ms deadline
   k_thread_weight_set(tid, 2);                           // Weight of 2

**Example:** Thread A (deadline=100, weight=2) vs Thread B (deadline=120, weight=1):

- Thread A ratio: 100/2 = 50
- Thread B ratio: 120/1 = 120
- Thread A has higher priority (lower ratio)

Weighted Shortest Remaining Time (WSRT)
========================================

Schedules based on ``time_left / weight`` ratio. Threads with less weighted
remaining time are scheduled first.

**Configuration:**

.. code-block:: kconfig

   CONFIG_736_WSRT=y
   CONFIG_SCHED_THREAD_USAGE=y
   CONFIG_THREAD_RUNTIME_STATS=y

**Usage:**

.. code-block:: c

   k_thread_deadline_set(tid, k_ms_to_cyc_ceil32(100));
   k_thread_exec_time_set(tid, k_ms_to_cyc_ceil32(50));
   k_thread_weight_set(tid, 2);

**Note:** Requires runtime statistics to track remaining execution time.

Rate Monotonic Scheduling (RMS)
================================

Simplified RMS where threads with shorter execution times have higher priority.
Suitable for periodic tasks.

**Configuration:**

.. code-block:: kconfig

   CONFIG_736_RMS=y

**Usage:**

.. code-block:: c

   k_thread_exec_time_set(tid, k_ms_to_cyc_ceil32(expected_exec_ms));

Least Laxity First (LLF)
=========================

A dynamic priority algorithm that schedules based on laxity (slack time).

**Formula:** Laxity = Deadline - Remaining_Time - Current_Time

Threads with less laxity (tighter deadlines relative to remaining work) get
higher priority.

**Characteristics:**

- Dynamic priority adjustment based on execution progress
- Can detect imminent deadline misses earlier than EDF
- May cause more context switches due to priority changes
- Known to exhibit thrashing behavior under overload

**Configuration:**

.. code-block:: kconfig

   CONFIG_736_LLF=y
   CONFIG_SCHED_THREAD_USAGE=y
   CONFIG_THREAD_RUNTIME_STATS=y

**Usage:**

.. code-block:: c

   k_thread_deadline_set(tid, k_ms_to_cyc_ceil32(100));
   k_thread_exec_time_set(tid, k_ms_to_cyc_ceil32(50));

**When to use:**

- Need early detection of potential deadline misses
- Studying dynamic vs. static priority scheduling
- Analyzing overload behavior and thrashing

Proportional Fair Scheduling (PFS)
===================================

Provides fairness-oriented scheduling based on weighted virtual runtime.
Each thread accumulates virtual runtime, and the scheduler picks the thread
with the lowest virtual_runtime/weight ratio.

**Characteristics:**

- Ensures fair CPU allocation proportional to weights
- Prevents thread starvation
- Good for mixed workloads (RT + best-effort tasks)
- Similar to Linux CFS (Completely Fair Scheduler)

**Configuration:**

.. code-block:: kconfig

   CONFIG_736_PFS=y

**Usage:**

.. code-block:: c

   k_thread_weight_set(tid, 2);  // Higher weight = more CPU share

**When to use:**

- Fairness is more important than hard deadline guarantees
- Mixed real-time and non-real-time workloads
- Preventing starvation in long-running systems

Implementation Architecture
***************************

The scheduling algorithms are implemented in a modular fashion:

Algorithm Comparison Functions
===============================

Each algorithm provides a comparison function in ``kernel/include/priority_q.h``:

- ``z_sched_cmp_edf()`` - Standard EDF
- ``z_sched_cmp_weighted_edf()`` - Weighted EDF
- ``z_sched_cmp_wsrt()`` - WSRT
- ``z_sched_cmp_rms()`` - RMS
- ``z_sched_cmp_llf()`` - Least Laxity First
- ``z_sched_cmp_pfs()`` - Proportional Fair Scheduling

These functions are called by ``z_sched_prio_cmp()`` based on the selected
configuration.

Thread Attributes
=================

Custom schedulers use additional thread base structure fields:

.. code-block:: c

   struct _thread_base {
       // ... existing fields ...
       #ifdef CONFIG_736
       int prio_exec_time;   // Expected execution time
       int prio_weight;      // Thread weight/importance
       #ifdef CONFIG_736_TIME_LEFT
       int prio_time_left;   // Remaining execution time
       #endif
       #endif
   };

Configuration Syscalls
======================

.. code-block:: c

   void k_thread_weight_set(k_tid_t tid, int weight);
   void k_thread_exec_time_set(k_tid_t tid, int exec_time);

Both syscalls properly handle thread requeuing when the thread is in the run queue.

API Reference
*************

See :zephyr_file:`include/zephyr/kernel/sched_rt.h` for the complete API.

Helper Function
===============

.. code-block:: c

   static inline void k_thread_rt_config(k_tid_t tid, uint32_t period_ms,
                                         uint32_t exec_time_ms, int weight)

Configures all RT parameters in one call for periodic tasks.

Example Usage
*************

Periodic Task with Weighted EDF
================================

.. code-block:: c

   #include <zephyr/kernel.h>
   #include <zephyr/kernel/sched_rt.h>

   void periodic_task(void *arg1, void *arg2, void *arg3)
   {
       uint32_t period_ms = 100;
       uint32_t exec_time_ms = 30;
       int weight = 2;  // Higher importance

       // Configure RT parameters
       k_thread_rt_config(k_current_get(), period_ms, exec_time_ms, weight);

       while (1) {
           // Wait for next period
           k_sleep(K_MSEC(period_ms));

           // Do work
           do_work();
       }
   }

Comparing Algorithms
====================

.. code-block:: text

   // prj.conf - Choose one:
   
   # Standard EDF
   CONFIG_SCHED_DEADLINE=y
   
   # Weighted EDF
   CONFIG_736_MOD_EDF=y
   
   # WSRT (requires runtime stats)
   CONFIG_736_WSRT=y
   CONFIG_SCHED_THREAD_USAGE=y
   CONFIG_THREAD_RUNTIME_STATS=y
   
   # RMS
   CONFIG_736_RMS=y
   
   # LLF (requires runtime stats)
   CONFIG_736_LLF=y
   CONFIG_SCHED_THREAD_USAGE=y
   CONFIG_THREAD_RUNTIME_STATS=y
   
   # PFS
   CONFIG_736_PFS=y

Algorithm Selection Guide
==========================

+------------------+----------------+-------------------+------------------+
| Algorithm        | Best For       | Key Strength      | Key Weakness     |
+==================+================+===================+==================+
| EDF              | Hard RT        | Optimal for       | No               |
|                  |                | single CPU        | differentiation  |
+------------------+----------------+-------------------+------------------+
| Weighted EDF     | Mixed          | Task importance   | Loses            |
|                  | criticality    | + deadlines       | optimality       |
+------------------+----------------+-------------------+------------------+
| WSRT             | Response time  | Adapts to         | Can starve       |
|                  | optimization   | progress          | long tasks       |
+------------------+----------------+-------------------+------------------+
| RMS              | Predictability | Static, well-     | Lower            |
|                  |                | studied           | utilization      |
+------------------+----------------+-------------------+------------------+
| LLF              | Early miss     | Dynamic laxity    | Thrashing        |
|                  | detection      | tracking          | under load       |
+------------------+----------------+-------------------+------------------+
| PFS              | Fairness       | Prevents          | No deadline      |
|                  |                | starvation        | guarantees       |
+------------------+----------------+-------------------+------------------+

See :zephyr_file:`ALGORITHM_COMPARISON.md` for detailed comparison.

Design Rationale
****************

Modularity
==========

Each algorithm is implemented as a separate comparison function, making it easy to:

- Add new algorithms by creating new comparison functions
- Test algorithms independently
- Understand algorithm logic without navigating conditional compilation

Zero Runtime Overhead
=====================

Algorithm selection is done at compile-time via ``#ifdef`` preprocessor directives.
This ensures zero runtime overhead - only the selected algorithm's code is included.

Consistency with Zephyr
========================

The implementation follows Zephyr's existing patterns:

- Uses ``ALWAYS_INLINE`` for performance-critical functions
- Maintains existing scheduler queue structure
- Compatible with all Zephyr priority queue implementations (simple, scalable, multiq)

Further Reading
***************

- :ref:`scheduling_v2`
- :ref:`threads_v2`
- :zephyr_file:`kernel/include/priority_q.h`
