# Workload 2: Event-Driven Communication System

## Overview
This workload simulates an event-driven communication/network processing system with sporadic task arrivals, interrupt handling, and potential priority inversion scenarios. Typical of network routers, communication gateways, or protocol stacks.

## Task Set

| Thread | Priority | Arrival Pattern | Execution Time | Deadline |
|--------|----------|----------------|----------------|----------|
| IRQ Handler | 0 (Critical) | Sporadic (15ms mean) | 500μs | 1ms |
| Packet Processor | 2 (High) | Sporadic (8ms mean) | 3ms | 5ms |
| Protocol Handler | 4 (Medium) | Event-driven | 2-8ms (variable) | Soft |
| Bulk TX | 6 (Low) | Continuous | 5ms | None |

## Key Characteristics

1. **Sporadic Arrivals**: Events arrive with random intervals (Poisson-like distribution)
2. **Priority Inversion**: Low priority bulk transmission can block high priority tasks via shared mutex
3. **Variable Execution**: Protocol handler has unpredictable execution time
4. **Hard Real-Time**: IRQ handler has strict 1ms deadline
5. **Resource Contention**: Multiple threads compete for shared resources

## Metrics Measured

1. **Interrupt Latency**: Time from event occurrence to handler execution
2. **Response Time Distribution**: Min, max, and average response times
3. **Deadline Adherence**: Percentage of deadline misses for critical tasks
4. **Priority Inversion Impact**: Effect of low-priority task blocking high-priority tasks
5. **Throughput**: Events processed per second under load

## Building and Running

```bash
# Build for qemu_cortex_m3
west build -b qemu_cortex_m3 app/scheduler_evaluation/workload2_event_driven

# Run in QEMU
west build -t run
```

## Testing Priority Inheritance

Edit `prj.conf` to enable/disable priority inheritance:

### With Priority Inheritance (Default)
```
CONFIG_PRIORITY_INHERITANCE=y
```

### Without Priority Inheritance
```
CONFIG_PRIORITY_INHERITANCE=n
```

Run both configurations and compare the impact on deadline misses and latency.

## Expected Results

### With Priority Inheritance
- Minimal deadline misses on critical tasks
- Lower maximum latency for high-priority threads
- Better worst-case behavior

### Without Priority Inheritance
- Increased deadline misses when bulk TX holds mutex
- Higher latency variance
- Potential unbounded priority inversion

## Interpretation

- **Low IRQ latency** (<100μs) indicates efficient interrupt handling
- **Zero/low deadline misses** for IRQ handler confirms hard real-time capability
- **Latency spikes** indicate priority inversion or scheduler overhead
- **High throughput** with low deadline misses shows effective priority scheduling

## Real-World Applications

This workload models:
- Network packet processing pipelines
- Communication protocol stacks
- Real-time data acquisition systems
- Interrupt-driven I/O subsystems
