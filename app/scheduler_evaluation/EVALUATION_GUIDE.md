# Scheduler Evaluation Suite - Documentation

## Overview

This evaluation suite provides comprehensive performance testing and comparison of 6 real-time scheduling algorithms in Zephyr RTOS:

1. **Base-EDF** (Earliest Deadline First) - Zephyr's default deadline scheduler
2. **Weighted-EDF** - EDF with task weight priorities
3. **RMS** (Rate Monotonic Scheduling) - Period-based priority assignment
4. **WSRT** (Weighted Shortest Remaining Time) - Shortest job first with weights
5. **LLF** (Least Laxity First) - Urgency-based scheduling
6. **PFS** (Proportional Fair Scheduling) - Fair resource allocation

## What the Evaluation Does

### Test Matrix

The evaluation runs **96 total tests** (6 schedulers × 16 scenarios each):

**Thread Count Variations:**
- 3 threads (low concurrency)
- 5 threads (moderate concurrency)
- 8 threads (high concurrency)
- 10 threads (very high concurrency)

**Load Levels:**
- **Light** (40% CPU utilization) - All tasks should meet deadlines
- **Moderate** (70% CPU utilization) - Most tasks meet deadlines
- **Heavy** (90% CPU utilization) - Stress test, some deadline misses expected
- **Overload** (110% CPU utilization) - Intentional overload to test graceful degradation

**Each test combination produces:**
- Total task activations
- Deadline misses
- Miss rate percentage

### Task Characteristics

**Periodic Real-Time Tasks:**
- Each task has a defined period, deadline, and execution time
- Deadlines are set to 80% of the period
- Tasks have weights (priority adjustments for weighted schedulers)
- First 1/3 of tasks marked as "critical" (higher importance)

**Workload Profiles:**

| Load Level | Base Period | Base Exec Time | Period Multiplier | Target Utilization |
|-----------|------------|----------------|-------------------|-------------------|
| Light     | 100ms      | 15ms          | 2x                | ~40%              |
| Moderate  | 80ms       | 25ms          | 2x                | ~70%              |
| Heavy     | 60ms       | 30ms          | 1x                | ~90%              |
| Overload  | 40ms       | 35ms          | 1x                | ~110%             |

**Task Generation:**
- Each thread gets progressively longer periods: `base_period + (i × 20 × multiplier)`
- Execution times increase linearly: `base_exec + (i × 3)`
- Weights assigned inversely: higher-index tasks get lower weights
- ±33% execution time variance to simulate realistic workloads

### Metrics Collected

For each scheduler × scenario combination:

**Performance Metrics:**
1. **Activations** - Number of times tasks were released
2. **Misses** - Number of deadline violations
3. **Miss Rate** - Percentage of activations that missed deadlines

**Latency Metrics:**
4. **Avg Response Time** - Mean time from activation to completion (ms)
5. **Max Response Time** - Worst-case response time observed (ms)
6. **Jitter** - Timing variance (max_response - min_response, in ms)

**Kernel Overhead Metrics:**
7. **Context Switches** - Total number of context switch operations
8. **Preemptions** - Number of times tasks were preempted
9. **CS per Activation** - Average context switches per task activation
10. **Scheduling Overhead %** - Estimated CPU time spent in scheduler
    - Calculated as: (context_switches × 10μs) / test_duration_ms × 100%

## Running the Evaluation

### Prerequisites

- Zephyr environment configured (west, toolchain)
- Python virtual environment at `/home/jack/.venv/zephyr/`
- Native simulator target (`native_sim`)

### Execution

```bash
cd /home/jack/cs736-project/zephyr/app/scheduler_evaluation
./scripts/run_evaluation.sh
```

### What Happens During Execution

1. **Backup Configuration** - Saves original `prj.conf`
2. **For Each Scheduler:**
   - Modifies `prj.conf` to enable the specific scheduler
   - Performs pristine build (`west build -p always`)
   - Runs 2-second simulation for each of 16 scenarios
   - Extracts performance data from output
   - Saves individual log and CSV files
3. **Restore Configuration** - Returns `prj.conf` to original state
4. **Generate Plots** - Creates visualizations from collected data
5. **Create Summary** - Statistical analysis of results

### Duration

- Each test scenario: ~5 seconds simulation + build time
- Total per scheduler: ~16 scenarios × (5s + build)
- **Total evaluation time: ~25-30 minutes** for all 6 schedulers

## Output Structure

Results are organized in timestamped directories:

