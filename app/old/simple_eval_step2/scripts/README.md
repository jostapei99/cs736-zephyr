# Simple Eval Step 2 - Scripts Documentation

This directory contains automation scripts for running comprehensive RT scheduler evaluations with **dynamic weighting** enabled.

## Overview

Tests 4 RT schedulers across 4 workload profiles with **dynamic weighting ON and OFF**, resulting in 32 total test combinations (4 schedulers × 4 workloads × 2 DW options).

### Key Features:
- **Dynamic Weighting Analysis**: Compare performance with adaptive weight adjustment ON vs OFF
- **Weight Evolution Tracking**: Visualize how task weights change over time
- **Comprehensive Metrics**: Deadline misses, response times, weight adjustments
- **Automated Testing**: Full test suite automation with configuration management

## Scripts

### 1. `check_deps.py`
**Purpose**: Verify Python dependencies are installed.

**Usage**:
```bash
python3 scripts/check_deps.py
```

**Dependencies checked**:
- pandas
- matplotlib
- seaborn
- numpy

---

### 2. `quick_demo.sh`
**Purpose**: Quick validation run testing 4 representative combinations.

**Usage**:
```bash
bash scripts/quick_demo.sh
```

**Test Matrix**:
| Test | Scheduler | Workload | DW | Utilization |
|------|-----------|----------|-----|-------------|
| 1    | EDF       | LIGHT    | OFF | ~50%        |
| 2    | WEIGHTED_EDF | MEDIUM | ON  | ~70%        |
| 3    | WSRT      | HEAVY    | OFF | ~90%        |
| 4    | RMS       | OVERLOAD | ON  | ~140%       |

**Execution Time**: ~5 minutes (70 seconds per test × 4 tests)

**Output**:
- CSV files: `results/quick_demo/EDF_LIGHT_DWOFF.csv`, etc.
- Summary files: `results/quick_demo/EDF_LIGHT_DWOFF_summary.txt`, etc.

---

### 3. `run_all_tests.sh`
**Purpose**: Comprehensive evaluation of all 32 scheduler/workload/DW combinations.

**Usage**:
```bash
bash scripts/run_all_tests.sh <output_directory>
```

**Example**:
```bash
bash scripts/run_all_tests.sh ./results/full_run
```

**Test Matrix**: 4 schedulers × 4 workloads × 2 DW options = 32 tests
- **Schedulers**: EDF, WEIGHTED_EDF, WSRT, RMS
- **Workloads**: LIGHT (~50%), MEDIUM (~70%), HEAVY (~90%), OVERLOAD (~140%)
- **Dynamic Weighting**: OFF, ON

**Execution Time**: ~40 minutes (70 seconds per test × 32 tests)

**Output**:
- 32 CSV files: `{SCHEDULER}_{WORKLOAD}_DW{OFF|ON}.csv`
- 32 Summary files: `{SCHEDULER}_{WORKLOAD}_DW{OFF|ON}_summary.txt`

**CSV Format** (11 columns):
```
type,timestamp,task_id,activation,response_time,exec_time,deadline_met,lateness,period,deadline,weight
```

---

### 4. `generate_graphs.py`
**Purpose**: Create comprehensive visualizations including dynamic weighting analysis.

**Usage**:
```bash
python3 scripts/generate_graphs.py
```

**Prerequisites**: Run `run_all_tests.sh` or `quick_demo.sh` first to generate CSV data.

**Generated Graphs**:

#### Dynamic Weighting Analysis:
1. **`dw_deadline_miss_comparison.png`** - Deadline miss rates with DW ON vs OFF for each scheduler
2. **`dw_response_time_comparison.png`** - Average response times with DW ON vs OFF
3. **`dw_improvement_summary.png`** - Percentage improvement from enabling DW
4. **`weight_evolution_*.png`** - Weight changes over time for each scheduler/workload (DW ON only)

#### Scheduler Comparison:
5. **`scheduler_comparison_heatmap.png`** - 4-panel heatmap showing:
   - Avg response time (DW OFF)
   - Deadline miss rate (DW OFF)
   - Avg response time (DW ON)
   - Deadline miss rate (DW ON)

#### Per-Task Analysis:
6. **`response_time_task1.png`** - Task 1 response times across schedulers (DW OFF vs ON)
7. **`response_time_task2.png`** - Task 2 response times
8. **`response_time_task3.png`** - Task 3 response times
9. **`response_time_task4.png`** - Task 4 response times

#### Text Report:
10. **`summary_report.txt`** - Detailed text summary with:
   - Per-scheduler statistics
   - Per-workload breakdown
   - Dynamic weighting comparison
   - Per-task metrics with average weights

**Output Location**: `results/graphs/`

---

### 5. `run_complete_evaluation.sh`
**Purpose**: Master orchestration script - runs tests and generates graphs automatically.

**Usage**:

**Interactive Mode** (no arguments):
```bash
bash scripts/run_complete_evaluation.sh
```

Shows menu:
```
1) Quick Demo    - 4 tests (~5 minutes)
2) Full Suite    - 32 tests (~40 minutes)
3) Graphs Only   - Generate from existing CSV data
4) Exit
```

