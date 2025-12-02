# EDF Deadline Scheduler Test

## Overview

This test specifically evaluates **CONFIG_SCHED_DEADLINE** - Zephyr's implementation of Earliest Deadline First (EDF) scheduling algorithm.

## What is EDF?

**Earliest Deadline First (EDF)** is an optimal dynamic priority scheduling algorithm:
- Tasks are scheduled based on their absolute deadlines
- The task with the earliest deadline gets highest priority
- **Optimal** for single-core systems - can achieve up to 100% CPU utilization
- Dynamically adjusts priorities as deadlines change

## Test Suite

This application runs **4 comprehensive tests**:

### Test 1: Basic EDF (Low Utilization - 49%)
```
Tasks:
  VeryFast:  Period=5ms,   Exec=0.8ms,  U=16%
  Fast:      Period=15ms,  Exec=2.5ms,  U=17%
  Medium:    Period=50ms,  Exec=4.0ms,  U=8%
  Slow:      Period=100ms, Exec=8.0ms,  U=8%
Total Utilization: 49%
```
**Expected Result**: 0 deadline misses (well below capacity)

### Test 2: High Utilization EDF (98.2%)
```
Tasks:
  T1: Period=10ms,  Exec=8.0ms,  U=80%
  T2: Period=20ms,  Exec=3.0ms,  U=15%
  T3: Period=100ms, Exec=2.0ms,  U=2%
  T4: Period=200ms, Exec=2.0ms,  U=1%
  T5: Period=500ms, Exec=1.0ms,  U=0.2%
Total Utilization: 98.2%
```
**Expected Result**: 0 deadline misses (EDF is optimal up to 100%)

### Test 3: Overload Condition (125%)
```
Tasks:
  Critical:  Period=10ms, Exec=7.0ms, U=70%
  Important: Period=20ms, Exec=9.0ms, U=45%
  Regular:   Period=50ms, Exec=5.0ms, U=10%
Total Utilization: 125% (OVERLOAD!)
```
**Expected Result**: Some deadline misses (unavoidable), but EDF minimizes lateness

### Test 4: Constrained Deadlines (D < P)
```
Tasks with deadline less than period:
  Tight:     Period=20ms,  Deadline=8ms   (40% of period)
  Medium:    Period=40ms,  Deadline=25ms  (62.5% of period)
  Loose:     Period=100ms, Deadline=80ms  (80% of period)
  VeryLoose: Period=200ms, Deadline=180ms (90% of period)
Total Utilization: 40.5%
```
**Expected Result**: 0 deadline misses (handles constrained deadlines)

## Building and Running

### Quick Start
```bash
cd /home/jack/cs736-project/zephyr
source ~/.venv/zephyr/bin/activate

# Build
west build -p -b qemu_cortex_m3 app/scheduler_evaluation/deadline_scheduler_test

# Run (takes ~25 seconds for all 4 tests)
timeout 30 west build -t run
```

### Expected Output
```
================================================================
  EDF Deadline Scheduler (CONFIG_SCHED_DEADLINE) Test Suite
================================================================

✓ CONFIG_SCHED_DEADLINE is ENABLED
✓ CONFIG_SCHED_SIMPLE is enabled (required for EDF)

Timing: 12000000 cycles/sec, 12 cycles/us

========================================
Test 1: Basic EDF (Low Utilization)
========================================
Utilization: 49% (well below 100% limit)
Expected: 0 deadline misses

Running for 5 seconds...

=== Test 1 Results ===
VeryFast (P=5ms, C=800us, D=5ms):
  Executions: 890
  Deadline Misses: 0
  Max Response: 850 us
...
Total Deadline Misses: 0
✓ PASS: EDF scheduled all tasks successfully!

[... Tests 2, 3, 4 continue ...]
```

## Key Concepts Tested

### 1. EDF Optimality
EDF can schedule task sets up to **100% utilization** (theoretical limit).
Priority-based schedulers typically max out at 69-88% (Liu & Layland bound).

### 2. Schedulability Test
For EDF with implicit deadlines (D = P):
```
U = Σ(Ci/Pi) ≤ 1.0
```
Where:
- Ci = Computation time (execution time)
- Pi = Period
- U = CPU Utilization

If U ≤ 1.0, all deadlines can be met.

### 3. Constrained Deadlines
EDF also handles tasks where deadline < period:
```
Di ≤ Pi  (constrained deadline)
Di = Pi  (implicit deadline)
```

