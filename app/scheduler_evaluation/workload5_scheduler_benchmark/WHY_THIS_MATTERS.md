# Workload 5: Why This Workload Matters

## The Problem

You have 4 workloads that simulate real applications:
1. **Periodic Control** - Industrial/robotics
2. **Event-Driven** - Network processing
3. **Mixed Criticality** - Safety-critical systems
4. **Deadline Scheduling** - Automotive/multimedia

But these workloads test **application behavior**, not **scheduler performance** directly.

**Example**: Workload 1 might run fine on all schedulers because it only has 4 threads. You won't see scheduler differences until you scale up.

## The Solution: A Scheduler-Focused Benchmark

Workload 5 is different:
- ✅ **Isolated testing**: Measures pure scheduler overhead
- ✅ **Scalability testing**: 1→5→10→20→30 threads
- ✅ **Multiple dimensions**: Latency, throughput, priority, yield
- ✅ **Direct comparison**: Same workload, different schedulers

## What Each Scheduler Excels At

### SCHED_SIMPLE (O(N) Linked List)
**Strengths**:
- Smallest code size (~2KB savings)
- Fast with <10 threads
- Simple, predictable behavior

**Weaknesses**:
- Degrades linearly with thread count
- Slow with >20 threads
- Not scalable

**Use when**:
- Code size is critical
- You have <10 threads
- Workload is simple and predictable

**This benchmark shows**:
- How much it degrades with many threads
- At what point you should switch schedulers

---

### SCHED_SCALABLE (O(log N) Red-Black Tree)
**Strengths**:
- Scales well to thousands of threads
- Consistent performance
- Required for SMP affinity

**Weaknesses**:
- ~2KB larger code size
- Slightly slower than MULTIQ for real-time
- More complex implementation

**Use when**:
- You have >20 threads
- Thread count varies dynamically
- You need SMP/multi-core support
- You're already using rbtree elsewhere

**This benchmark shows**:
- Performance stays consistent as threads increase
- Small overhead compared to SIMPLE at low thread counts

---

### SCHED_MULTIQ (O(1) Array of Lists)
**Strengths**:
- True O(1) performance
- Best for real-time systems
- Fast wake-up and context switch
- No scheduler jitter

**Weaknesses**:
- Can't be used with deadline scheduling
- Moderate RAM overhead (priority array)
- Can't traverse all threads efficiently

**Use when**:
- You need deterministic real-time performance
- Thread count is moderate (<100)
- You're not using deadline scheduling
- Predictable latency is critical

**This benchmark shows**:
- Highest throughput (most context switches/sec)
- Lowest latency variance
- Best for real-time constraints

---

## How This Complements Other Workloads

| Workload | What It Tests | When To Use |
|----------|---------------|-------------|
| **1: Periodic** | Real app behavior | Validate your periodic control system |
| **2: Event-Driven** | Priority inversion | Test network/interrupt heavy systems |
| **3: Mixed Criticality** | Mode changes | Safety-critical with overload |
| **4: Deadline** | EDF scheduling | When you need deadline guarantees |
| **5: Benchmark** | **Scheduler choice** | **Before implementing your system** |

## Typical Workflow

### Phase 1: Choose Your Scheduler (Use Workload 5)
```bash
# Run benchmark with each scheduler
./test_all_schedulers.sh

# Analyze results
# Decision: MULTIQ for our 15-thread real-time system
```

### Phase 2: Validate With Application Workload
```bash
# Now test with realistic workload
west build -b qemu_cortex_m3 app/scheduler_evaluation/workload1_periodic_control
# Modify prj.conf to use chosen scheduler (MULTIQ)
# Re-run and verify performance
```

### Phase 3: Stress Test Edge Cases
```bash
# Test overload handling
west build -b qemu_cortex_m3 app/scheduler_evaluation/workload3_mixed_criticality

# Test deadline scheduling if needed
west build -b qemu_cortex_m3 app/scheduler_evaluation/workload4_deadline_sporadic
```

## Real-World Example

### Scenario: Drone Flight Controller
- 20 threads (sensors, control, actuators, telemetry, etc.)
- Hard real-time constraints
- Code size not critical (32KB available)

