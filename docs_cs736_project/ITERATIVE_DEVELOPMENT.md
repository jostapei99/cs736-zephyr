# Clean Iterative Development Guide
## Building Custom RT Schedulers for Zephyr from Scratch

This guide walks through adding custom RT scheduling features incrementally,
with unit tests at each step.

---

## Phase 0: Clean Slate

### Start Fresh
```bash
cd /home/jack/cs736-project/zephyr
chmod +x scripts/revert_cs736_changes.sh
./scripts/revert_cs736_changes.sh
```

### Manual cleanup in kernel/Kconfig
Comment out lines 120-268 (all custom config 736 entries)

### Verify clean build
```bash
cd samples/hello_world
west build -b native_sim -p
```

---

## Phase 1: Basic Infrastructure

### Step 1.1: Add thread weight field
**File**: `include/zephyr/kernel.h`

In `struct _thread_base`, add:
```c
#ifdef CONFIG_736
	uint32_t prio_weight;        /* Thread weight for weighted schedulers */
#endif
```

### Step 1.2: Add Kconfig for infrastructure
**File**: `kernel/Kconfig`

After `config SCHED_DEADLINE`, add:
```kconfig
config 736
	bool "Custom RT Scheduling Infrastructure"
	depends on SCHED_DEADLINE
	help
	  Enable infrastructure for custom real-time scheduling.
	  Adds weight field to thread structure.
```

### Step 1.3: Create basic API header
**File**: `include/zephyr/kernel/sched_rt.h`

```c
#ifndef ZEPHYR_INCLUDE_KERNEL_SCHED_RT_H_
#define ZEPHYR_INCLUDE_KERNEL_SCHED_RT_H_

#include <zephyr/kernel.h>
#include <zephyr/syscall.h>

#ifdef CONFIG_736

__syscall int k_thread_weight_set(k_tid_t thread, uint32_t weight);

#include <zephyr/syscalls/sched_rt.h>

#endif /* CONFIG_736 */
#endif /* ZEPHYR_INCLUDE_KERNEL_SCHED_RT_H_ */
```

### Step 1.4: Implement syscall
**File**: `kernel/sched_rt.c` (create new)

```c
#include <zephyr/kernel.h>
#include <zephyr/kernel/sched_rt.h>

int z_impl_k_thread_weight_set(k_tid_t tid, uint32_t weight)
{
	struct k_thread *thread = tid ? tid : _current;
	
	K_SPINLOCK(&_sched_spinlock) {
		thread->base.prio_weight = weight;
	}
	
	return 0;
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER(k_thread_weight_set, tid, weight)
{
	K_OOPS(K_SYSCALL_OBJ(tid, K_OBJ_THREAD));
	return z_impl_k_thread_weight_set(tid, weight);
}
#endif
```

### Step 1.5: Add to build system
**File**: `kernel/CMakeLists.txt`

Add:
```cmake
zephyr_library_sources_ifdef(CONFIG_736 sched_rt.c)
```

### Step 1.6: Unit Test
**Create**: `app/unit_tests/test_weight_api/`

```c
#include <zephyr/kernel.h>
#include <zephyr/kernel/sched_rt.h>

int main(void)
{
	printk("Testing weight API...\n");
	
	int ret = k_thread_weight_set(NULL, 10);
	if (ret == 0) {
		printk("SUCCESS: Weight set\n");
	} else {
		printk("FAIL: Weight set returned %d\n", ret);
	}
	
	return 0;
}
```

**prj.conf**:
```
CONFIG_SCHED_DEADLINE=y
CONFIG_736=y
CONFIG_PRINTK=y
```

**Test**:
```bash
cd app/unit_tests/test_weight_api
west build -b native_sim -p
west build -t run
```

---

## Phase 2: Add Weighted EDF Scheduler

### Step 2.1: Add scheduler comparison function
**File**: `kernel/include/priority_q.h`

