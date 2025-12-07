# RT Scheduler Evaluation - Automation & Graphing Guide

## 🎯 Quick Start

The simplest way to run everything:

```bash
cd /home/jack/cs736-project/zephyr/app/simple_eval_step1
bash scripts/run_complete_evaluation.sh
```

This will:
1. Check Python dependencies
2. Let you choose Quick Demo or Full Suite
3. Run all tests automatically
4. Generate all graphs
5. Create summary report

---

## 📊 What Graphs Are Generated?

### 1. Response Time by Scheduler
**File:** `response_time_by_scheduler.png`

Shows average response time for each task across all schedulers and workloads.
- 4 subplots (one per task)
- Grouped bar charts
- **Use for:** Comparing scheduler efficiency

### 2. Deadline Miss Rate
**File:** `deadline_miss_rate.png`

Shows percentage of missed deadlines for each scheduler/workload combo.
- Single chart with all combinations
- **Use for:** Identifying most reliable scheduler

### 3. Response Time Distribution
**File:** `response_time_distribution.png`

Box plots showing response time variance.
- 4 subplots (one per workload)
- Shows min/max/median/quartiles
- **Use for:** Understanding response time predictability

### 4. Response Time Timeline (4 files)
**Files:** `response_time_timeline_LIGHT.png`, `_MEDIUM.png`, `_HEAVY.png`, `_OVERLOAD.png`

Shows how response time changes over the test duration.
- 4 subplots per file (one per task)
- Line graphs over time
- **Use for:** Seeing scheduler behavior patterns and stability

### 5. Scheduler Comparison Heatmap
**File:** `scheduler_comparison_heatmap.png`

Two side-by-side heatmaps:
- Left: Average response time (lower is better)
- Right: Deadline miss rate (lower is better)
- **Use for:** At-a-glance comparison of all combinations

### 6. Lateness Distribution
**File:** `lateness_distribution.png`

Histograms showing how late tasks are when they miss deadlines.
- 4 subplots (one per workload)
- Only shows missed deadlines
- **Use for:** Understanding severity of failures

### 7. Summary Report
**File:** `summary_report.txt`

Text file with detailed statistics for every scheduler/workload combination.
- Per-task breakdown
- Total activations, misses, response times
- **Use for:** Detailed analysis and paper writing

---

## 🔧 Script Details

### `run_complete_evaluation.sh`
**Purpose:** One-command solution  
**Duration:** Depends on choice (1-10 minutes)  
**Output:** Everything

### `quick_demo.sh`
**Purpose:** Fast verification  
**Tests:** 4 combinations (one from each category)  
**Duration:** ~1 minute  
**Use when:** Testing changes or verifying setup

### `run_all_tests.sh`
**Purpose:** Comprehensive evaluation  
**Tests:** All 16 combinations (4 schedulers × 4 workloads)  
**Duration:** ~10 minutes  
**Use when:** Final evaluation or research data collection

### `generate_graphs.py`
**Purpose:** Data visualization  
**Input:** CSV files from `results/*.csv`  
**Output:** Graphs in `results/graphs/`  
**Requirements:** pandas, matplotlib, seaborn, numpy

### `check_deps.py`
**Purpose:** Verify Python environment  
**Checks:** pandas, matplotlib, seaborn, numpy  
**Use:** Run before graph generation

---

## 📁 Output Structure

After running tests:

```
results/
├── EDF_LIGHT.csv              # CSV data
├── EDF_LIGHT.log              # Full execution log
├── EDF_LIGHT_summary.txt      # Extracted statistics
├── ... (16 files total for each combination)
└── graphs/
    ├── response_time_by_scheduler.png
    ├── deadline_miss_rate.png
    ├── response_time_distribution.png
    ├── response_time_timeline_LIGHT.png
    ├── response_time_timeline_MEDIUM.png
    ├── response_time_timeline_HEAVY.png
    ├── response_time_timeline_OVERLOAD.png
    ├── scheduler_comparison_heatmap.png
    ├── lateness_distribution.png
    └── summary_report.txt
```

---

## 🎨 Customization Examples

### Change Test Duration

Edit `scripts/run_all_tests.sh`:
```bash
TIMEOUT=60  # Change from 30 to 60 seconds
```

### Change Activations Per Task

Edit `src/main.c`:
```c
#define MAX_ACTIVATIONS 100  // Change from 50
```

Then rebuild and retest.

### Test Subset of Schedulers

