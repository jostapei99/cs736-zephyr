# Custom RT Scheduler Changes - Reversion Guide

This document lists all custom changes made to Zephyr for the CS736 project.
To return to clean Zephyr, comment out or remove these modifications.

## Modified Files

### 1. kernel/Kconfig
**Lines 120-268**: Added custom scheduler configuration
- `config SCHED_DEADLINE_ONLY` (line 130)
- `choice` block for RT scheduling policies (line 120-199)
- `config 736_MOD_EDF` (line 137)
- `config 736_WSRT` (line 148)
- `config 736_RMS` (line 161)
- `config 736_LLF` (line 171)
- `config 736_PFS` (line 187)
- `config 736` (line 203)
- `config 736_TIME_LEFT` (line 215)
- `config 736_RT_STATS` (line 226)
- `config 736_RT_STATS_DETAILED` (line 244)
- `config 736_RT_STATS_SQUARED` (line 256)

**Action**: Comment out lines 120-268

### 2. include/zephyr/kernel.h
- Added syscall declarations for `k_thread_weight_set()` and `k_thread_exec_time_set()`
- These are DUPLICATES - they should be removed from kernel.h (already in sched_rt.h)

**Action**: Remove duplicate syscall declarations (already fixed)

### 3. include/zephyr/kernel/sched_rt.h
**Entire file is custom** - defines RT scheduling API
- k_thread_weight_set()
- k_thread_exec_time_set()
- Scheduler name functions

**Action**: Delete or comment out entire file

### 4. include/zephyr/kernel/rt_stats.h
**Entire file is custom** - defines RT statistics API
- struct k_thread_rt_stats
- k_thread_rt_stats_get()
- k_thread_rt_stats_reset()
- k_thread_rt_stats_activation()
- k_thread_rt_stats_deadline_miss()
- Helper functions for calculating stats

**Action**: Delete or comment out entire file

### 5. include/zephyr/kernel.h (thread structure)
Added fields to `struct _thread_base`:
- `uint32_t prio_weight`
- `uint32_t prio_exec_time`
- `uint32_t prio_time_left`
- `struct k_thread_rt_stats_internal rt_stats`

**Action**: Comment out these field additions

### 6. kernel/include/priority_q.h
Added scheduler comparison functions:
- `z_sched_cmp_wsrt()`
- `z_sched_cmp_rms()`
- `z_sched_cmp_llf()`
- `z_sched_cmp_pfs()`

**Action**: Comment out these functions (wrapped in #ifdef CONFIG_736_*)

### 7. kernel/sched.c
**Multiple modifications**:
- Line 289-293: Added RT stats preemption tracking
- Line 344-347: Added RT stats ready time tracking
- Line 876-931: Added RT timing calculations in z_get_next_switch_handle() (SMP path)
- Line 976-1034: Added RT timing calculations (non-SMP path)
- Line 995-1112: Added RT stats syscall implementations
  - z_impl_k_thread_rt_stats_get()
  - z_impl_k_thread_rt_stats_reset()
  - z_impl_k_thread_rt_stats_activation()
  - z_impl_k_thread_rt_stats_deadline_miss()

**Action**: Comment out all CONFIG_736_RT_STATS blocks

### 8. kernel/sched_rt.c
**Entire file is custom** - implements RT scheduling syscalls
- z_impl_k_thread_weight_set()
- z_impl_k_thread_exec_time_set()
- Scheduler name string functions

**Action**: Delete or comment out entire file

### 9. kernel/CMakeLists.txt
Added: `zephyr_library_sources_ifdef(CONFIG_736 sched_rt.c)`

**Action**: Comment out this line

## Application Directories (Can be deleted entirely)

- `app/workloads/` - Workload evaluation suite
- `app/testing/sched0_edf/` - EDF test application
- `app/simple_eval_step1/` - Simple evaluation
- `app/advanced_eval/` - Advanced evaluation

**Action**: Delete these directories or keep for reference

## Clean Start Procedure

1. Comment out all Kconfig entries in `kernel/Kconfig`
2. Delete or rename custom header files:
   - `include/zephyr/kernel/sched_rt.h`
   - `include/zephyr/kernel/rt_stats.h`
3. Comment out custom fields in thread structure (kernel.h)
4. Comment out all `#ifdef CONFIG_736*` blocks in:
   - `kernel/sched.c`
   - `kernel/include/priority_q.h`
5. Comment out sched_rt.c in `kernel/CMakeLists.txt`
6. Optional: Delete application directories

After these changes, Zephyr should build in its original state.
