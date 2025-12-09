# Scheduler Evaluation Suite - Changelog

## Latest Updates (December 8, 2025)

### Enhancement: Utilization-Normalized Metrics

**What Changed:**
- Added **theoretical utilization** calculation: sum of (exec_time/period) for all threads
- Added **normalized miss rate**: miss_rate / theoretical_utilization
- Display both raw and normalized metrics in results table
- Added "Util%" and "NormMR" columns to table output
- CSV now includes Utilization and NormalizedMissRate columns

**Why This Matters:**
- **Reveals scheduler efficiency** independent of workload difficulty
- A scheduler with 20% miss @ 50% util (norm=40) is **worse** than 40% miss @ 200% util (norm=20)
- Allows fair comparison across different load scenarios
- Shows which schedulers handle overload most gracefully per unit of workload

**Example Interpretation:**
- PFS: 11% miss @ 150% avg util → normalized ≈ 7.3 (excellent efficiency)
- Base-EDF: 61% miss @ 150% avg util → normalized ≈ 40.7 (poor efficiency)
- PFS is **5.6× more efficient** per utilization unit

**Impact:**
- Better understanding of scheduler behavior under stress
- Can now compare "apples to apples" across different workload intensities
- Research value: reveals intrinsic scheduler efficiency

### Enhancement: Extended Test Duration to 10 Seconds

**What Changed:**
- Test duration increased from **5 seconds to 10 seconds** per scenario
- Script timeout increased from 100s to 200s per scheduler

**Why This Matters:**
- **Better statistical accuracy** - More activations per test
- **Captures long-term behavior** - Sees patterns beyond initial transients
- **More realistic evaluation** - Real systems run much longer than 5s
- **Improved steady-state metrics** - Initial startup effects are diluted

**Impact:**
- Total evaluation time: ~50-60 minutes (was ~25-30 minutes)
- More activations per test = better statistical confidence
- Slight increase in overhead tracking accuracy

### Enhancement: Dynamic Time-Left Tracking for LLF/WSRT

**What Changed:**
- Execution now happens in **5ms slices** instead of one continuous busy-wait
- `time_left` is **decremented during execution** to reflect actual remaining time
- Scheduler is **yielded to** after each slice for priority reevaluation

**Why This Matters:**
- **More realistic LLF behavior** - laxity changes dynamically as tasks execute
- **Enables laxity-based preemption** - tasks with decreasing laxity can be preempted
- **Better WSRT evaluation** - remaining time factor updates during execution
- **Matches theoretical algorithms** - both LLF and WSRT assume runtime tracking

**Implementation:**
```c
// Execute in 5ms slices with time_left updates
while (remaining_us > 0) {
    k_busy_wait(5ms);
    remaining_us -= 5ms;
    
    // Update remaining execution time for scheduler
    k_thread_time_left_set(current, new_time_left);
    
    // Allow scheduler to reevaluate with updated laxity
    k_yield();
}
```

**Performance Impact:**
- Context switches increase from ~1.5 to ~4-9 per activation
- Overhead increases from ~0.04% to ~0.08-0.24%
- More realistic scheduler behavior under dynamic priority algorithms

**Trade-off:** Slightly higher overhead for significantly more realistic LLF/WSRT evaluation.

### Bug Fix: Kernel Overhead Tracking

**Issue:** Context switches and overhead metrics were always showing 0.00

**Root Cause:** Missing Kconfig dependencies
- `CONFIG_736_RT_STATS` requires `CONFIG_736_ADD_ONS=y`
- Context switch tracking requires `CONFIG_INSTRUMENT_THREAD_SWITCHING=y`
- `INSTRUMENT_THREAD_SWITCHING` requires `CONFIG_THREAD_RUNTIME_STATS=y`

**Fix Applied:**
- Added `CONFIG_THREAD_RUNTIME_STATS=y` to enable runtime statistics
- Changed `CONFIG_736_ADD_ONS=y` (was `n`) to enable RT stats infrastructure
- Updated `run_evaluation.sh` to keep `CONFIG_736_ADD_ONS=y` even for Base-EDF
- Context switches, preemptions, and overhead % now report correctly

