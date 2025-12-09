# RT Scheduler Evaluation Scripts

This directory contains automation scripts for testing and analyzing real-time scheduler performance.

## Scripts Overview

### 1. `run_all_tests.sh`
**Purpose**: Automated test execution  
**What it does**:
- Tests all combinations of schedulers (EDF, Weighted EDF, WSRT, RMS) and workloads (Light, Medium, Heavy, Overload)
- Automatically updates configuration files
- Builds and runs each test
- Collects CSV data and log files
- Restores original configuration when done

**Usage**:
```bash
cd /home/jack/cs736-project/zephyr/app/simple_eval_step1
bash scripts/run_all_tests.sh
```

**Output**:
- `results/*.csv` - CSV data files (16 total: 4 schedulers × 4 workloads)
- `results/*.log` - Full execution logs
- `results/*_summary.txt` - Extracted summary statistics

**Duration**: ~10 minutes (16 tests × 30 seconds each)

---

### 2. `generate_graphs.py`
**Purpose**: Data visualization and analysis  
**What it does**:
- Loads all CSV data from test runs
- Generates comprehensive performance graphs
- Creates summary report

**Usage**:
```bash
cd /home/jack/cs736-project/zephyr/app/simple_eval_step1
python3 scripts/generate_graphs.py
```

**Requirements**:
```bash
pip3 install pandas matplotlib seaborn numpy
```

**Output Graphs**:

1. **`response_time_by_scheduler.png`**
   - 4 subplots (one per task)
   - Grouped bar charts comparing schedulers across workloads
   - Shows average response times

2. **`deadline_miss_rate.png`**
   - Grouped bar chart
   - Shows deadline miss percentage for each scheduler/workload combo
   - Highlights scheduler effectiveness under different loads

3. **`response_time_distribution.png`**
   - 4 subplots (one per workload)
   - Box plots showing response time distribution
   - Reveals variance and outliers

4. **`response_time_timeline_*.png`** (4 files, one per workload)
   - Line graphs showing response time evolution over time
   - 4 subplots (one per task)
   - Shows temporal behavior patterns

5. **`scheduler_comparison_heatmap.png`**
   - Two heatmaps side-by-side
   - Left: Average response time
   - Right: Deadline miss rate
   - Easy comparison across all scheduler/workload combinations

6. **`lateness_distribution.png`**
   - 4 subplots (one per workload)
   - Histograms of lateness for missed deadlines
   - Shows severity of deadline violations

7. **`summary_report.txt`**
   - Text-based comprehensive report
   - Statistics for each scheduler/workload combination
   - Per-task breakdowns

---

## Quick Start

### Complete Workflow

1. **Run all tests** (first time or after changes):
   ```bash
   bash scripts/run_all_tests.sh
   ```

2. **Generate graphs**:
   ```bash
   python3 scripts/generate_graphs.py
   ```

3. **View results**:
   ```bash
   # Open graphs
   xdg-open results/graphs/*.png
   
   # Read summary report
   cat results/graphs/summary_report.txt
   ```

---

## Directory Structure

After running the scripts:

```
app/simple_eval_step1/
├── scripts/
│   ├── README.md              # This file
│   ├── run_all_tests.sh       # Test automation
│   └── generate_graphs.py     # Graph generation
├── results/
│   ├── EDF_LIGHT.csv          # CSV data files (16 total)
│   ├── EDF_LIGHT.log          # Log files (16 total)
│   ├── EDF_LIGHT_summary.txt  # Summary files (16 total)
│   └── graphs/
│       ├── response_time_by_scheduler.png
│       ├── deadline_miss_rate.png
│       ├── response_time_distribution.png
│       ├── response_time_timeline_LIGHT.png
│       ├── response_time_timeline_MEDIUM.png
│       ├── response_time_timeline_HEAVY.png
│       ├── response_time_timeline_OVERLOAD.png
│       ├── scheduler_comparison_heatmap.png
│       ├── lateness_distribution.png
│       └── summary_report.txt
```