**Command-Line Mode**:
```bash
# Quick demo
bash scripts/run_complete_evaluation.sh quick

# Full evaluation
bash scripts/run_complete_evaluation.sh full

# Graphs only (requires existing CSV data)
bash scripts/run_complete_evaluation.sh graphs

# Help
bash scripts/run_complete_evaluation.sh --help
```

**Workflow Steps**:
1. Check Python dependencies
2. Run tests (quick or full)
3. Generate all graphs
4. Display summary

---

## Quick Start

### Minimal Test (Recommended for First Run):
```bash
cd app/simple_eval_step2
bash scripts/quick_demo.sh
python3 scripts/generate_graphs.py
```

### Complete Evaluation (One Command):
```bash
cd app/simple_eval_step2
bash scripts/run_complete_evaluation.sh full
```

### Re-generate Graphs Only:
```bash
cd app/simple_eval_step2
python3 scripts/generate_graphs.py
```

---

## Configuration Switching

The scripts automatically modify configuration files to test different combinations:

### Dynamic Weighting (in `include/workloads.h`):
```c
#define DYNAMIC_WEIGHTING DYNAMIC_WEIGHTING_OFF  // Static weights
#define DYNAMIC_WEIGHTING DYNAMIC_WEIGHTING_ON   // Adaptive weights
```

### Workload Selection (in `include/workloads.h`):
```c
#define CURRENT_WORKLOAD WORKLOAD_LIGHT    // ~50% utilization
#define CURRENT_WORKLOAD WORKLOAD_MEDIUM   // ~70% utilization
#define CURRENT_WORKLOAD WORKLOAD_HEAVY    // ~90% utilization
#define CURRENT_WORKLOAD WORKLOAD_OVERLOAD // ~140% utilization
```

### Scheduler Selection (in `prj.conf`):
```bash
CONFIG_SCHED_DEADLINE=y     # EDF (baseline)
CONFIG_736_MOD_EDF=y        # Weighted EDF
CONFIG_736_WSRT=y           # Weighted Shortest Remaining Time
CONFIG_736_RMS=y            # Rate Monotonic Scheduling
```

**Important**: Scripts restore original configuration after each test run.

---

## Timeouts & Duration

| Script | Timeout/Test | Total Time | Tests |
|--------|--------------|------------|-------|
| `quick_demo.sh` | 70s | ~5 min | 4 |
| `run_all_tests.sh` | 70s | ~40 min | 32 |

---

## Output Files

### Directory Structure:
```
app/simple_eval_step2/
├── results/
│   ├── EDF_LIGHT_DWOFF.csv
│   ├── EDF_LIGHT_DWON.csv
│   ├── ... (32 CSV files)
│   ├── EDF_LIGHT_DWOFF_summary.txt
│   ├── ... (32 summary files)
│   └── graphs/
│       ├── dw_deadline_miss_comparison.png
│       ├── dw_response_time_comparison.png
│       ├── dw_improvement_summary.png
│       ├── scheduler_comparison_heatmap.png
│       ├── response_time_task*.png (4 files)
│       ├── weight_evolution_*.png (variable)
│       └── summary_report.txt
```

---

## Dynamic Weighting Behavior

When **DYNAMIC_WEIGHTING_ON** is enabled:
- Tasks that miss deadlines get increased weight
- Tasks that meet deadlines maintain or decrease weight
- Weight adjustment threshold: 10% deadline miss rate
- Allows scheduler to prioritize struggling tasks

### Weight Evolution Example:
```
Task 1: Initial weight=1.0
  → Misses deadline → weight increases to 1.5
  → Continues missing → weight increases to 2.0
  → Starts meeting deadlines → weight stabilizes
```

### Analysis Focus:
- Does dynamic weighting reduce deadline misses?
- How quickly do weights adapt?
- Which schedulers benefit most from DW?
- What's the overhead of weight adjustment?

---

## Troubleshooting

### Missing Python Dependencies:
```bash
pip3 install pandas matplotlib seaborn numpy
```

### Build Errors:
```bash
cd app/simple_eval_step2
west build -b native_sim -p always app/simple_eval_step2
```

### Permission Denied on Scripts:
```bash
chmod +x scripts/*.sh
```

### No CSV Data for Graphing:
Ensure tests completed successfully:
```bash
ls -lh results/*.csv
```

---

## Comparison with simple_eval_step1

| Feature | step1 | step2 |
|---------|-------|-------|
| **Dynamic Weighting** | No | Yes (ON/OFF) |
| **Test Combinations** | 16 | 32 |
| **Graphs** | 10 | 10+ (includes DW analysis) |
| **Total Runtime** | ~8 min | ~40 min |
| **Weight Tracking** | Static | Adaptive visualization |

---

## References

- Workload Definitions: `../include/workloads.h`
- Dynamic Weighting Threshold: `WEIGHT_ADJUSTMENT_THRESHOLD` (10%)
- Application Source: `../src/main.c`

---

**Last Updated**: December 2024  
**Version**: 1.0 (simple_eval_step2 - Dynamic Weighting Analysis)