**Step 1**: Run Workload 5
```
Results with 20 threads:

SIMPLE:
  Throughput: 3200 iter/sec
  Avg latency: 250 us
  Max latency: 800 us  ← Too high!

SCALABLE:
  Throughput: 8500 iter/sec
  Avg latency: 80 us
  Max latency: 150 us  ← Better

MULTIQ:
  Throughput: 11000 iter/sec  ← Best!
  Avg latency: 45 us
  Max latency: 65 us  ← Most consistent
```

**Decision**: Choose MULTIQ

**Step 2**: Validate with Workload 1 (periodic control)
- Configure with MULTIQ
- Verify all control loops meet deadlines
- Confirm sensor→control→actuator latency <10ms

**Step 3**: Stress test with Workload 3 (mixed criticality)
- Simulate sensor failures
- Test mode changes (emergency landing)
- Verify safety-critical tasks always run

**Result**: MULTIQ chosen based on data, validated with realistic workloads

## Key Metrics From This Benchmark

### 1. Wake-up Latency (Phase 1)
**Measures**: How fast scheduler wakes sleeping threads

**Critical for**:
- Sensor reading responsiveness
- Interrupt-to-thread latency
- Timer precision

**Interpretation**:
```
Average latency: 50 us   ← Good
Average latency: 500 us  ← Bad, scheduler too slow
```

### 2. Throughput (Phase 2)
**Measures**: Context switches per second

**Critical for**:
- Systems with many competing threads
- Understanding scheduler overhead
- Capacity planning

**Interpretation**:
```
10000 iterations/sec  ← High throughput, low overhead
2000 iterations/sec   ← Low throughput, high overhead
```

### 3. Context Switch Rate
**Measures**: Pure scheduler efficiency

**Critical for**:
- CPU utilization calculations
- Power consumption (more switches = more power)
- Cache performance

**Interpretation**:
```
5000 switches/sec  ← Efficient scheduler
500 switches/sec   ← Inefficient, wasting CPU time
```

### 4. Priority Enforcement (Phase 4)
**Measures**: Scheduler correctness

**Critical for**:
- Safety-critical systems
- Real-time guarantees
- Deterministic behavior

**Interpretation**:
```
High priority: 10000 iterations
Low priority:   1000 iterations  ← Correct, 10:1 ratio

High priority: 5000 iterations
Low priority:  5000 iterations   ← WRONG! Scheduler bug
```

## When NOT to Use This Workload

Don't use this benchmark when:

1. **You only have 1-3 threads**
   - All schedulers perform similarly
   - Use SIMPLE by default (smallest code)

2. **You're testing specific features**
   - Use Workload 4 for deadline scheduling
   - Use Workload 2 for priority inversion
   - Use Workload 3 for mode changes

3. **You need application-specific validation**
   - This tests scheduler in isolation
   - Still need to test your actual application

4. **Code size is your only concern**
   - Just use `west build -t rom_report`
   - SIMPLE is always smallest

## Benchmark Validity

This benchmark is valid when:
- ✅ Tested on target hardware (QEMU timing varies)
- ✅ Multiple runs averaged (reduces variance)
- ✅ Realistic thread count (match your application)
- ✅ Realistic work duration (100us is typical control loop)

This benchmark is NOT valid when:
- ❌ Only run once (timing jitter)
- ❌ Thread count too low (<5)
- ❌ Thread count too high for platform (>100 on small MCU)
- ❌ Work duration unrealistic (1ns or 1s)

## Summary: Why Workload 5 Matters

**Before this workload**:
- "We use SIMPLE because it's default"
- "Don't know if scheduler is bottleneck"
- "Can't justify switching schedulers"

**After this workload**:
- "MULTIQ gives 3x better throughput with our 20 threads"
- "Scheduler overhead reduced from 40% to 10%"
- "Data-driven decision to switch from SIMPLE to MULTIQ"

**Bottom line**: This workload answers the question **"Which scheduler should I use?"** with data, not guesses.

## Next Steps

1. **Run the benchmark** on your target hardware
2. **Record results** in a spreadsheet
3. **Choose your scheduler** based on data
4. **Validate** with application workloads (1-4)
5. **Document** your decision for future reference
6. **Re-test** when thread count changes significantly

The other workloads test **if your application works**.

This workload tests **which scheduler to use** for your application.

Both are essential for building robust real-time systems.
