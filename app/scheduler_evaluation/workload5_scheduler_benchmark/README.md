# Workload 5: Scheduler Scaling Benchmark

## Purpose

This workload is specifically designed to **systematically evaluate and compare** Zephyr's built-in schedulers:
- **SCHED_SIMPLE**: O(N) linked list
- **SCHED_SCALABLE**: O(log N) red-black tree
- **SCHED_MULTIQ**: O(1) array of priority queues

Unlike the other workloads that simulate real applications, this is a **microbenchmark** that isolates scheduler behavior.

## What Makes This Different

The existing workloads test realistic scenarios but don't directly compare scheduler algorithms. This workload:

1. **Tests scalability** - Runs with 1, 5, 10, 20, 30 threads to see how each scheduler scales
2. **Isolates scheduler overhead** - Measures pure scheduling time, not application complexity
3. **Tests multiple dimensions**:
   - Wake-up latency (responsiveness)
   - Throughput (how many context switches per second)
   - Yield behavior (cooperative scheduling)
   - Priority enforcement (correctness)

## Test Phases

### Phase 1: Wake-up Latency
**What it tests**: How quickly the scheduler can wake up sleeping threads

- All threads sleep for 10ms then wake up
- Measures time from timer expiry to thread execution
- **Expected results**:
  - SIMPLE: Fast with few threads, degrades linearly
  - SCALABLE: Consistent, slight overhead
  - MULTIQ: Fastest, O(1) lookup

### Phase 2: Throughput
**What it tests**: Maximum context switches per second

- All threads are runnable (competing for CPU)
- Each does 100μs of work then yields
- Measures total iterations completed
- **Expected results**:
  - SIMPLE: Slows down dramatically with >10 threads
  - SCALABLE: Maintains performance
  - MULTIQ: Highest throughput

### Phase 3: Yield Behavior
**What it tests**: Cooperative context switching efficiency

- Threads voluntarily yield (k_yield)
- Measures how efficiently scheduler handles cooperative scheduling
- **Expected results**:
  - All should be similar, tests implementation quality

### Phase 4: Priority Enforcement
**What it tests**: Correct priority-based scheduling

- Threads at different priorities (0-15)
- Higher priority threads should get more CPU
- Measures if scheduler respects priorities correctly
- **Expected results**:
  - All schedulers should give higher priority threads more iterations

## Building and Running

### Build with SIMPLE scheduler (default):
```bash
west build -p -b qemu_cortex_m3 app/scheduler_evaluation/workload5_scheduler_benchmark
west build -t run
```

### Build with SCALABLE scheduler:
Edit `prj.conf`:
```
# CONFIG_SCHED_SIMPLE=y
CONFIG_SCHED_SCALABLE=y
```
Then rebuild:
```bash
west build -p -b qemu_cortex_m3 app/scheduler_evaluation/workload5_scheduler_benchmark
west build -t run
```

### Build with MULTIQ scheduler:
Edit `prj.conf`:
```
# CONFIG_SCHED_SIMPLE=y
CONFIG_SCHED_MULTIQ=y
```
Then rebuild:
```bash
west build -p -b qemu_cortex_m3 app/scheduler_evaluation/workload5_scheduler_benchmark
west build -t run
```

## Expected Output

```
========================================
  Zephyr Scheduler Benchmark v1.0
========================================
Test duration: 10 seconds
Work per iteration: 100 us
Testing thread counts: 1, 5, 10, 20, 30

========================================
Testing with 1 thread(s)
========================================

--- Phase 1: Wake-up Latency ---

Results:
  Thread 0 (P0): 250 iterations, Avg latency: 45 us

Aggregate Statistics:
  Total iterations: 250
  Throughput: 100 iterations/sec
  Average latency: 45 us
  Max latency: 120 us
  Min latency: 32 us

[... continues for each phase and thread count ...]
```

## How to Analyze Results

### 1. Compare Latency Across Schedulers
- Lower is better
- Look for degradation as thread count increases
- **Simple**: Should degrade linearly (O(N))
- **Scalable**: Should stay relatively constant (O(log N))
- **MultiQ**: Should stay constant (O(1))