```
results/run_YYYYMMDD_HHMMSS/
├── all_results.csv          # Merged CSV with all 96 test results
├── Base-EDF.csv            # Individual scheduler CSV
├── Base-EDF.log            # Full build and execution log
├── Weighted-EDF.csv
├── Weighted-EDF.log
├── RMS.csv
├── RMS.log
├── WSRT.csv
├── WSRT.log
├── LLF.csv
├── LLF.log
├── PFS.csv
├── PFS.log
└── plots/
    ├── analysis.txt                # Statistical summary
    ├── miss_rate_by_load.png       # Grouped bar chart by load level
    ├── scheduler_comparison.png    # Overall performance comparison
    ├── scalability.png             # Performance vs. thread count
    ├── kernel_overhead.png         # Overhead and context switch analysis
    └── response_time_analysis.png  # Latency and jitter visualizations
```

## Understanding the Results

### CSV Format

```csv
Scheduler,Threads,Load,Activations,Misses,MissRate,AvgResponseTime,MaxResponseTime,Jitter,ContextSwitches,Preemptions,CSPerActivation,OverheadPercent
Base-EDF,3,Light,43,0,0.00,16,38,15,124,87,2.88,0.12
Base-EDF,3,Moderate,50,0,0.00,22,52,28,145,102,2.90,0.15
Base-EDF,3,Heavy,63,10,15.87,35,89,47,183,128,2.90,0.18
...
```

**Column Descriptions:**
- `Scheduler` - Scheduling algorithm name
- `Threads` - Number of concurrent tasks (3, 5, 8, or 10)
- `Load` - Workload level (Light, Moderate, Heavy, Overload)
- `Activations` - Total task activations in 5-second test
- `Misses` - Deadline violations
- `MissRate` - Percentage of missed deadlines
- `AvgResponseTime` - Average response time in milliseconds
- `MaxResponseTime` - Maximum response time in milliseconds
- `Jitter` - Timing variance (max - min response time)
- `ContextSwitches` - Total context switches during test
- `Preemptions` - Total preemptions during test
- `CSPerActivation` - Context switches per activation
- `OverheadPercent` - Estimated scheduling overhead percentage

### Analysis Metrics

**From `analysis.txt`:**

- **Average Miss Rate** - Mean deadline miss percentage across all 16 scenarios
- **Min/Max Miss Rate** - Best and worst case performance
- **Zero-miss scenarios** - How many of 16 tests had perfect deadline adherence
- **Rating** - Overall assessment:
  - ★★★★★ EXCELLENT (< 5% avg miss rate)
  - ★★★★☆ GOOD (5-10%)
  - ★★★☆☆ ACCEPTABLE (10-20%)
  - ★★☆☆☆ POOR (20-30%)
  - ★☆☆☆☆ CRITICAL (> 30%)

### Example Results (Recent Run)

| Scheduler    | Avg Miss Rate | Zero-Miss | Max Miss | Rating       |
|-------------|---------------|-----------|----------|--------------|
| PFS         | 12.38%        | 4/16      | 37.31%   | ★★★☆☆ ACCEPTABLE |
| WSRT        | 14.92%        | 8/16      | 50.00%   | ★★★☆☆ ACCEPTABLE |
| RMS         | 14.96%        | 8/16      | 50.00%   | ★★★☆☆ ACCEPTABLE |
| Weighted-EDF| 16.15%        | 6/16      | 50.65%   | ★★☆☆☆ POOR |
| LLF         | 23.92%        | 3/16      | 48.39%   | ★★☆☆☆ POOR |
| Base-EDF    | 25.21%        | 4/16      | 49.23%   | ★★☆☆☆ POOR |

## Visualization Plots

### 1. Miss Rate by Load Level (`miss_rate_by_load.png`)

Grouped bar chart showing how each scheduler performs under different load conditions:
- Light load: Should show near-zero miss rates
- Moderate load: Low miss rates for good schedulers
- Heavy load: Differentiation becomes clear
- Overload: Tests graceful degradation

### 2. Scheduler Comparison (`scheduler_comparison.png`)

Overall performance comparison:
- Average miss rate across all scenarios
- Error bars showing variance
- Helps identify best overall performer

### 3. Scalability Analysis (`scalability.png`)

Performance vs. thread count:
- Shows how schedulers handle increasing concurrency
- Good schedulers maintain low miss rates as threads increase
- Reveals scalability limitations

### 4. Kernel Overhead Analysis (`kernel_overhead.png`)

Two-panel visualization:
- **Left**: Average scheduling overhead percentage per scheduler
- **Right**: Context switches per activation
- Reveals efficiency cost of scheduling decisions
- Lower values indicate more efficient schedulers

### 5. Response Time Analysis (`response_time_analysis.png`)

Four-panel latency visualization:
- **Top-left**: Average response time vs. load level
- **Top-right**: Maximum response time vs. load level
- **Bottom-left**: Jitter vs. load level
- **Bottom-right**: Overall average response time by scheduler
- Shows timing predictability and worst-case behavior

## Technical Implementation

### Scheduler Configuration

**Base-EDF:**
```kconfig
CONFIG_736_ADD_ONS=n
CONFIG_SCHED_DEADLINE=y
```

