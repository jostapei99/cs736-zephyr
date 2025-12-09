# Scheduler Evaluation Suite

Comprehensive automated testing framework for evaluating 6 real-time scheduling algorithms in Zephyr RTOS.

## Quick Start

```bash
cd /home/jack/cs736-project/zephyr/app/scheduler_evaluation
./scripts/run_evaluation.sh
```

This will:
1. Test all 6 schedulers (Base-EDF, Weighted-EDF, RMS, WSRT, LLF, PFS)
2. Run 16 scenarios per scheduler (4 thread counts × 4 load levels = 96 total tests)
3. Each scenario runs for **5 seconds** (extended for better accuracy)
4. Create timestamped results directory with CSV data, logs, and plots
5. Generate statistical analysis and visualizations

**Duration**: ~25-30 minutes | **Output**: `results/run_TIMESTAMP/`

📖 **For detailed methodology and interpretation, see [EVALUATION_GUIDE.md](EVALUATION_GUIDE.md)**

## What This Evaluates

**6 Real-Time Schedulers:**

| Scheduler    | Type | Description                                    |
|--------------|------|------------------------------------------------|
| Base-EDF     | EDF  | Zephyr's default Earliest Deadline First       |
| Weighted-EDF | EDF  | EDF with task weight priorities                |
| RMS          | RM   | Rate Monotonic Scheduling (period-based)       |
| WSRT         | SJF  | Weighted Shortest Remaining Time               |
| LLF          | LLF  | Least Laxity First (urgency-based)             |
| PFS          | Fair | Proportional Fair Scheduling                   |

**Test Matrix:** 96 total scenarios
- **Thread Counts**: 3, 5, 8, 10 (concurrency scaling)
- **Load Levels**: Light (40%), Moderate (70%), Heavy (90%), Overload (110%)
- **Test Duration**: 5 seconds per scenario (extended for accuracy)
- **Metrics Collected**:
  - Deadline miss rates
  - Response time (average and maximum)
  - Jitter (timing variance)
  - Context switches and preemptions
  - Kernel scheduling overhead percentage

**Each scenario:**
- 5-second simulation with periodic real-time tasks
- Tasks have varying periods, deadlines, and execution times
- ±33% execution variance to simulate realistic workloads
- Comprehensive latency and overhead measurements

```
scheduler_evaluation/
├── src/                      # Application source code
│   └── main.c               # Evaluation application
├── scripts/                  # All automation scripts
│   ├── run_evaluation.sh    # Main evaluation script
│   └── plot_results.py      # Plotting script
├── results/                  # All test results
│   └── run_YYYYMMDD_HHMMSS/ # Each run gets its own directory
│       ├── results.csv      # CSV data
│       ├── summary.txt      # Execution log
│       ├── logs/            # Per-scheduler logs
│       │   ├── Base-EDF.log
│       │   ├── Weighted-EDF.log
│       │   └── ...
│       └── plots/           # Generated visualizations
│           ├── miss_rate_by_load.png
│           ├── scheduler_comparison.png
│           ├── scalability.png
│           └── analysis.txt
├── CMakeLists.txt           # Build configuration
└── prj.conf                 # Zephyr configuration
```

## Results & Analysis

### Quick View

```bash
# View statistical summary
cat results/run_*/plots/analysis.txt

# Pretty-print CSV
column -t -s, results/run_*/all_results.csv | less

# Compare schedulers
grep -E "^(RMS|WSRT)," results/run_*/all_results.csv
```

### CSV Format

```csv
Scheduler,Threads,Load,Activations,Misses,MissRate,AvgResponseTime,MaxResponseTime,Jitter,ContextSwitches,Preemptions,CSPerActivation,OverheadPercent
RMS,5,Moderate,65,0,0.00,18,45,12,187,131,2.88,0.19
```

**Columns:**
- `Scheduler` - Algorithm tested
- `Threads` - Concurrent tasks (3/5/8/10)
- `Load` - Workload level (Light/Moderate/Heavy/Overload)
- `Activations` - Task releases during 5-second test
- `Misses` - Deadline violations
- `MissRate` - Percentage that missed deadlines
- `AvgResponseTime` - Average response time (ms)
- `MaxResponseTime` - Maximum response time (ms)
- `Jitter` - Timing variance (max - min response time)
- `ContextSwitches` - Total context switches
- `Preemptions` - Total preemptions
- `CSPerActivation` - Context switches per activation
- `OverheadPercent` - Estimated scheduling overhead %

### Performance Ratings (from analysis.txt)

- ★★★★★ EXCELLENT (< 5% avg miss rate)
- ★★★★☆ GOOD (5-10%)
- ★★★☆☆ ACCEPTABLE (10-20%)
- ★★☆☆☆ POOR (20-30%)
- ★☆☆☆☆ CRITICAL (> 30%)