### 4. Overload Behavior
When U > 1.0:
- Some deadlines will be missed (unavoidable)
- EDF minimizes maximum lateness
- Still optimal in this sense

## Comparison with Other Schedulers

### Priority-Based (Rate Monotonic)
```
Utilization Bound: 69-88% (depends on task count)
Priority: Fixed (shorter period = higher priority)
Optimal: No (suboptimal for deadline scheduling)
```

### EDF
```
Utilization Bound: 100% (optimal)
Priority: Dynamic (earliest deadline = highest priority)
Optimal: Yes (for single-core)
```

**Example**: At 95% utilization:
- Rate Monotonic: **Will miss deadlines**
- EDF: **Meets all deadlines** ✓

## When to Use CONFIG_SCHED_DEADLINE

### Use EDF when:
✓ Tasks have explicit deadline requirements  
✓ Need to maximize CPU utilization (>70%)  
✓ Want optimal deadline-based scheduling  
✓ Working on single-core systems  
✓ Deadlines are more important than priorities  

### Don't use EDF when:
✗ Working on multi-core/SMP systems  
✗ Tasks don't have deadline requirements  
✗ Using CONFIG_SCHED_MULTIQ (incompatible)  
✗ Need O(1) scheduling (EDF is O(N))  

## Configuration Requirements

**Required**:
```kconfig
CONFIG_SCHED_DEADLINE=y
CONFIG_SCHED_SIMPLE=y  # EDF requires SIMPLE as base
```

**Recommended**:
```kconfig
CONFIG_TIMESLICING=n   # Let EDF control scheduling
CONFIG_SYS_CLOCK_TICKS_PER_SEC=10000  # High resolution
```

**Incompatible**:
```kconfig
CONFIG_SCHED_MULTIQ=y  # Cannot use with EDF
CONFIG_SCHED_SCALABLE=y  # Should use SIMPLE
```

## EDF API Usage

### Setting Thread Deadline
```c
struct k_thread *thread = k_current_get();
k_thread_deadline_set(thread, deadline_ms);
```

The thread will be scheduled based on its absolute deadline. Threads with earlier deadlines run first.

### Period vs Deadline
```c
// Implicit deadline (D = P)
k_thread_deadline_set(thread, period_ms);
k_sleep(K_MSEC(period_ms));

// Constrained deadline (D < P)
k_thread_deadline_set(thread, 10);  // 10ms deadline
k_sleep(K_MSEC(20));                 // 20ms period
```

## Metrics Measured

For each test:
- **Executions**: Number of task instances
- **Deadline Misses**: Instances where response time > deadline
- **Max Response Time**: Longest execution time observed
- **Max Tardiness**: How late tasks complete (overload test)

## Interpreting Results

### Test 1 (49% utilization)
- Should have **0 misses**
- If misses occur: EDF implementation issue

### Test 2 (98% utilization)
- Should have **0 or very few misses**
- Demonstrates EDF optimality near 100%

### Test 3 (125% overload)
- **Will have misses** (expected)
- EDF minimizes the maximum lateness
- All tasks should get some execution time

### Test 4 (Constrained deadlines)
- Should have **0 misses**
- Proves EDF handles D < P correctly

## Real-World Applications

EDF is ideal for:
- **Industrial control**: Hard real-time deadlines
- **Video/audio processing**: Frame deadlines
- **Communications**: Packet deadlines
- **Automotive**: Safety-critical timing
- **Robotics**: Sensor fusion with timing constraints

## Technical Details

- **Algorithm Complexity**: O(N) per scheduling decision
- **Memory**: Minimal overhead (deadline stored per thread)
- **Jitter**: Low (deterministic based on deadlines)
- **Starvation**: No (all tasks eventually run if U ≤ 1.0)

## References

- Liu & Layland (1973): "Scheduling Algorithms for Multiprogramming in a Hard-Real-Time Environment"
- Zephyr Documentation: Scheduling in the Kernel Primer
- CONFIG_SCHED_DEADLINE Kconfig option

## Troubleshooting

**No deadline misses at 125% utilization?**
- Check if execution times are accurate
- Verify timing calibration

**Unexpected misses at low utilization?**
- Check for priority inversions
- Verify CONFIG_SCHED_DEADLINE is enabled
- Check system overhead (interrupts, etc.)

**Build errors?**
- Ensure CONFIG_SCHED_SIMPLE=y
- Don't combine with CONFIG_SCHED_MULTIQ