**Impact:** 
- All kernel overhead metrics now functional
- Baseline values (without time_left slicing): 1.0-1.5 CS/Act, 0.02-0.06% overhead
- With time_left slicing: 4-9 CS/Act, 0.08-0.24% overhead

### Major Enhancements

#### 1. Extended Test Duration
- **Changed from 2 seconds → 5 seconds → 10 seconds per scenario**
- Provides more accurate statistical measurements
- Better captures steady-state behavior
- Total evaluation time: ~50-60 minutes for full suite

#### 2. Response Time and Jitter Metrics
**New latency measurements:**
- **Average Response Time** - Mean time from task activation to completion
- **Maximum Response Time** - Worst-case latency observed
- **Jitter** - Timing variance (max - min response time)
  - Critical for real-time predictability
  - Low jitter = consistent timing
  - High jitter = timing uncertainty

**Why This Matters:**
- Deadline miss rate alone doesn't show HOW CLOSE tasks are to deadlines
- Response time shows actual task completion latency
- Jitter reveals timing predictability and consistency
- Essential for hard real-time system validation

#### 3. Kernel Overhead Analysis
**New overhead measurements:**
- **Context Switches** - Total scheduler context switch operations
- **Preemptions** - Number of times tasks were preempted
- **CS per Activation** - Average context switches per task activation
- **Scheduling Overhead %** - Estimated CPU time spent in scheduler
  - Calculated as: (context_switches × 10μs) / test_duration_ms × 100%

**Why This Matters:**
- Shows real cost of scheduling decisions
- Reveals scheduler efficiency
- Higher overhead = less CPU time for actual tasks
- Helps compare intrusive vs. lightweight schedulers

### Updated Output Format

#### Enhanced CSV Columns
```csv
Scheduler,Threads,Load,Activations,Misses,MissRate,AvgResponseTime,MaxResponseTime,Jitter,ContextSwitches,Preemptions,CSPerActivation,OverheadPercent
```

**Before (6 columns):**
- Scheduler, Threads, Load, Activations, Misses, MissRate

**Now (13 columns):**
- Added: AvgResponseTime, MaxResponseTime, Jitter
- Added: ContextSwitches, Preemptions, CSPerActivation, OverheadPercent

#### Enhanced Table Display
**Before:**
```
│ Th │   Load    │ Activ │ Miss  │ Miss%  │
```

**Now:**
```
│ Th │   Load    │ Activ │ Miss  │ Miss%  │ AvgRT │ MaxRT │ Jittr │ CS/Act │ Ovrhd% │
```

### New Visualizations

#### 4. Kernel Overhead Plot (`kernel_overhead.png`)
Two-panel visualization:
- **Left panel**: Average scheduling overhead percentage per scheduler
  - Shows CPU time consumed by scheduler
  - Lower is better (less intrusive)
- **Right panel**: Context switches per activation
  - Indicates scheduling activity/aggressiveness
  - Reveals scheduler decision frequency

#### 5. Response Time Analysis (`response_time_analysis.png`)
Four-panel comprehensive latency visualization:
- **Top-left**: Average response time vs. load level
  - Shows how latency scales with load
- **Top-right**: Maximum response time vs. load level
  - Reveals worst-case behavior
- **Bottom-left**: Jitter vs. load level
  - Shows timing predictability degradation
- **Bottom-right**: Overall average response time comparison
  - Bar chart comparing scheduler responsiveness

### Enhanced Analysis Report

**New sections in `analysis.txt`:**

```
Latency Metrics:
  Avg response time:      18.5 ms
  Max response time:      87 ms
  Avg jitter:             12.3 ms
  Max jitter:             45 ms

Kernel Overhead:
  Scheduling overhead:    0.23%
  CS per activation:      2.85
  Total context switches: 1547
```

### Technical Implementation