```c
#ifdef CONFIG_736_MOD_EDF
static inline bool z_sched_cmp_mod_edf(struct k_thread *a, struct k_thread *b)
{
	uint32_t weight_a = a->base.prio_weight ? a->base.prio_weight : 1;
	uint32_t weight_b = b->base.prio_weight ? b->base.prio_weight : 1;
	
	uint64_t ratio_a = ((uint64_t)a->base.prio_deadline * 1000) / weight_a;
	uint64_t ratio_b = ((uint64_t)b->base.prio_deadline * 1000) / weight_b;
	
	return ratio_a < ratio_b;
}
#endif
```

### Step 2.2: Update scheduler selection
**File**: `kernel/include/priority_q.h`

In `z_sched_prio_cmp()`, add:
```c
#ifdef CONFIG_736_MOD_EDF
	if (z_sched_cmp_mod_edf(thread_1st, thread_2nd)) {
		return 1;
	}
#elif defined(CONFIG_SCHED_DEADLINE)
	/* ... existing EDF code ... */
#endif
```

### Step 2.3: Add Kconfig
**File**: `kernel/Kconfig`

```kconfig
config 736_MOD_EDF
	bool "Weighted EDF Scheduler"
	select 736
	depends on SCHED_DEADLINE
	help
	  Weighted EDF based on deadline/weight ratio.
```

### Step 2.4: Unit Test
**Create**: `app/unit_tests/test_weighted_edf/`

Two threads with different weights, verify scheduling order.

---

## Phase 3: Add RT Statistics

### Step 3.1: Add stats structure
**File**: `include/zephyr/kernel/rt_stats.h`

```c
struct k_thread_rt_stats {
	uint32_t activations;
	uint32_t preemptions;
	uint32_t deadline_misses;
};

__syscall int k_thread_rt_stats_get(k_tid_t thread, 
                                     struct k_thread_rt_stats *stats);
__syscall void k_thread_rt_stats_activation(k_tid_t thread);
```

### Step 3.2: Add stats to thread
**File**: `include/zephyr/kernel.h`

```c
#ifdef CONFIG_736_RT_STATS
	struct {
		uint32_t activations;
		uint32_t preemptions;
		uint32_t deadline_misses;
	} rt_stats;
#endif
```

### Step 3.3: Implement syscalls
**File**: `kernel/sched.c`

```c
#ifdef CONFIG_736_RT_STATS
int z_impl_k_thread_rt_stats_get(k_tid_t tid, struct k_thread_rt_stats *stats)
{
	struct k_thread *thread = tid ? tid : _current;
	
	K_SPINLOCK(&_sched_spinlock) {
		stats->activations = thread->base.rt_stats.activations;
		stats->preemptions = thread->base.rt_stats.preemptions;
		stats->deadline_misses = thread->base.rt_stats.deadline_misses;
	}
	
	return 0;
}

void z_impl_k_thread_rt_stats_activation(k_tid_t tid)
{
	struct k_thread *thread = tid ? tid : _current;
	
	K_SPINLOCK(&_sched_spinlock) {
		thread->base.rt_stats.activations++;
	}
}
#endif
```

### Step 3.4: Unit Test
Test that stats can be read and incremented.

---

## Phase 4: Timing Statistics

Add response time, waiting time tracking incrementally with tests.

---

## Phase 5: Additional Schedulers

Add WSRT, RMS, LLF, PFS one at a time, with unit tests for each.

---

## Testing Strategy

For each phase:
1. Add minimal code
2. Write unit test
3. Verify test passes
4. Document what was added
5. Commit changes
6. Move to next phase

Never add multiple features without testing in between.

---

## Directory Structure

```
app/
  unit_tests/
    test_weight_api/          # Phase 1
    test_weighted_edf/        # Phase 2
    test_rt_stats_basic/      # Phase 3
    test_timing_stats/        # Phase 4
    test_wsrt/                # Phase 5.1
    test_rms/                 # Phase 5.2
    ...
```

Each test app should:
- Test ONE feature
- Be simple (< 100 lines)
- Print clear PASS/FAIL
- Document what it's testing