**Recent Run Example:**
| Scheduler    | Avg Miss | Zero-Miss Scenarios | Rating    |
|--------------|----------|---------------------|-----------|
| PFS          | 12.38%   | 4/16                | ★★★☆☆ |
| WSRT         | 14.92%   | 8/16                | ★★★☆☆ |
| RMS          | 14.96%   | 8/16                | ★★★☆☆ |
| Weighted-EDF | 16.15%   | 6/16                | ★★☆☆☆ |
| LLF          | 23.92%   | 3/16                | ★★☆☆☆ |
| Base-EDF     | 25.21%   | 4/16                | ★★☆☆☆ |

## How It Works

### Execution Flow

1. **Backup Configuration** - Saves original `prj.conf`
2. **For Each Scheduler:**
   - Uses `sed` to edit `prj.conf` and enable specific scheduler
   - Performs pristine build (`west build -p always`)
   - Runs 16 test scenarios (4 thread counts × 4 loads)
   - Each scenario simulates for 5 seconds
   - Extracts performance data from output
   - Saves individual log and CSV files
3. **Restore Configuration** - Returns `prj.conf` to original state
4. **Generate Visualizations** - Creates plots and statistical analysis
5. **Summary** - Organizes all data in timestamped directory

### Task Simulation

Each test scenario creates periodic real-time tasks:

```c
// Example task configuration
Period:    100ms (how often task activates)
Deadline:  80ms  (must complete within this time, 80% of period)
Exec Time: 20ms  (expected work duration, ±33% variance)
Weight:    5     (priority for weighted schedulers)
```

Tasks execute in a loop:
1. Wait for period
2. Set deadline parameters
3. Execute workload with realistic variance
4. Measure response time and jitter
5. Check if deadline was met
6. Record statistics (activations, misses, latency, context switches)

### Workload Profiles

| Load      | Base Period | Base Exec | Target CPU | Expectations            |
|-----------|-------------|-----------|------------|-------------------------|
| Light     | 100ms       | 15ms      | ~40%       | Zero misses expected    |
| Moderate  | 80ms        | 25ms      | ~70%       | Low miss rates          |
| Heavy     | 60ms        | 30ms      | ~90%       | Stress test             |
| Overload  | 40ms        | 35ms      | ~110%      | Tests graceful degradation |

## Metrics Collected

### Deadline Performance
- **Miss Rate** - Percentage of task activations that missed deadlines
- **Total Misses** - Absolute count of deadline violations
- **Zero-Miss Scenarios** - Test cases with perfect deadline adherence

### Latency Metrics
- **Average Response Time** - Mean time from task activation to completion
- **Maximum Response Time** - Worst-case response time observed
- **Jitter** - Timing variance (max - min response time)
  - Low jitter = predictable performance
  - High jitter = timing uncertainty

### Kernel Overhead
- **Context Switches** - Total scheduler context switch operations
- **Preemptions** - Number of times tasks were preempted
- **CS per Activation** - Average context switches per task activation
- **Scheduling Overhead %** - Estimated CPU time spent in scheduler
  - Calculated as: (context_switches × 10μs) / test_duration

### What These Reveal
- **Response Time**: How quickly tasks complete after activation
- **Jitter**: Predictability and consistency of timing
- **Overhead**: Real cost of scheduling decisions
- **Miss Rate**: Hard deadline guarantee capability

## Requirements & Installation

**Prerequisites:**
- Zephyr RTOS environment configured (west, toolchain)
- Python 3.7+ virtual environment at `/home/jack/.venv/zephyr/`
- Native simulator target (`native_sim`)

**Required Zephyr Configurations:**

The `prj.conf` file must include the following to enable kernel overhead tracking:

```conf
# RT Statistics Infrastructure (required for overhead metrics)
CONFIG_736_ADD_ONS=y              # Enables RT stats subsystem
CONFIG_736_RT_STATS=y             # Core RT statistics
CONFIG_736_RT_STATS_DETAILED=y    # Detailed metrics
CONFIG_736_RT_STATS_SQUARED=y     # Variance calculations

# Context Switch Tracking (required for CS/preemption counts)
CONFIG_THREAD_RUNTIME_STATS=y     # Parent config for thread runtime stats
CONFIG_SCHED_THREAD_USAGE=y       # Enables context switch instrumentation
```

**Why These Configs Matter:**
- Without `CONFIG_736_ADD_ONS=y`: RT stats will be disabled (even if you set `CONFIG_736_RT_STATS=y`)
- Without `CONFIG_THREAD_RUNTIME_STATS=y`: Context switch tracking is unavailable
- Missing these → All overhead metrics will show 0.00

See `DEBUG_KERNEL_OVERHEAD_FIX.md` for the full debugging story if overhead metrics aren't working.

**Optional (for plots):**
```bash
source /home/jack/.venv/zephyr/bin/activate
pip install matplotlib seaborn numpy pandas
```

## Interpretation Guidelines

### What Good Schedulers Show

✅ Low miss rate under light/moderate loads (< 5%)  
✅ Many zero-miss scenarios (> 50% of tests)  
✅ Graceful degradation under overload  
✅ Consistent performance across thread counts  

### Load-Specific Expectations

**Light Load (40% CPU):**
- ✅ Expected: 0% miss rate
- ⚠️ Concerning: > 5% indicates fundamental issues