#### Source Code Changes (`src/main.c`)
- Extended `TEST_DURATION_MS` from 2000 to 5000
- Added `context_switches` tracking to thread statistics
- Collect from RT stats structure: `rt_stats.context_switches`
- Calculate overhead: `(total_cs × 10μs) / duration × 100%`
- Updated table format to display all 10 metric columns
- Enhanced summary section with latency and overhead statistics

#### Script Changes (`run_evaluation.sh`)
- Increased timeout from 70s to 100s (16 scenarios × 5s + overhead)
- Updated CSV headers to include all 13 columns
- Modified extraction to parse 10-column table format
- Calculate context_switches and preemptions from table data

#### Plotting Changes (`plot_results.py`)
- Added `plot_kernel_overhead()` function
- Added `plot_response_time_analysis()` function
- Updated `load_csv()` to handle 7 new columns
- Enhanced `generate_text_report()` with latency/overhead metrics
- Now generates 5 plots total (was 3)

### Documentation Updates

#### README.md
- Updated test duration to 5 seconds
- Added complete CSV column descriptions
- Added "Metrics Collected" section explaining all measurements
- Updated example results with new columns
- Extended duration estimate to 25-30 minutes

#### EVALUATION_GUIDE.md
- Expanded "Metrics Collected" section
- Added detailed explanations of latency metrics
- Added kernel overhead calculation details
- Updated CSV format documentation
- Added descriptions for new visualizations
- Updated duration estimates

### Migration Notes

**For existing results:**
- Old CSV files (6 columns) will still work with plotting script
- New columns default to 0 if missing (backward compatible)
- `load_csv()` uses `.get()` for optional columns

**Breaking changes:**
- None - fully backward compatible
- Old results can still be plotted
- New results have richer data

### Performance Insights Unlocked

With these new metrics, you can now answer:

1. **"How responsive is each scheduler?"**
   - Average and maximum response times show latency characteristics

2. **"How predictable is timing?"**
   - Jitter reveals consistency of task execution times

3. **"What's the real cost of scheduling?"**
   - Overhead percentage shows CPU time lost to scheduler

4. **"How aggressive is the scheduler?"**
   - Context switches per activation reveals decision frequency

5. **"Which scheduler is most efficient?"**
   - Compare overhead across schedulers
   - Lower overhead = more CPU time for tasks

6. **"Is performance degradation graceful?"**
   - Response time curves show how latency increases with load
   - Jitter curves show how predictability degrades

### Example Interpretations

**Low overhead + low jitter + acceptable miss rate:**
- ✅ Excellent scheduler - efficient and predictable

**High overhead + low jitter + low miss rate:**
- ⚠️ Works but intrusive - succeeds at cost of CPU cycles

**Low overhead + high jitter + low miss rate:**
- ⚠️ Efficient but unpredictable - may have timing surprises

**High overhead + high jitter + high miss rate:**
- ❌ Poor scheduler - expensive and unreliable

### Files Modified

1. `src/main.c` (561 → 595 lines)
   - Extended test duration
   - Added response time/jitter/overhead metrics
   - Enhanced table and summary output

2. `scripts/run_evaluation.sh` (188 → 192 lines)
   - Increased timeout
   - Updated CSV extraction for 10 columns

3. `scripts/plot_results.py` (226 → 358 lines)
   - Added 2 new plotting functions
   - Enhanced data loading
   - Updated analysis report

4. `README.md`
   - Added metrics documentation
   - Updated examples
   - Added interpretation guidelines

5. `EVALUATION_GUIDE.md`
   - Comprehensive metric explanations
   - Updated all examples
   - Added new visualization descriptions

### Testing

All changes compile successfully:
```bash
west build -b native_sim app/scheduler_evaluation
# Build: SUCCESS
```

Ready to run:
```bash
./scripts/run_evaluation.sh
# Duration: ~25-30 minutes for all 6 schedulers
# Output: 5 plots + enhanced analysis
```

---

## Summary

This update transforms the evaluation suite from a basic deadline miss counter into a comprehensive real-time performance analysis tool. The addition of response time, jitter, and kernel overhead metrics provides deep insights into scheduler behavior, efficiency, and predictability - essential for real-time system validation and scheduler comparison.
