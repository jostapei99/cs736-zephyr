# Scheduler Evaluation Workloads for Zephyr RTOS

This directory contains 4 comprehensive workloads designed to evaluate and compare different scheduling algorithms in Zephyr RTOS on embedded systems.

## Overview

These workloads are specifically designed to stress-test schedulers and measure key real-time performance metrics:
- **Latency**: Response time from event to execution
- **Tardiness**: Deadline misses and how late tasks complete
- **Throughput**: Total work completed
- **Schedulability**: Can the system meet all deadlines?

## Workload Descriptions

### Workload 1: Periodic Control System
**Application Domain**: Industrial control, robotics, motor control, flight control

**Characteristics**:
- Fixed periodic tasks with rate-monotonic priority assignment
- Predictable execution times
- Hard real-time requirements
- Tests: Basic scheduler functionality, priority enforcement

**Task Set**:
- Sensor: 10ms period, 2ms execution (Priority 1)
- Control: 20ms period, 5ms execution (Priority 3)  
- Actuator: 50ms period, 3ms execution (Priority 5)
- Logger: Best-effort background task (Priority 7)

**Best Scheduler**: Simple or Multi-Queue (low overhead for periodic tasks)

---

### Workload 2: Event-Driven Communication System
**Application Domain**: Network processing, communication stacks, protocol handlers

**Characteristics**:
- Sporadic event arrivals (Poisson-like)
- Potential priority inversion via shared resources
- Hard real-time constraints on critical paths
- Tests: Interrupt latency, priority inheritance, mutex handling

**Task Set**:
- IRQ Handler: Sporadic, <1ms deadline (Priority 0)
- Packet Processor: Sporadic, 5ms deadline (Priority 2)
- Protocol Handler: Event-driven, variable execution (Priority 4)
- Bulk Transmission: Continuous, no deadline (Priority 6)

**Best Scheduler**: Any with priority inheritance enabled

---

### Workload 3: Mixed Criticality System
**Application Domain**: Avionics (DO-178C), medical devices (IEC 62304), automotive (ISO 26262)

**Characteristics**:
- Multiple criticality levels (safety-critical to best-effort)
- Mode changes and task shedding under overload
- Overload conditions to test graceful degradation
- Tests: Safety guarantees, overload handling, mode switching

**Task Set**:
- Safety Monitor: 10ms period, MUST NEVER miss deadline (Priority 0)
- Mission Function: 20ms period, occasional miss OK (Priority 2)
- User Interface: 100ms period, soft real-time (Priority 4)
- Diagnostics: Aperiodic, best-effort (Priority 6)

**Best Scheduler**: Any, but tests fault tolerance

---

### Workload 4: Multi-Rate Sporadic with Deadline Scheduling
**Application Domain**: Automotive ECU, IoT gateways, multimedia systems

**Characteristics**:
- Multiple time scales (1ms to 100ms)
- Sporadic arrivals with varying deadlines
- Tests EDF (Earliest Deadline First) scheduling
- Tests: Deadline scheduling, aperiodic servers, optimal scheduling

**Task Set**:
- Fast Events: 1-10ms arrival, 5ms deadline
- Medium Events: 10-50ms arrival, 15ms deadline
- Slow Periodic: 100ms period, 100ms deadline
- Deadline Task: 20-60ms arrival, 10ms deadline

**Best Scheduler**: DEADLINE (EDF) - can achieve higher utilization

---

## Building and Running

### Prerequisites
```bash
# Ensure Zephyr environment is set up
cd /path/to/zephyr
source zephyr-env.sh
```

### Build Any Workload
```bash
# General pattern
west build -b qemu_cortex_m3 app/scheduler_evaluation/workload<N>_<name>

# Example: Build workload 1
west build -b qemu_cortex_m3 app/scheduler_evaluation/workload1_periodic_control

# Run in QEMU
west build -t run
```

### Clean Build
```bash
west build -t pristine
```

## Testing Different Schedulers

Zephyr supports multiple scheduler implementations. Edit `prj.conf` in each workload:

### 1. Simple Scheduler (Default)
```kconfig
CONFIG_SCHED_SIMPLE=y
# CONFIG_SCHED_SCALABLE is not set
# CONFIG_SCHED_MULTIQ is not set
# CONFIG_SCHED_DEADLINE is not set
```
- **Best for**: Small number of threads (<10)
- **Advantages**: Lowest code size (~2KB savings), very fast for few threads
- **Disadvantages**: O(N) scaling with thread count

### 2. Scalable Scheduler (Red-Black Tree)
```kconfig
# CONFIG_SCHED_SIMPLE is not set
CONFIG_SCHED_SCALABLE=y
# CONFIG_SCHED_MULTIQ is not set
```
- **Best for**: Many threads (>20)
- **Advantages**: O(log N) scaling, efficient with many threads
- **Disadvantages**: ~2KB code overhead, higher constant-time overhead

### 3. Multi-Queue Scheduler
```kconfig
# CONFIG_SCHED_SIMPLE is not set
# CONFIG_SCHED_SCALABLE is not set
CONFIG_SCHED_MULTIQ=y
```
- **Best for**: Traditional RTOS behavior
- **Advantages**: O(1) operations, very predictable
- **Disadvantages**: Higher RAM usage, incompatible with deadline scheduling

### 4. Deadline Scheduler (EDF)
```kconfig
CONFIG_SCHED_DEADLINE=y
CONFIG_SCHED_SIMPLE=y
```
- **Best for**: Tasks with explicit deadlines
- **Advantages**: Optimal for single-core, can achieve 100% utilization
- **Disadvantages**: Must use with simple scheduler
- **Note**: **Workload 4 specifically tests this!**

