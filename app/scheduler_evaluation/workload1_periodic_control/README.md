# Workload 1: Periodic Control System

## Overview
This workload simulates a periodic control system typical in industrial automation, robotics, or embedded control applications. It features multiple periodic tasks with different priorities and timing requirements.

## Task Set

| Thread | Priority | Period | Execution Time | Deadline |
|--------|----------|--------|----------------|----------|
| Sensor | 1 (High) | 10ms | ~2ms | 10ms |
| Control | 3 (Med) | 20ms | ~5ms | 20ms |
| Actuator | 5 (Low) | 50ms | ~3ms | 50ms |
| Logger | 7 (BG) | Aperiodic | ~1ms | None |

## Metrics Measured

1. **Latency**: Time between scheduled wakeup and actual execution start
2. **Response Time**: Total time from start to completion of each task instance
3. **Deadline Misses**: Count of instances where response time exceeded period
4. **Tardiness Rate**: Percentage of deadline misses
5. **Throughput**: Total task executions per second

## Schedulability Analysis

Using Rate-Monotonic Analysis (RMA):
- Utilization: U = (2/10) + (5/20) + (3/50) = 0.2 + 0.25 + 0.06 = 0.51 (51%)
- RMA bound for 3 tasks: 3(2^(1/3) - 1) ≈ 0.78
- System is schedulable under rate-monotonic scheduling

## Building and Running

```bash
# Build for qemu_cortex_m3
west build -b qemu_cortex_m3 app/scheduler_evaluation/workload1_periodic_control

# Run in QEMU
west build -t run
```

## Testing Different Schedulers

Edit `prj.conf` to enable different schedulers:

### Simple Scheduler (Default)
```
CONFIG_SCHED_SIMPLE=y
```

### Scalable Scheduler
```
CONFIG_SCHED_SIMPLE=n
CONFIG_SCHED_SCALABLE=y
```

### Multi-Queue Scheduler
```
CONFIG_SCHED_SIMPLE=n
CONFIG_SCHED_MULTIQ=y
```

### Deadline Scheduler
```
CONFIG_SCHED_DEADLINE=y
```

## Expected Results

With proper scheduler configuration:
- Sensor thread should have minimal deadline misses
- All high-priority tasks should meet deadlines
- Background logging may be delayed but should execute when CPU is available
- Average latency should be low and predictable

## Interpretation

- **Low latency** indicates efficient scheduler with minimal delay
- **Zero deadline misses** shows real-time guarantees are met
- **High throughput** indicates good CPU utilization
- **Low tardiness** confirms predictable real-time behavior
