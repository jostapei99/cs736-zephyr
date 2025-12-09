# Kernel Overhead Debug & Fix - December 8, 2025

## Problem Statement
All kernel overhead statistics were showing 0.00:
- Context switches: 0
- Preemptions: 0  
- Overhead percentage: 0.00%
- CS/Act (Context Switches per Activation): 0.00

## Investigation Process

### Step 1: Verify RT Stats Infrastructure
✓ Confirmed `k_thread_rt_stats_get()` exists in `kernel/sched.c`
✓ Confirmed application code correctly calls this API
✓ Verified RT stats structure has `context_switches` and `preemptions` fields

### Step 2: Find Context Switch Tracking Code
Located in `kernel/thread.c:1000`:
```c
#ifdef CONFIG_INSTRUMENT_THREAD_SWITCHING
void z_thread_mark_switched_in(void)
{
    #ifdef CONFIG_736_RT_STATS
    _current->base.rt_stats.context_switches++;
    #endif
}
```

**Critical Finding:** Context switch tracking requires TWO configs:
1. `CONFIG_INSTRUMENT_THREAD_SWITCHING` (outer guard)
2. `CONFIG_736_RT_STATS` (inner guard)

### Step 3: Check Configuration Dependencies

Checked `prj.conf` - Found:
- ✓ `CONFIG_736_RT_STATS=y` was set
- ✗ `CONFIG_INSTRUMENT_THREAD_SWITCHING` was NOT in prj.conf

Checked `kernel/Kconfig`:
```kconfig
config SCHED_THREAD_USAGE
    bool "Collect thread runtime usage"
    default y
    select INSTRUMENT_THREAD_SWITCHING if !USE_SWITCH
```

**Finding:** `INSTRUMENT_THREAD_SWITCHING` is not directly settable - it must be enabled via `SCHED_THREAD_USAGE`.

### Step 4: Check Parent Dependencies

Ran: `grep "CONFIG_SCHED_THREAD_USAGE" build/zephyr/.config`
Result: Config was missing entirely!

Checked Kconfig hierarchy:
```
menuconfig THREAD_RUNTIME_STATS  <-- PARENT (was disabled)
    config SCHED_THREAD_USAGE    <-- CHILD (needs parent enabled)
```

**Root Cause Found:** `CONFIG_THREAD_RUNTIME_STATS` was not enabled, so `SCHED_THREAD_USAGE` was never available!

### Step 5: Check RT Stats Dependencies

Build warnings showed:
```
warning: 736_RT_STATS was assigned the value 'y' but got the value 'n'. 
Check these unsatisfied dependencies: 736_ADD_ONS (=n).
```

**Second Root Cause:** `CONFIG_736_RT_STATS` requires `CONFIG_736_ADD_ONS=y`

## Root Causes Identified

1. **Missing `CONFIG_THREAD_RUNTIME_STATS=y`**
   - Required parent config for `SCHED_THREAD_USAGE`
   - Without it, `INSTRUMENT_THREAD_SWITCHING` cannot be enabled

2. **Missing `CONFIG_736_ADD_ONS=y`**  
   - Required for `CONFIG_736_RT_STATS` to actually enable
   - Was intentionally disabled for Base-EDF in original design
   - But RT stats infrastructure requires it

## Configuration Dependency Chain

```
CONFIG_THREAD_RUNTIME_STATS=y
    └─> CONFIG_SCHED_THREAD_USAGE=y
            └─> select INSTRUMENT_THREAD_SWITCHING (if !USE_SWITCH)

CONFIG_736_ADD_ONS=y
    └─> CONFIG_736_RT_STATS=y (dependency satisfied)
```

Both chains must be complete for context switch tracking to work!

## Fix Applied

### File: `app/scheduler_evaluation/prj.conf`

**Before:**
```conf
CONFIG_736_ADD_ONS=n
CONFIG_736_RT_STATS=y
CONFIG_736_RT_STATS_DETAILED=y
CONFIG_736_RT_STATS_SQUARED=y
```

**After:**
```conf
CONFIG_736_ADD_ONS=y              # Changed: Required for RT stats
CONFIG_736_RT_STATS=y
CONFIG_736_RT_STATS_DETAILED=y
CONFIG_736_RT_STATS_SQUARED=y

# Required for context switch tracking (enables INSTRUMENT_THREAD_SWITCHING)
CONFIG_THREAD_RUNTIME_STATS=y      # Added: Parent config
CONFIG_SCHED_THREAD_USAGE=y        # Added: Enables context switch tracking
```

### File: `app/scheduler_evaluation/scripts/run_evaluation.sh`