Edit `scripts/run_all_tests.sh`:
```bash
SCHEDULERS=(
    "EDF:CONFIG_SCHED_DEADLINE"
    "WEIGHTED_EDF:CONFIG_736_MOD_EDF"
    # "WSRT:CONFIG_736_WSRT"        # Comment out
    # "RMS:CONFIG_736_RMS"          # Comment out
)
```

### Test Subset of Workloads

Edit `scripts/run_all_tests.sh`:
```bash
WORKLOADS=(
    "LIGHT:WORKLOAD_LIGHT"
    "OVERLOAD:WORKLOAD_OVERLOAD"
    # "MEDIUM:WORKLOAD_MEDIUM"      # Comment out
    # "HEAVY:WORKLOAD_HEAVY"        # Comment out
)
```

### Change Graph Colors

Edit `scripts/generate_graphs.py`:
```python
SCHEDULER_COLORS = {
    "EDF": "#YOUR_COLOR",
    "WEIGHTED_EDF": "#YOUR_COLOR",
    # ...
}
```

### Change Graph Size

Edit `scripts/generate_graphs.py`:
```python
plt.rcParams['figure.figsize'] = (16, 10)  # Change from (12, 8)
```

---

## 📈 Interpreting Results

### Good Scheduler Characteristics
- **Low deadline miss rate** (especially under LIGHT/MEDIUM)
- **Consistent response times** (narrow box plots)
- **Stable timeline** (flat lines, not chaotic)
- **Graceful degradation** (under OVERLOAD)

### Warning Signs
- High miss rate under LIGHT/MEDIUM workloads
- Wide variance in response times (large boxes in box plots)
- Erratic timeline behavior
- Sudden performance cliff (not gradual)

### Workload Expectations

| Workload | Expected Behavior |
|----------|-------------------|
| LIGHT (58%) | All schedulers should have 0% miss rate |
| MEDIUM (70%) | Most schedulers handle well, maybe occasional misses |
| HEAVY (90%) | Some schedulers start showing stress |
| OVERLOAD (140%) | All will miss deadlines - compare degradation |

---

## 🔬 Research Use Cases

### For a Paper/Thesis

1. Run full test suite:
   ```bash
   bash scripts/run_all_tests.sh
   ```

2. Generate all graphs:
   ```bash
   python3 scripts/generate_graphs.py
   ```

3. Use graphs in paper:
   - `scheduler_comparison_heatmap.png` - Overview figure
   - `deadline_miss_rate.png` - Main results
   - `response_time_distribution.png` - Detailed analysis

4. Reference `summary_report.txt` for exact numbers

### For Quick Comparison

1. Run quick demo:
   ```bash
   bash scripts/quick_demo.sh
   ```

2. Look at heatmap for overview

### For Debugging Scheduler

1. Test just one scheduler/workload:
   - Edit workloads.h and prj.conf manually
   - Build and run once
   - Check log file

2. Or use timeline graphs to see temporal behavior

---

## 🐛 Troubleshooting

### "No data available to plot!"
**Solution:** Run tests first: `bash scripts/run_all_tests.sh`

### Build failures
**Check:**
- `results/*.log` files for errors
- Scheduler configs in `prj.conf`
- Kernel implementation

### Missing Python packages
**Solution:** 
```bash
pip3 install pandas matplotlib seaborn numpy
```

### Graphs look weird
**Check:**
- CSV files aren't empty: `wc -l results/*.csv`
- CSV format is correct: `head results/EDF_LIGHT.csv`
- Python script completed: check for error messages

### Script won't run
**Solution:**
```bash
chmod +x scripts/*.sh
```

---

## 💡 Tips & Tricks

### Parallel Testing
The scripts run sequentially. For faster results, manually run multiple instances with different scheduler/workload combos (not recommended unless you know what you're doing).

### Comparing Before/After Changes
1. Run tests and save: `mv results results_before`
2. Make your changes
3. Run tests again: creates new `results/`
4. Compare: `diff results_before/*.csv results/*.csv`

### Exporting for Excel
CSV files can be directly opened in Excel/LibreOffice for custom analysis.

### Adding Custom Metrics
Edit `generate_graphs.py` and add new plotting functions. The DataFrame `df` contains all CSV data with columns: `timestamp`, `task_id`, `activation`, `response_time`, `deadline_met`, `lateness`, `period`, `deadline`, `weight`, `scheduler`, `workload`.

---

## 📚 Further Reading

See `scripts/README.md` for detailed script documentation.
See `STATUS.md` for overall project status.
See `../GUIDE.md` for RT scheduler implementation guide.

---

**Last Updated:** December 4, 2025  
**Status:** ✅ Ready for use
