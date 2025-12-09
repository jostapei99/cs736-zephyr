# RT Scheduler Workload Evaluation Suite

This directory contains a comprehensive collection of workload types designed to evaluate the performance characteristics of the 6 real-time scheduling algorithms implemented in this Zephyr branch.

## Scheduling Algorithms Evaluated

1. **EDF** (Earliest Deadline First) - Baseline deadline-based scheduling
2. **Weighted EDF** - Deadline scheduling with task importance
3. **WSRT** (Weighted Shortest Remaining Time) - Response time optimization
4. **RMS** (Rate Monotonic Scheduling) - Static priority based on period
5. **LLF** (Least Laxity First) - Dynamic priority based on slack time
6. **PFS** (Proportional Fair Scheduling) - Fairness-oriented scheduling

## Workload Categories

### 1. Periodic Workloads (`periodic/`)
Standard periodic task sets with varying characteristics:
- **light_load/** - 40-50% CPU utilization
- **medium_load/** - 60-70% CPU utilization
- **heavy_load/** - 80-90% CPU utilization
- **harmonic/** - Harmonic period relationships (100ms, 200ms, 400ms)
- **non_harmonic/** - Non-harmonic periods (100ms, 150ms, 230ms)

### 2. Sporadic/Bursty Workloads (`sporadic/`)
Aperiodic and bursty task patterns:
- **aperiodic/** - Random arrival times with bounded rates
- **bursty/** - Clustered activations with quiet periods
- **mixed_periodic_sporadic/** - Combination of periodic and sporadic tasks

### 3. Mixed-Criticality Workloads (`mixed_criticality/`)
Tasks with different importance levels:
- **high_low/** - High-importance + low-importance tasks
- **three_tier/** - Critical, important, and best-effort tasks
- **degraded_mode/** - Workload that transitions between modes

### 4. Overload Scenarios (`overload/`)
Stress testing under overload conditions:
- **gradual_overload/** - Slowly increasing utilization
- **sudden_overload/** - Abrupt overload events
- **sustained_overload/** - Continuous >100% utilization
- **transient_overload/** - Temporary overload spikes

### 5. Response Time Focused (`response_time/`)
Workloads designed to test response time characteristics:
- **short_deadline/** - Tight deadlines relative to periods
- **long_deadline/** - Loose deadlines (deadline > period)
- **varied_execution/** - High execution time variance

### 6. Fairness Testing (`fairness/`)
Workloads to evaluate fair CPU allocation:
- **equal_weight/** - All tasks with equal weight
- **varied_weight/** - Wide range of weight values
- **starvation_test/** - Potential starvation scenarios

## Directory Structure

```
workloads/
├── README.md                      # This file
├── common/
│   ├── workload_common.h         # Common definitions and utilities
│   ├── task_generator.h          # Task creation helpers
│   ├── statistics_collector.h    # Statistics gathering
│   └── csv_output.h              # CSV formatting for results
├── periodic/
│   ├── light_load/
│   │   ├── CMakeLists.txt
│   │   ├── prj.conf
│   │   ├── README.md
│   │   └── src/main.c
│   ├── medium_load/
│   ├── heavy_load/
│   ├── harmonic/
│   └── non_harmonic/
├── sporadic/
│   ├── aperiodic/
│   ├── bursty/
│   └── mixed_periodic_sporadic/
├── mixed_criticality/
│   ├── high_low/
│   ├── three_tier/
│   └── degraded_mode/
├── overload/
│   ├── gradual_overload/
│   ├── sudden_overload/
│   ├── sustained_overload/
│   └── transient_overload/
├── response_time/
│   ├── short_deadline/
│   ├── long_deadline/
│   └── varied_execution/
├── fairness/
│   ├── equal_weight/
│   ├── varied_weight/
│   └── starvation_test/
└── scripts/
    ├── run_all_workloads.sh      # Run all workloads with all schedulers
    ├── compare_schedulers.py     # Generate comparison reports
    └── plot_results.py           # Visualization scripts

```

## Evaluation Metrics

Each workload collects the following metrics using the kernel statistics infrastructure:

1. **Deadline Miss Rate** - Percentage of missed deadlines
2. **Average Response Time** - Mean time from activation to completion
3. **Response Time Jitter** - Standard deviation of response times
4. **Context Switches** - Total number of preemptions
5. **CPU Utilization** - Actual CPU usage percentage
6. **Fairness Metric** - Jain's fairness index for weighted fairness
7. **Preemption Count** - Number of times tasks were preempted
8. **Min/Max Response Time** - Best and worst case response times

## Quick Start

### Running a Single Workload

```bash
# Build and run light load workload with EDF
cd app/workloads/periodic/light_load
west build -b native_sim
west build -t run

# Run with different scheduler - edit prj.conf first:
# CONFIG_736_MOD_EDF=y  # for Weighted EDF
# CONFIG_736_WSRT=y     # for WSRT
# etc.
```

### Running All Workloads

```bash
cd app/workloads/scripts
./run_all_workloads.sh
```

This will:
1. Build each workload with each scheduler
2. Run for specified duration
3. Collect statistics in CSV format
4. Generate comparison reports

### Comparing Results

```bash
cd app/workloads/scripts
python3 compare_schedulers.py --workload periodic/light_load
python3 plot_results.py --workload periodic/light_load --metric deadline_misses
```

## Workload Configuration Format

Each workload defines tasks using a common structure:

```c
typedef struct {
    const char *name;           // Task name for logging
    uint32_t period_ms;         // Period in milliseconds
    uint32_t exec_time_ms;      // Expected execution time
    uint32_t deadline_ms;       // Relative deadline (0 = implicit deadline = period)
    uint32_t weight;            // Task weight/importance (1-10)
    bool is_sporadic;           // true for aperiodic tasks
    uint32_t min_interarrival;  // For sporadic tasks
} workload_task_config_t;
```

## Adding a New Workload

1. Create directory under appropriate category
2. Copy template from `common/template/`
3. Define task configuration in `src/main.c`
4. Update `CMakeLists.txt` and `prj.conf`
5. Document expected behavior in workload's `README.md`

## Expected Results by Scheduler

### Light Load (50% utilization)
- **EDF**: 0% deadline misses, optimal
- **Weighted EDF**: 0% deadline misses, prioritizes high-weight tasks
- **WSRT**: Low response times, potential fairness issues
- **RMS**: 0% deadline misses if schedulable
- **LLF**: 0% deadline misses, more context switches
- **PFS**: Fair CPU allocation, may miss some deadlines

### Heavy Load (90% utilization)
- **EDF**: Possible deadline misses near limit
- **Weighted EDF**: Protects high-weight tasks
- **WSRT**: Short tasks complete quickly
- **RMS**: Higher miss rate than EDF
- **LLF**: Early detection of misses, possible thrashing
- **PFS**: Fair degradation across all tasks

### Overload (>100% utilization)
- **EDF**: Unpredictable which tasks miss
- **Weighted EDF**: High-weight tasks protected
- **WSRT**: Starves long-running tasks
- **RMS**: Static priority determines victims
- **LLF**: May exhibit thrashing behavior
- **PFS**: Degradation proportional to weights

## Research Questions Addressed

1. **What is the utilization bound for each algorithm?**
   - Test with periodic workloads at increasing utilization levels

2. **How do algorithms handle overload?**
   - Use overload scenarios to observe degradation patterns

3. **Which algorithm minimizes response time?**
   - Compare response_time workloads across schedulers

4. **How effective is weight-based differentiation?**
   - Analyze mixed_criticality workloads

5. **What is the context switch overhead?**
   - Count preemptions in all workload types

6. **How do algorithms handle aperiodic arrivals?**
   - Test with sporadic workloads

## Output Format

All workloads produce CSV output for easy analysis:

```csv
timestamp_ms,task_id,activation_count,response_time_ms,deadline_missed,preempted
1000,1,1,25,0,0
1100,2,1,48,0,1
1200,1,2,27,0,0
```

Summary statistics printed at end:

```
=== Workload: Light Load ===
Scheduler: Weighted EDF
Duration: 10000 ms
Total Tasks: 4

Task Statistics:
  Task 1: 100 activations, 0 misses (0.00%), avg response: 25.3ms
  Task 2: 50 activations, 0 misses (0.00%), avg response: 47.8ms
  Task 3: 33 activations, 0 misses (0.00%), avg response: 38.2ms
  Task 4: 20 activations, 0 misses (0.00%), avg response: 49.1ms

Overall:
  Total Activations: 203
  Deadline Misses: 0 (0.00%)
  Context Switches: 412
  Avg Response Time: 35.2ms
  Response Time Jitter: 12.4ms
```

## Building for Different Platforms

While most testing uses `native_sim` for reproducibility, workloads can be built for real hardware:

```bash
# For ARM Cortex-M board
west build -b nucleo_f411re

# For ESP32
west build -b esp32_devkitc_wroom

# For QEMU ARM
west build -b qemu_cortex_m3
```

## Troubleshooting

### High Deadline Miss Rate on Native Sim
- `native_sim` timing may not be perfectly accurate
- Use `CONFIG_TICKLESS_KERNEL=y` for better timing
- Consider testing on real hardware for critical measurements

### Workload Won't Build
- Check that `CONFIG_SCHED_DEADLINE=y` is set
- For WSRT/LLF, ensure runtime tracking is enabled
- Verify CMakeLists.txt includes common headers

### Statistics Not Collected
- Enable `CONFIG_736_RT_STATS=y`
- For detailed stats, add `CONFIG_736_RT_STATS_DETAILED=y`
- Ensure sufficient stack size for statistics structures

## References

- `RT_SCHEDULER_README.md` - Overview of scheduling algorithms
- `ALGORITHM_COMPARISON.md` - Detailed algorithm comparison
- `SCHEDULER_USER_GUIDE.md` - Complete user guide
- `RT_STATISTICS_GUIDE.md` - Statistics collection guide

## Contributing

When adding new workloads:
1. Follow the directory structure convention
2. Document expected behavior in workload README
3. Use common infrastructure from `common/`
4. Output CSV format for consistency
5. Test with all 6 schedulers

## License

SPDX-License-Identifier: Apache-2.0