### 2. Compare Throughput
- Higher iterations/sec = better
- **Simple**: Should drop significantly with many threads
- **MultiQ**: Should maintain high throughput

### 3. Check Priority Enforcement
- In Phase 4, high priority threads should have more iterations
- If low priority threads run as much as high priority, scheduler is broken

### 4. Context Switch Rate
- Shows scheduler overhead
- Higher rate with lower overhead = better scheduler

## Comparison with Other Workloads

| Workload | Purpose | What It Tests |
|----------|---------|---------------|
| Workload 1 | Periodic control | Real-time constraints, predictable tasks |
| Workload 2 | Event-driven | Interrupt latency, priority inversion |
| Workload 3 | Mixed criticality | Mode changes, overload handling |
| Workload 4 | Deadline scheduling | EDF algorithm, deadline enforcement |
| **Workload 5** | **Scheduler benchmark** | **Pure scheduler performance, scalability** |

## When to Use This Workload

Use this workload when you need to:
1. **Choose a scheduler** for your application
2. **Understand scheduler overhead** in your system
3. **Verify scheduler scalability** with many threads
4. **Debug scheduling issues** (unexpected latency, priority problems)
5. **Optimize configuration** (number of priority levels, etc.)

## Recommendations Based on Results

After running the benchmark, use these guidelines:

### Use SCHED_SIMPLE if:
- You have <10 threads
- Code size is critical (~2KB savings)
- Your workload is simple and predictable

### Use SCHED_MULTIQ if:
- You need real-time determinism
- Thread count is moderate (<100)
- You need O(1) performance
- You're NOT using deadline scheduling

### Use SCHED_SCALABLE if:
- You have >20 threads
- Thread count varies dynamically
- You need SMP affinity features
- You're using other rbtree features anyway

## Key Metrics to Record

Create a table like this:

| Scheduler | Threads | Avg Latency (μs) | Throughput (iter/s) | Context Switches/s |
|-----------|---------|------------------|---------------------|-------------------|
| SIMPLE    | 1       |                  |                     |                   |
| SIMPLE    | 5       |                  |                     |                   |
| SIMPLE    | 10      |                  |                     |                   |
| SIMPLE    | 20      |                  |                     |                   |
| SIMPLE    | 30      |                  |                     |                   |
| SCALABLE  | 1       |                  |                     |                   |
| ...       | ...     |                  |                     |                   |

## Technical Details

### Why These Thread Counts?
- **1 thread**: Baseline, no contention
- **5 threads**: Typical embedded system
- **10 threads**: Break-even point for SIMPLE vs others
- **20 threads**: Where SIMPLE degrades significantly
- **30 threads**: Stress test, shows scaling differences

### Why 100μs Work Duration?
- Long enough to measure accurately
- Short enough to get many iterations
- Typical of control loop execution time

### Why These Phases?
- Each tests a different scheduler code path
- Together they cover all major operations:
  - Sleep/wake (timer queue)
  - Yield (ready queue manipulation)
  - Priority (queue ordering)

## Limitations

This benchmark does NOT test:
- Interrupt handling
- Mutex/semaphore scheduling
- Deadline scheduling (use Workload 4)
- SMP/multi-core behavior
- Memory pressure effects

## Code Size Comparison

Check code size after building:
```bash
# After building with each scheduler
west build -t rom_report
west build -t ram_report
```

Expected sizes:
- SIMPLE: Smallest (~2KB less than others)
- MULTIQ: Medium (requires priority array)
- SCALABLE: Largest (includes rbtree code)

## Author Notes

This workload was designed to answer the question: **"Which scheduler should I use?"**

The answer depends on your constraints:
- **Code size limited?** → SIMPLE
- **Real-time with <100 threads?** → MULTIQ  
- **Many threads or dynamic?** → SCALABLE
- **Need deadline scheduling?** → SIMPLE + SCHED_DEADLINE

Run this benchmark on your target hardware with your typical thread count to make an informed decision.