---

## Customization

### Test Duration
Edit `run_all_tests.sh`:
```bash
TIMEOUT=30  # Change to desired seconds per test
```

### Number of Activations
Edit `src/main.c`:
```c
#define MAX_ACTIVATIONS 50  // Change to desired activations per task
```

### Workloads to Test
Edit `run_all_tests.sh`:
```bash
WORKLOADS=(
    "LIGHT:WORKLOAD_LIGHT"
    "MEDIUM:WORKLOAD_MEDIUM"
    # Comment out workloads you don't want to test
)
```

### Schedulers to Test
Edit `run_all_tests.sh`:
```bash
SCHEDULERS=(
    "EDF:CONFIG_SCHED_DEADLINE"
    "WEIGHTED_EDF:CONFIG_736_MOD_EDF"
    # Comment out schedulers you don't want to test
)
```

### Graph Styling
Edit `generate_graphs.py`:
```python
# Change color schemes
SCHEDULER_COLORS = {
    "EDF": "#1f77b4",  # Your custom colors
    # ...
}

# Change figure sizes
plt.rcParams['figure.figsize'] = (12, 8)  # (width, height)
```

---

## Troubleshooting

### "No data available to plot!"
- Run `bash scripts/run_all_tests.sh` first
- Check that `results/*.csv` files exist
- Verify CSV files contain data (not empty)

### Build Failures
- Check `results/*.log` files for error messages
- Verify all schedulers are properly implemented in kernel
- Ensure `CONFIG_736=y` and `CONFIG_SCHED_DEADLINE=y` are set

### Missing Python Packages
```bash
pip3 install pandas matplotlib seaborn numpy
# or
sudo apt install python3-pandas python3-matplotlib python3-seaborn python3-numpy
```

### Script Permission Denied
```bash
chmod +x scripts/run_all_tests.sh
```

---

## Analysis Tips

### Identifying Best Scheduler
1. Check `deadline_miss_rate.png` - look for lowest bars
2. Review `scheduler_comparison_heatmap.png` - darker is worse (for miss rate)
3. Read `summary_report.txt` - detailed statistics

### Understanding Workload Impact
1. Check `response_time_distribution.png` - see how variance increases
2. Review timeline graphs - see when system becomes unstable
3. Compare Light vs Overload in all graphs

### Per-Task Analysis
1. Look at `response_time_by_scheduler.png` - each subplot is one task
2. Check per-task breakdown in `summary_report.txt`
3. Identify which tasks miss deadlines most often

---

## Expected Results

### Light Workload (~58% utilization)
- All schedulers should have 0% miss rate
- Response times close to execution times
- Low variance

### Medium Workload (~70% utilization)
- Most schedulers should handle well
- Slight increase in response times
- Some variance

### Heavy Workload (~90% utilization)
- Some schedulers may show occasional misses
- Higher response times
- More variance

### Overload Workload (~140% utilization)
- All schedulers will miss deadlines
- Compare which scheduler degrades gracefully
- High variance and lateness

---

## Contributing

To add new analysis graphs:

1. Add function to `generate_graphs.py`:
   ```python
   def plot_my_analysis(df):
       # Your plotting code
       output_file = GRAPHS_DIR / "my_graph.png"
       plt.savefig(output_file, dpi=300, bbox_inches='tight')
       print(f"✓ Saved: {output_file.name}")
       plt.close()
   ```

2. Call in `main()`:
   ```python
   plot_my_analysis(df)
   ```

---

## References

- Zephyr RT Scheduler Documentation: Check `GUIDE.md` in parent directory
- Pandas Documentation: https://pandas.pydata.org/docs/
- Matplotlib Documentation: https://matplotlib.org/stable/contents.html
- Seaborn Documentation: https://seaborn.pydata.org/