## Comprehensive Testing Procedure

### Step 1: Baseline Testing
Run each workload with the simple scheduler to establish baseline metrics.

```bash
# For each workload
cd /path/to/zephyr
west build -b qemu_cortex_m3 app/scheduler_evaluation/workload1_periodic_control
west build -t run > results_w1_simple.txt

west build -b qemu_cortex_m3 app/scheduler_evaluation/workload2_event_driven
west build -t run > results_w2_simple.txt

# ... repeat for all workloads
```

### Step 2: Scheduler Comparison
For each workload, test all compatible schedulers:

```bash
# Edit prj.conf to change scheduler
# Rebuild and run
west build -t pristine
west build -b qemu_cortex_m3 app/scheduler_evaluation/workload1_periodic_control
west build -t run > results_w1_scalable.txt
```

### Step 3: Analyze Results
Compare metrics across schedulers:
- Deadline miss rate
- Average/max latency
- Average/max response time
- Tardiness
- Throughput

### Step 4: Stress Testing
Modify workload parameters to create overload:
- Reduce periods (increase arrival rate)
- Increase execution times
- Add more threads

## Key Metrics to Compare

| Metric | Description | Goal |
|--------|-------------|------|
| **Deadline Miss Rate** | % of jobs missing deadline | 0% for hard RT |
| **Average Latency** | Time from release to start | Minimize |
| **Max Latency** | Worst-case latency | Bound and minimize |
| **Average Response Time** | Release to completion | Minimize |
| **Max Response Time** | Worst-case response | Must be < deadline |
| **Tardiness** | How late missed deadlines are | Minimize |
| **Throughput** | Jobs completed per second | Maximize |
| **Context Switches** | Scheduler overhead | Minimize |

## Expected Performance

### Workload 1 (Periodic Control)
- **Simple/MultiQ**: Should have 0 deadline misses (U=51%, schedulable)
- **Scalable**: Slight overhead but still schedulable
- **Deadline**: Not needed (fixed priorities work well)

### Workload 2 (Event-Driven)
- **All schedulers**: Should handle sporadic arrivals
- **Priority inheritance**: Critical for preventing priority inversion
- **Interrupt latency**: Should be <100μs

### Workload 3 (Mixed Criticality)
- **All schedulers**: Safety task must NEVER miss deadline
- **Overload handling**: System should gracefully degrade
- **Mode changes**: Lower priority tasks should be shed

### Workload 4 (Deadline Sporadic)
- **Deadline scheduler**: Best performance, lowest miss rate
- **Priority-based**: Higher miss rate, potential starvation
- **EDF advantage**: Can handle higher utilization

## Scheduler Selection Guidelines

```
Choose SIMPLE when:
- Few threads (<10)
- Code size critical
- Simple priority-based scheduling sufficient

Choose SCALABLE when:
- Many threads (>20)
- Complex threading patterns
- Need good scaling

Choose MULTIQ when:
- Traditional RTOS behavior desired
- Very predictable performance needed
- Compatible with existing priority schemes

Choose DEADLINE when:
- Tasks have explicit deadlines
- Need optimal scheduling
- Higher utilization required
- Sporadic/aperiodic workloads
```

## Hardware Platforms

While designed for `qemu_cortex_m3`, these workloads can run on any Zephyr-supported board:

```bash
# Example: Run on real hardware
west build -b <your_board> app/scheduler_evaluation/workload1_periodic_control
west flash
```

Popular boards:
- `qemu_cortex_m3` - QEMU emulation (default)
- `nucleo_f429zi` - STM32 development board
- `frdm_k64f` - NXP Kinetis board
- `nrf52840dk_nrf52840` - Nordic nRF52840 DK

## Customization

Each workload can be customized by editing the source code:

### Adjust Task Parameters
```c
// In main.c, modify these defines:
#define SENSOR_PERIOD_MS 10      // Change period
#define SENSOR_EXEC_US 2000      // Change execution time
#define SENSOR_PRIORITY 1        // Change priority
```

### Adjust Test Duration
```c
#define TEST_DURATION_MS 10000   // Run for 10 seconds
```

### Add More Tasks
Copy an existing thread function and adjust parameters.

## Troubleshooting

### Build Errors
```bash
# Clean build
west build -t pristine
west build -b qemu_cortex_m3 samples/scheduler_evaluation/workload1_periodic_control
```

### QEMU Doesn't Exit
Press `Ctrl+A` then `X` to exit QEMU.

### Timing Calibration Fails
Increase calibration time:
```c
k_busy_wait(1000000);  // Wait longer for calibration
```

### Deadline Scheduling Not Working
Ensure configuration:
```kconfig
CONFIG_SCHED_DEADLINE=y
CONFIG_SCHED_SIMPLE=y
```

## Citation and References

These workloads are based on real-time scheduling theory:

- **Rate Monotonic Analysis (RMA)**: Liu & Layland, 1973
- **Earliest Deadline First (EDF)**: Liu & Layland, 1973  
- **Mixed Criticality Systems**: Vestal, 2007
- **Priority Inheritance**: Sha, Rajkumar, Lehoczky, 1990

## Contributing

To add new workloads:
1. Create new directory: `workload<N>_<name>/`
2. Add `src/main.c`, `prj.conf`, `CMakeLists.txt`, `README.md`
3. Follow existing patterns for metrics collection
4. Document the workload characteristics

## License

SPDX-License-Identifier: Apache-2.0
