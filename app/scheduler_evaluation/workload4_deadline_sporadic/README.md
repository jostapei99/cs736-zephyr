# Workload 4: Multi-Rate Sporadic Workload with Deadline Scheduling

## Overview
This workload tests Earliest Deadline First (EDF) scheduling with sporadic task arrivals at multiple rates. Typical of automotive ECUs, IoT gateways, and multimedia processing systems where tasks have varying arrival rates and deadlines.

## Task Set

| Thread | Arrival Pattern | Execution Time | Deadline | Characteristics |
|--------|----------------|----------------|----------|-----------------|
| Fast Events | 1-10ms (sporadic) | 0.8ms | 5ms | High frequency, tight deadline |
| Medium Events | 10-50ms (sporadic) | 2.5ms | 15ms | Medium frequency |
| Slow Periodic | 100ms (periodic) | 8ms | 100ms | Background processing |
| Deadline Task | 20-60ms (sporadic) | 3-5ms (variable) | 10ms | Tests EDF directly |

## Key Characteristics

### 1. Sporadic Arrivals
- Events arrive with random inter-arrival times
- No guaranteed minimum separation
- Can create bursts of high load

### 2. Multiple Time Scales
- Fast events: millisecond scale
- Medium events: tens of milliseconds
- Slow tasks: hundreds of milliseconds

### 3. Deadline-Based Scheduling
All tasks have same **static priority** but different **deadlines**. When `CONFIG_SCHED_DEADLINE` is enabled:
- Scheduler picks task with earliest deadline
- EDF is optimal for uniprocessor systems
- Can achieve 100% utilization (vs ~69% for RMA)

### 4. Variable Execution Time
Some tasks have unpredictable execution time, testing scheduler robustness.

## Metrics Measured

1. **Deadline Miss Rate**: Percentage of jobs that complete after deadline
2. **Tardiness**: How late tasks complete (beyond deadline)
3. **Latency Distribution**: Queuing delays before execution
4. **Inter-Arrival Statistics**: Min/max time between events
5. **Throughput**: Events processed per second

## Schedulability Analysis

### With EDF (CONFIG_SCHED_DEADLINE=y)
- Utilization: U = 0.8/5 + 2.5/15 + 8/100 = 0.16 + 0.17 + 0.08 = 0.41 (41%)
- EDF can schedule up to 100% utilization
- System is schedulable ✓

### Without EDF (Priority-based)
- Without proper priority assignment, deadline misses likely
- Fixed priority may starve lower priority tasks
- Not optimal for sporadic workloads

## Building and Running

```bash
# Build with EDF enabled (default)
west build -b qemu_cortex_m3 app/scheduler_evaluation/workload4_deadline_sporadic
west build -t run
```

## Testing Different Schedulers

### EDF Scheduler (Recommended for this workload)
```
CONFIG_SCHED_DEADLINE=y
CONFIG_SCHED_SIMPLE=y
```

### Priority-Based (For comparison)
```
# CONFIG_SCHED_DEADLINE is not set
CONFIG_SCHED_SIMPLE=y
```

Compare the deadline miss rates between configurations!

## Expected Results

### With EDF Enabled
- Very low deadline miss rate (<5%)
- Fair scheduling across all tasks
- Tasks with tighter deadlines get priority
- Efficient use of CPU time

### Without EDF
- Higher deadline miss rate
- Potential starvation of some tasks
- Less predictable behavior
- Priority inversion issues

## How EDF Works in Zephyr

Threads use `k_thread_deadline_set()` API:
```c
// Set relative deadline
k_thread_deadline_set(k_current_get(), k_ms_to_cyc_ceil32(deadline_ms));
```

When multiple threads at same static priority are ready:
1. Scheduler checks their deadlines
2. Selects thread with earliest (soonest) deadline
3. Executes that thread

## Interpreting Results

### Good Scheduler Performance
- Deadline miss rate < 5%
- Low average tardiness
- Consistent latency
- No starvation (all tasks make progress)

### Poor Scheduler Performance
- High deadline miss rate
- Large tardiness values
- High latency variance
- Some tasks never execute

## Real-World Applications

This workload models:
- Automotive ECU (Engine Control Unit)
  - Fast: Injector timing events
  - Medium: Sensor readings
  - Slow: Diagnostic checks
  
- IoT Gateway
  - Fast: Critical sensor data
  - Medium: Network packets
  - Slow: Data aggregation
  
- Multimedia Processing
  - Fast: Audio frame processing
  - Medium: Video frame processing
  - Slow: Encoding/compression

## Advanced Testing

### Overload Testing
Increase arrival rates to test behavior under overload:
- Reduce `FAST_MIN_INTERVAL_MS` to 0.5ms
- Observe graceful degradation

### Burst Testing
Create synchronized bursts of events:
- All generators trigger simultaneously
- Test worst-case response time

### Jitter Analysis
Track variance in response times to measure jitter.