**Moderate Load (70% CPU):**
- ✅ Good: < 5% miss rate
- ⚠️ Acceptable: 5-15%
- ❌ Poor: > 15%

**Heavy Load (90% CPU):**
- ✅ Good: < 20% miss rate
- ⚠️ Acceptable: 20-40%
- ❌ Poor: > 40%

**Overload (110% CPU):**
- ✅ Good: < 45% (efficient triage)
- ⚠️ Expected: 45-55%
- ❌ Poor: > 55% (scheduler overhead causing extra misses)

## Troubleshooting

### Build Failures
Check individual scheduler logs in `results/run_*/` directory:
```bash
cat results/run_*/Base-EDF.log
```

Common issues:
- Missing `CONFIG_736_ADD_ONS` in `prj.conf`
- Scheduler config flag not defined
- Conflicting scheduler options

### All Schedulers Show Same Name
The configuration switching isn't working. Verify:
- `prj.conf` has `CONFIG_736_ADD_ONS=n` by default
- Script is using `sed -i` to edit `prj.conf`
- Check "Detected:" lines in log files show different scheduler names

### No Plots Generated
Install plotting dependencies:
```bash
source /home/jack/.venv/zephyr/bin/activate
pip install matplotlib seaborn numpy pandas
```

### Inconsistent Results
- Increase `TEST_DURATION_MS` in `src/main.c` for more samples
- Run multiple evaluations and compare
- Check system load (native_sim timing affected by host CPU)

## Customization

### Modify Test Parameters

Edit `src/main.c`:
```c
#define TEST_DURATION_MS 2000    // Length of each test
#define NUM_THREAD_CONFIGS 4     // Thread count variations
#define NUM_LOAD_CONFIGS 4       // Load level variations
```

### Add Custom Scheduler

1. Add to `scripts/run_evaluation.sh`:
```bash
SCHEDULERS=(
    ...
    "My-Scheduler:CONFIG_MY_SCHED"
)
```

2. Add detection in `src/main.c`:
```c
#elif defined(CONFIG_MY_SCHED)
    return "My-Scheduler";
```

### Adjust Workload Profiles

Edit workload generation in `src/main.c`:
```c
case LOAD_CUSTOM:
    base_period = 50;        // ms between activations
    base_exec = 20;          // ms of work per activation
    period_multiplier = 1;
    break;
```

## Advanced Analysis

### Python Data Analysis

```python
import pandas as pd
import matplotlib.pyplot as plt

# Load results
df = pd.read_csv('results/run_20251208_192955/all_results.csv')

# Best scheduler per load level
print(df.groupby('Load')['MissRate'].agg(['min', 'max', 'mean']))

# Scheduler comparison
print(df.groupby('Scheduler')['MissRate'].mean().sort_values())

# Custom plots
df[df['Load'] == 'Heavy'].plot(x='Threads', y='MissRate', 
                                 kind='bar', by='Scheduler')
plt.show()
```

### Terminal Analysis

```bash
# Latest run
ls -lt results/ | head -2

# Filter specific scheduler
grep "RMS," results/run_*/all_results.csv

# Compare two schedulers
awk -F, '$1=="RMS" || $1=="Weighted-EDF"' results/run_*/all_results.csv

# Count zero-miss scenarios
awk -F, '$6==0.00 {count++} END {print count}' results/run_*/all_results.csv

# Average miss rate by load
awk -F, 'NR>1 {sum[$3]+=$6; count[$3]++} 
         END {for (load in sum) print load, sum[load]/count[load]}' \
         results/run_*/all_results.csv
```

## Key Insights

**What This Reveals:**
- **Light Load**: Tests basic scheduler correctness
- **Moderate Load**: Shows real-world performance
- **Heavy Load**: Reveals efficiency under stress
- **Overload**: Tests triage capability when resources are insufficient

**Scheduler Trade-offs:**
- **EDF variants**: Optimal when all deadlines can be met
- **RMS**: Predictable, works well with harmonic periods
- **WSRT**: Good for mixed-criticality workloads
- **LLF**: Dynamic but higher overhead
- **PFS**: Fair allocation but may sacrifice deadline adherence

## Documentation

📖 **[EVALUATION_GUIDE.md](EVALUATION_GUIDE.md)** - Comprehensive documentation covering:
- Detailed test methodology
- Task characteristics and workload profiles
- Metric interpretation guidelines  
- Visualization explanations
- Advanced customization options
- Statistical analysis techniques
- Performance rating system
- Troubleshooting guide

## Files Overview

| File                     | Purpose                                    |
|--------------------------|---------------------------------------------|
| `src/main.c`             | 595-line evaluation application            |
| `scripts/run_evaluation.sh` | Automation with pristine builds         |
| `scripts/plot_results.py`   | Matplotlib visualization generator      |
| `prj.conf`               | Zephyr kernel configuration                |
| `CMakeLists.txt`         | Build system configuration                 |
| `README.md`              | Quick reference (this file)                |
| `EVALUATION_GUIDE.md`    | Detailed documentation                     |
