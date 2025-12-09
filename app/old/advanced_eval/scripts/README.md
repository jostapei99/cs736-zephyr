# Advanced RT Scheduler Evaluation - Scripts Documentation

This directory contains automation scripts for running comprehensive real-time scheduler evaluations with the **enhanced advanced_eval** application.

## Overview

The scripts automate testing of 4 RT schedulers across 4 workload profiles, collecting detailed metrics including **response time, execution time, deadline compliance, and jitter analysis**.

### Key Enhancements over simple_eval_step1:
- **Jitter Tracking**: Measures response time variance (std deviation)
- **Extended Metrics**: Execution time tracking (min/max/avg), lateness statistics
- **Interactive Shell**: Runtime configuration and monitoring via shell commands
- **Multiple Output Formats**: CSV, JSON, Human-readable, Quiet modes
- **Longer Test Duration**: 100 activations per task (vs 50 in simple_eval)

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

**Output**: Reports which packages are installed/missing.

---

### 2. `quick_demo.sh`
**Purpose**: Quick validation run testing 4 scheduler/workload combinations.

**Usage**:
```bash
bash scripts/quick_demo.sh
```

**Test Matrix**:
| Test | Scheduler | Workload | Utilization |
|------|-----------|----------|-------------|
| 1    | EDF       | LIGHT    | ~58%        |
| 2    | WEIGHTED_EDF | MEDIUM | ~70%        |
| 3    | WSRT      | HEAVY    | ~90%        |
| 4    | RMS       | OVERLOAD | ~140%       |

**Execution Time**: ~80 seconds (20 seconds per test × 4 tests)

**Output**:
- CSV files: `results/EDF_LIGHT.csv`, etc.
- Summary files: `results/EDF_LIGHT_summary.txt`, etc.

---

### 3. `run_all_tests.sh`
**Purpose**: Comprehensive evaluation of all 16 scheduler/workload combinations.

**Usage**:
```bash
bash scripts/run_all_tests.sh
```

**Test Matrix**: 4 schedulers × 4 workloads = 16 tests
- **Schedulers**: EDF, WEIGHTED_EDF, WSRT, RMS
- **Workloads**: LIGHT (~58%), MEDIUM (~70%), HEAVY (~90%), OVERLOAD (~140%)

**Execution Time**: ~10 minutes (40 seconds per test × 16 tests)

**Timeout**: 40 seconds per test (vs 30s in simple_eval_step1 due to 100 activations)

**Output**:
- 16 CSV files: `results/{SCHEDULER}_{WORKLOAD}.csv`
- 16 Summary files: `results/{SCHEDULER}_{WORKLOAD}_summary.txt`
- Master summary: `results/test_summary.txt`

**CSV Format** (11 columns, includes jitter):
```
type,timestamp,task_id,activation,response_time,exec_time,deadline_met,lateness,period,deadline,weight,jitter
```

---

### 4. `generate_graphs.py`
**Purpose**: Create comprehensive visualizations from CSV data, including jitter analysis.

**Usage**:
```bash
python3 scripts/generate_graphs.py
```

**Prerequisites**: Run `run_all_tests.sh` or `quick_demo.sh` first to generate CSV data.

**Generated Graphs** (13 total):

#### Response Time Analysis:
1. **`response_time_by_scheduler.png`** - Bar chart comparing avg response time across schedulers
2. **`response_time_distribution.png`** - Box plots showing response time distributions
3. **`response_time_timeline_LIGHT.png`** - Response time evolution over time (LIGHT workload)
4. **`response_time_timeline_MEDIUM.png`** - Response time evolution (MEDIUM)
5. **`response_time_timeline_HEAVY.png`** - Response time evolution (HEAVY)
6. **`response_time_timeline_OVERLOAD.png`** - Response time evolution (OVERLOAD)

#### Deadline & Lateness Analysis:
7. **`deadline_miss_rate.png`** - Bar chart of deadline miss percentages
8. **`lateness_distribution.png`** - Histograms of lateness for missed deadlines

#### Jitter Analysis (NEW in advanced_eval):
9. **`jitter_comparison.png`** - Bar chart comparing max jitter across schedulers
10. **`jitter_evolution_LIGHT.png`** - Jitter over time (LIGHT workload)
11. **`jitter_evolution_MEDIUM.png`** - Jitter over time (MEDIUM)
12. **`jitter_evolution_HEAVY.png`** - Jitter over time (HEAVY)
13. **`jitter_evolution_OVERLOAD.png`** - Jitter over time (OVERLOAD)

#### Execution Time Analysis (NEW):
14. **`exec_time_accuracy.png`** - Box plots showing execution time deviations

#### Heatmaps:
15. **`scheduler_comparison_heatmap.png`** - 3-panel heatmap showing:
   - Average response time
   - Deadline miss rate
   - Maximum jitter (NEW)

#### Text Report:
16. **`summary_report.txt`** - Detailed text summary with jitter statistics

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
1) Quick Demo    - 4 tests (~80 seconds)
2) Full Suite    - 16 tests (~10 minutes)
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
cd app/advanced_eval
bash scripts/quick_demo.sh
python3 scripts/generate_graphs.py
```

### Complete Evaluation (One Command):
```bash
cd app/advanced_eval
bash scripts/run_complete_evaluation.sh full
```

### Re-generate Graphs Only:
```bash
cd app/advanced_eval
python3 scripts/generate_graphs.py
```

---

## Configuration Switching

The scripts automatically modify `prj.conf` to enable different schedulers:

```bash
# Enable EDF
CONFIG_SCHED_DEADLINE=y
# CONFIG_736_MOD_EDF=n
# CONFIG_736_WSRT=n
# CONFIG_736_RMS=n

