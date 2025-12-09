# Modular RT Scheduler Example

## Description

This example demonstrates the modular real-time scheduler API introduced
in the CS736 project. It shows how to:

1. Configure periodic tasks with RT parameters (period, exec_time, weight)
2. Use the simplified `k_thread_rt_config()` API
3. Switch between different scheduling algorithms via Kconfig

## Features

- **3 periodic tasks** with different periods and weights
- **Unified API** that works with all scheduler algorithms
- **Runtime statistics** showing response times
- **Easy algorithm comparison** by changing prj.conf

## Building

### With Weighted EDF
```bash
west build -b native_sim samples/scheduler_example -- -DCONFIG_736_MOD_EDF=y
west build -t run
```

### With WSRT
```bash
west build -b native_sim samples/scheduler_example -- -DCONFIG_736_WSRT=y
west build -t run
```

### With RMS
```bash
west build -b native_sim samples/scheduler_example -- -DCONFIG_736_RMS=y
west build -t run
```

### With Standard EDF
```bash
west build -b native_sim samples/scheduler_example
west build -t run
```

## Expected Output

```
=== Modular RT Scheduler Example ===
Scheduler: Weighted EDF
Creating 3 periodic tasks...

[HighPrio] Configured: period=100 ms, exec=20 ms, weight=3
[MedPrio] Configured: period=200 ms, exec=40 ms, weight=2
[LowPrio] Configured: period=500 ms, exec=50 ms, weight=1
Tasks started. Monitoring execution...

[HighPrio] Executed: response_time=20 ms
[MedPrio] Executed: response_time=40 ms
...
```

## Task Configuration

| Task     | Period | Exec Time | Weight | Effective Priority (Weighted EDF) |
|----------|--------|-----------|--------|-----------------------------------|
| HighPrio | 100ms  | 20ms      | 3      | 100/3 ≈ 33 (highest)             |
| MedPrio  | 200ms  | 40ms      | 2      | 200/2 = 100                      |
| LowPrio  | 500ms  | 50ms      | 1      | 500/1 = 500 (lowest)             |

## Comparing Algorithms

1. Run with each scheduler configuration
2. Observe differences in response times
3. Check deadline miss behavior under load
4. Compare priority ordering

## API Highlights

### Simple Configuration
```c
k_thread_rt_config(tid, period_ms, exec_time_ms, weight);
```

This single call replaces:
```c
k_thread_deadline_set(tid, k_ms_to_cyc_ceil32(period_ms));
k_thread_exec_time_set(tid, k_ms_to_cyc_ceil32(exec_time_ms));
k_thread_weight_set(tid, weight);
```

### Millisecond Convenience
```c
k_thread_exec_time_set_ms(tid, exec_time_ms);
```

Automatically converts milliseconds to cycles.

## Further Reading

- See `include/zephyr/kernel/sched_rt.h` for complete API
- See `doc/kernel/services/scheduling/rt_schedulers.rst` for algorithm details
- See `MODULAR_DESIGN.md` for implementation architecture