**Before:**
```bash
if [ "$sched_config" = "CONFIG_SCHED_DEADLINE" ]; then
    # Base EDF - disable all custom schedulers
    sed -i 's/^CONFIG_736_ADD_ONS=y/CONFIG_736_ADD_ONS=n/' "$APP_DIR/prj.conf"
```

**After:**
```bash
if [ "$sched_config" = "CONFIG_SCHED_DEADLINE" ]; then
    # Base EDF - keep ADD_ONS enabled for RT stats, disable all custom schedulers
    # Note: We keep CONFIG_736_ADD_ONS=y to enable RT stats tracking
    # (removed the line that disabled ADD_ONS)
```

## Verification

### Build Configuration Check
```bash
$ grep -E "CONFIG_INSTRUMENT_THREAD_SWITCHING|CONFIG_SCHED_THREAD_USAGE|CONFIG_THREAD_RUNTIME_STATS|CONFIG_736" build/zephyr/.config | grep -v "not set"

CONFIG_736_ADD_ONS=y
CONFIG_736_RT_STATS=y
CONFIG_736_RT_STATS_DETAILED=y
CONFIG_736_RT_STATS_SQUARED=y
CONFIG_INSTRUMENT_THREAD_SWITCHING=y
CONFIG_THREAD_RUNTIME_STATS=y
CONFIG_SCHED_THREAD_USAGE=y
CONFIG_SCHED_THREAD_USAGE_ALL=y
CONFIG_SCHED_THREAD_USAGE_AUTO_ENABLE=y
```
✓ All required configs now enabled!

### Test Results (Before Fix)
```
│  3 │ Light     │   105 │     0 │   0.00 │    13 │    40 │    30 │   0.00 │   0.00 │
│  3 │ Moderate  │   121 │     1 │   0.83 │    30 │    70 │    50 │   0.00 │   0.00 │

Kernel Overhead:
  Total context switches:     0
  Total preemptions:          0
  Avg scheduling overhead:    0.00%
```

### Test Results (After Fix)
```
│  3 │ Light     │   105 │     0 │   0.00 │    13 │    40 │    30 │   1.15 │   0.02 │
│  3 │ Moderate  │   121 │     1 │   0.83 │    30 │    70 │    50 │   1.33 │   0.03 │

Kernel Overhead:
  Total context switches:     3253
  Total preemptions:          3259
  Avg scheduling overhead:    0.04%
  Max scheduling overhead:    0.06%
  Context switches per test:  203
```
✓ Overhead metrics now working correctly!

## Typical Values After Fix

For Standard-EDF scheduler across 16 test scenarios (5 seconds each):
- **CS/Act:** 0.99 - 1.52 context switches per activation
- **Overhead %:** 0.02% - 0.06% CPU time spent in scheduler
- **Total context switches:** ~3200 across all tests
- **Total preemptions:** ~3200 across all tests
- **CS per test:** ~200 per scheduler evaluation

These values indicate:
- Reasonable scheduler efficiency (~1 context switch per task activation)
- Very low overhead (<0.1% CPU time)
- Consistent behavior across different load scenarios

## Impact

✓ **Kernel overhead metrics now functional**
✓ **Context switch tracking working**
✓ **Preemption counting working**  
✓ **Overhead percentage calculations accurate**
✓ **Full evaluation suite can now provide complete metrics**

## Lessons Learned

1. **Check entire dependency chain** - Kconfig dependencies can be multilevel
2. **Read build warnings carefully** - They often reveal unsatisfied dependencies
3. **Configs can be "selected" not "set"** - Some configs are indirect (like `INSTRUMENT_THREAD_SWITCHING`)
4. **Parent menus must be enabled** - Child configs are hidden if parent is disabled
5. **RT stats requires ADD_ONS** - Even for Base-EDF, we need the infrastructure enabled

## Files Modified

1. `app/scheduler_evaluation/prj.conf` - Added 2 configs, changed ADD_ONS to =y
2. `app/scheduler_evaluation/scripts/run_evaluation.sh` - Removed ADD_ONS disable for Base-EDF
3. `app/scheduler_evaluation/CHANGELOG.md` - Documented the fix

## Testing Checklist

- [x] Build completes without warnings about unsatisfied dependencies
- [x] `CONFIG_INSTRUMENT_THREAD_SWITCHING=y` in `.config`
- [x] `CONFIG_736_RT_STATS=y` in `.config`
- [x] Context switches show non-zero values in output
- [x] Preemptions show non-zero values in output
- [x] Overhead percentage shows realistic values (0.02-0.06%)
- [x] CS/Act shows reasonable values (1.0-1.5)
- [x] Full evaluation script works with all 6 schedulers