**Custom Schedulers (Weighted-EDF, RMS, WSRT, LLF, PFS):**
```kconfig
CONFIG_736_ADD_ONS=y
CONFIG_736_<SCHEDULER>=y
```

### Build Process

The evaluation script uses:
- **Pristine builds** (`-p always`) to ensure clean configuration switches
- **sed editing** of `prj.conf` to enable/disable schedulers
- **Timeout** (70s) to handle potential hangs
- **Backup/restore** to preserve original configuration

### Task Simulation

```c
// Periodic task structure
struct thread_config {
    uint32_t period_ms;      // How often task activates
    uint32_t deadline_ms;    // Time limit for completion (80% of period)
    uint32_t exec_time_ms;   // Expected execution duration
    uint32_t weight;         // Priority weight (higher = more important)
    bool is_critical;        // Criticality flag
};

// Each iteration:
1. Wait for period
2. Set deadline parameters
3. Execute workload (with ±33% variance)
4. Check if deadline was met
5. Record statistics
```

## Interpretation Guidelines

### What to Look For

**Good Scheduler Characteristics:**
- Low average miss rate (< 15%)
- Many zero-miss scenarios under light/moderate loads
- Graceful degradation under overload
- Consistent performance across thread counts

**Red Flags:**
- High miss rates even under light load
- Poor scalability (performance degrades rapidly with more threads)
- High variance between scenarios
- Inability to prioritize critical tasks

### Load-Specific Expectations

**Light Load (40% utilization):**
- ✅ Expected: 0% miss rate for all schedulers
- ⚠️ Concerning: > 5% miss rate indicates fundamental issues

**Moderate Load (70% utilization):**
- ✅ Expected: < 5% miss rate
- ⚠️ Acceptable: 5-15% miss rate
- ❌ Poor: > 15% miss rate

**Heavy Load (90% utilization):**
- ✅ Good: < 20% miss rate
- ⚠️ Acceptable: 20-40% miss rate
- ❌ Poor: > 40% miss rate

**Overload (110% utilization):**
- ✅ Good: < 45% miss rate (efficient triage)
- ⚠️ Expected: 45-55% miss rate
- ❌ Poor: > 55% miss rate (scheduler overhead causing extra misses)

## Customization

### Modifying Test Parameters

Edit `src/main.c`:

```c
#define NUM_THREAD_CONFIGS 4    // Add/remove thread count tests
#define NUM_LOAD_CONFIGS 4      // Add/remove load levels
#define TEST_DURATION_MS 2000   // Longer tests = more data
```

### Adding Workload Profiles

```c
case LOAD_CUSTOM:
    base_period = 50;     // ms
    base_exec = 20;       // ms
    period_multiplier = 1;
    break;
```

### Testing Additional Schedulers

1. Add to `scripts/run_evaluation.sh`:
```bash
SCHEDULERS=(
    ...
    "My-Scheduler:CONFIG_MY_SCHEDULER"
)
```

2. Add detection in `src/main.c`:
```c
#elif defined(CONFIG_MY_SCHEDULER)
    return "My-Scheduler";
```

## Troubleshooting

### Build Failures
- Check that all scheduler configs are in `prj.conf`
- Verify `CONFIG_736_ADD_ONS` setting matches scheduler type
- Review individual `.log` files in results directory

### Missing Data
- Check if simulation timed out (70s limit)
- Verify CSV extraction pattern matches output format
- Look for errors in scheduler-specific log files

### Inconsistent Results
- Increase `TEST_DURATION_MS` for more samples
- Check for system load affecting native_sim timing
- Run multiple evaluations and compare

## Further Analysis

### Viewing Raw Data
```bash
# Pretty-print CSV
column -t -s, results/run_*/all_results.csv | less

# Filter specific scheduler
grep "RMS," results/run_*/all_results.csv

# Compare two schedulers
awk -F, '$1=="RMS" || $1=="Weighted-EDF"' results/run_*/all_results.csv
```

### Custom Plots
```python
import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv('results/run_*/all_results.csv')
# Your custom analysis here
```

### Statistical Analysis
```python
# In results directory
import pandas as pd
df = pd.read_csv('all_results.csv')

# Group by scheduler
print(df.groupby('Scheduler')['MissRate'].describe())

# Best performer per load level
print(df.groupby('Load')['MissRate'].idxmin())
```

## Conclusion

This evaluation suite provides a standardized, reproducible framework for comparing real-time schedulers under realistic workload conditions. The combination of varied thread counts, load levels, and task characteristics reveals scheduler behavior across the full operational spectrum from light load to overload conditions.

Use the results to:
- Select appropriate scheduler for your workload
- Validate scheduler implementations
- Identify performance bottlenecks
- Compare algorithm trade-offs
- Publish benchmark results