# Enable Weighted EDF
# CONFIG_SCHED_DEADLINE=n
CONFIG_736_MOD_EDF=y
# CONFIG_736_WSRT=n
# CONFIG_736_RMS=n
```

Workloads are switched by modifying `src/main.c`:
```c
// static struct task_config *task_configs = light_workload;
static struct task_config *task_configs = medium_workload;
```

**Important**: Scripts restore original configuration after each test.

---

## Timeouts & Duration

| Script | Timeout/Test | Total Time | Reason |
|--------|--------------|------------|--------|
| `quick_demo.sh` | 20s | ~80s | 100 activations + shell overhead |
| `run_all_tests.sh` | 40s | ~10min | 100 activations + safety margin |

**Note**: Timeouts are longer than simple_eval_step1 (30s) because advanced_eval runs 100 activations per task (vs 50).

---

## Output Files

### Directory Structure:
```
app/advanced_eval/
├── results/
│   ├── EDF_LIGHT.csv
│   ├── EDF_LIGHT_summary.txt
│   ├── ... (16 CSV + 16 summary files)
│   ├── test_summary.txt
│   └── graphs/
│       ├── response_time_by_scheduler.png
│       ├── jitter_comparison.png
│       ├── jitter_evolution_LIGHT.png
│       ├── ... (13 PNG files)
│       └── summary_report.txt
```

### CSV Data Format:
```csv
type,timestamp,task_id,activation,response_time,exec_time,deadline_met,lateness,period,deadline,weight,jitter
RT_METRICS,5125,1,1,5,5,1,0,20,20,1.0,0.00
RT_METRICS,10188,2,1,5,5,1,0,30,30,2.0,0.00
```

**Columns**:
- `type`: "RT_METRICS" or "RT_SUMMARY"
- `timestamp`: Milliseconds since start
- `task_id`: 1-4
- `activation`: Activation number (1-100)
- `response_time`: Time from release to completion (ms)
- `exec_time`: Actual execution time (ms)
- `deadline_met`: 1=met, 0=missed
- `lateness`: ms past deadline (0 if met)
- `period`: Task period (ms)
- `deadline`: Task deadline (ms)
- `weight`: Task weight (for Weighted EDF)
- `jitter`: Response time std deviation (ms) **[NEW]**

### Summary Text Format:
Extracts the human-readable summary from shell output:
```
╔═══════════════════════════════════════════════════════════════════════════════╗
║                         RT SCHEDULER STATISTICS                               ║
╠═══════════════════════════════════════════════════════════════════════════════╣
║ Task 1: Activations=100, Misses=0 (0%), RT: avg=5.2ms, min=4ms, max=7ms      ║
║ Task 2: Activations=100, Misses=0 (0%), RT: avg=10.1ms, min=9ms, max=12ms    ║
...
```

---

## Troubleshooting

### Missing Python Dependencies:
```bash
pip3 install pandas matplotlib seaborn numpy
```

### Build Errors:
```bash
cd app/advanced_eval
west build -b native_sim -p  # Pristine build
```

### Timeout Issues:
If tests timeout, increase values in `run_all_tests.sh`:
```bash
TIMEOUT=60  # Increase from 40s
```

### No CSV Data:
Ensure application completed successfully:
```bash
# Check for errors in summary files
cat results/*_summary.txt | grep -i error
```

### Graph Generation Fails:
Check CSV file structure:
```bash
# Should have 11 columns
head -n 1 results/EDF_LIGHT.csv
```

---

## Advanced Usage

### Custom Workload Testing:
Modify `src/main.c` to create custom workload profiles:
```c
struct task_config custom_workload[] = {
    {1, 20, 20, 5, 10, 1.0},   // task_id, period, deadline, weight, exec_time, weight_factor
    {2, 40, 40, 5, 15, 1.5},
    {3, 60, 60, 5, 20, 2.0},
    {4, 80, 80, 5, 25, 2.5}
};
```

Then run manually:
```bash
west build -b native_sim
west build -t run > output.txt 2>&1
```

### Shell Commands During Execution:
When running manually, use interactive shell:
```
rt show           # Show current configuration
rt stats          # Display statistics
rt format csv     # Switch output format
rt format json    # JSON output
rt format human   # Human-readable
rt util           # Show CPU utilization
rt reset          # Reset metrics
```

### Comparing with simple_eval_step1:
```bash
# Run both applications
cd app/simple_eval_step1
bash scripts/run_all_tests.sh

cd ../advanced_eval
bash scripts/run_all_tests.sh

# Compare results
diff simple_eval_step1/results/EDF_LIGHT.csv advanced_eval/results/EDF_LIGHT.csv
```

---

## Performance Notes

- **Native Sim**: ~100ms wall-clock time ≈ 1 second simulated time
- **100 Activations**: Longest task (Task4: 80ms period) → ~8 seconds simulated → ~800ms wall-clock (minimum)
- **Safety Margin**: Scripts use 40s timeout to allow for shell processing and output buffering
- **CSV Overhead**: Shell and metrics processing add ~5-10% overhead vs simple_eval_step1

---

## References

- Application Guide: `../GUIDE.md`
- Application Status: `../STATUS.md`
- Workload Definitions: `../src/main.c`
- Metrics Implementation: `../src/metrics.c`
- Shell Commands: `../src/shell_commands.c`

---

## Support

For issues or questions:
1. Check `STATUS.md` for known issues
2. Review `GUIDE.md` for detailed documentation
3. Verify Python dependencies with `check_deps.py`
4. Try pristine build: `west build -b native_sim -p`

---

**Last Updated**: December 2024  
**Version**: 1.0 (advanced_eval)
