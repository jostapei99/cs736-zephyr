# Advanced RT Scheduler Evaluation - User Guide

## Overview

`advanced_eval` is an enhanced real-time scheduler evaluation application that builds upon `simple_eval_step1` with advanced metrics, runtime configuration, and interactive shell commands.

## Key Features

### 1. Enhanced Metrics
- **Jitter Analysis**: Calculates response time variance and standard deviation
- **Execution Time Tracking**: Min/max/average actual execution times
- **Lateness Statistics**: Tracks how late tasks are when missing deadlines
- **System-Wide Metrics**: Aggregated statistics across all tasks

### 2. Interactive Shell
Runtime control via shell commands:
- `rt show` - Display current task configuration
- `rt stats` - Show real-time statistics
- `rt format <csv|json|human|quiet>` - Change output format
- `rt set <task> <param> <value>` - Modify task parameters
- `rt reset` - Reset all statistics
- `rt util` - Show CPU utilization analysis

### 3. Multiple Output Formats
- **CSV**: Compatible with simple_eval_step1 (plus jitter column)
- **JSON**: Machine-readable structured data
- **Human**: Formatted tables with detailed statistics
- **Quiet**: Minimal output for performance testing

### 4. Runtime Configuration
Modify task parameters without rebuilding:
- Change execution times
- Adjust periods and deadlines
- Modify weights for weighted schedulers
- Test different scenarios interactively

## Building and Running

### Build
```bash
cd /home/jack/cs736-project/zephyr
west build -b native_sim app/advanced_eval
```

### Run
```bash
west build -t run
```

The application will start with a shell prompt:
```
rt_eval:~$
```

## Usage Examples

### Example 1: View Current Configuration
```
rt_eval:~$ rt show
═══════════════════════════════════════════════════
Current Workload: Light Load (~50%)
═══════════════════════════════════════════════════
Task1: P=100ms E=20ms D=100ms W=1 (20.0%)
Task2: P=200ms E=30ms D=200ms W=1 (15.0%)
Task3: P=300ms E=40ms D=300ms W=1 (13.3%)
Task4: P=500ms E=50ms D=500ms W=1 (10.0%)
───────────────────────────────────────────────────
Total Utilization: 58.3%
═══════════════════════════════════════════════════
```

### Example 2: Monitor Statistics
```
rt_eval:~$ rt stats
═══════════════════════════════════════════════════
Runtime Statistics
═══════════════════════════════════════════════════
Task1: Act=45 Miss=0 (0.0%) AvgRT=20ms Jitter=0.50ms
Task2: Act=22 Miss=0 (0.0%) AvgRT=30ms Jitter=0.75ms
Task3: Act=15 Miss=0 (0.0%) AvgRT=40ms Jitter=1.20ms
Task4: Act=9 Miss=0 (0.0%) AvgRT=50ms Jitter=2.30ms
═══════════════════════════════════════════════════
```

### Example 3: Change Output Format
```
rt_eval:~$ rt format json
Output format: JSON

# Now all output will be in JSON format
{"timestamp":5210,"task_id":1,"activation":45,...}
```

### Example 4: Modify Task at Runtime
```
rt_eval:~$ rt set 1 exec 40
Task1 exec_time set to 40 ms

rt_eval:~$ rt util
Task Utilization:
  Task1: 40.00% (C=40, T=100)
  Task2: 15.00% (C=30, T=200)
  Task3: 13.33% (C=40, T=300)
  Task4: 10.00% (C=50, T=500)
Total: 78.33%
Status: Schedulable
```

### Example 5: Create Overload Scenario
```
rt_eval:~$ rt set 1 exec 60
Task1 exec_time set to 60 ms

rt_eval:~$ rt set 2 exec 90
Task2 exec_time set to 90 ms

rt_eval:~$ rt util
Task Utilization:
  Task1: 60.00% (C=60, T=100)
  Task2: 45.00% (C=90, T=200)
  Task3: 13.33% (C=40, T=300)
  Task4: 10.00% (C=50, T=500)
Total: 128.33%
Status: OVERLOADED!

rt_eval:~$ rt stats
# Watch deadline misses increase...
```

## Output Format Details

### CSV Format
Enhanced with jitter column:
```
CSV_HEADER,timestamp,task_id,activation,response_time,exec_time,deadline_met,lateness,period,deadline,weight,jitter
CSV,510,1,1,20,20,1,0,100,100,1,0.00
CSV,610,1,2,20,20,1,0,100,100,1,0.00
...
```

### JSON Format
Structured data for automated analysis:
```json
{"timestamp":510,"task_id":1,"activation":1,"response_time":20,"exec_time":20,"deadline_met":true,"lateness":0,"period":100,"deadline":100,"weight":1,"jitter":0.00}
```

### Human Format
Detailed periodic summaries every 20 activations:
```
╔════════════════════════════════════════════════════════════╗
║  Task1 Summary (Task ID: 1)
╠════════════════════════════════════════════════════════════╣
║  Configuration:
║    Period:      100 ms
║    Exec Time:   20 ms (target)
║    Deadline:    100 ms
║    Weight:      1
║
║  Execution Statistics:
║    Activations: 20
║    Avg Exec:    20 ms (100.0% of target)
║    Min/Max:     20 / 21 ms
║
║  Response Time:
║    Average:     20 ms
║    Min/Max:     20 / 22 ms
║    Std Dev:     0.45 ms (jitter)
║    Variance:    0.20 ms²
║
║  Deadline Performance:
║    Misses:      0 / 20 (0.00%)
╚════════════════════════════════════════════════════════════╝
```

## Configuration Files

### prj.conf
Key settings:
- `CONFIG_SHELL=y` - Enable interactive shell
- `CONFIG_THREAD_NAME=y` - Named threads for debugging
- `CONFIG_THREAD_STACK_INFO=y` - Stack usage monitoring
- Scheduler selection (same as simple_eval_step1)

### workloads.h
Same structure as simple_eval_step1, plus:
- `WORKLOAD_CUSTOM` option for runtime-configurable workloads

## Advanced Use Cases

### 1. Testing Scheduler Robustness
```bash
# Start with light load
rt_eval:~$ rt show

# Gradually increase load
rt_eval:~$ rt set 1 exec 30
rt_eval:~$ rt set 2 exec 50
# ... monitor stats

# Push to overload
rt_eval:~$ rt set 3 exec 100
rt_eval:~$ rt stats  # Watch degradation
```

### 2. Jitter Sensitivity Analysis
```bash
# Set output to human readable
rt_eval:~$ rt format human

# Monitor jitter values in periodic summaries
# Compare different workloads
# Identify which tasks have high variance
```

### 3. Comparing Output Formats
```bash
# Collect CSV data for graphing
rt_eval:~$ rt format csv
# ... let run for a while
# Copy CSV output to file

# Switch to JSON for automated parsing
rt_eval:~$ rt format json
# ... collect data

# Human format for presentations
rt_eval:~$ rt format human
```

### 4. Performance Testing
```bash
# Quiet mode for minimal overhead
rt_eval:~$ rt format quiet

# Run performance-critical tests
# Shell still available for status checks
rt_eval:~$ rt stats
```

## Differences from simple_eval_step1

| Aspect | simple_eval_step1 | advanced_eval |
|--------|------------------|---------------|
| **Interaction** | Static, compile-time | Dynamic, runtime shell |
| **Output** | CSV only | CSV, JSON, Human, Quiet |
| **Metrics** | Basic | Enhanced with jitter |
| **Configuration** | Rebuild required | Runtime modification |
| **Debugging** | printk only | Named threads, stack info |
| **Max Activations** | 50 | 100 (configurable) |
| **Thread Names** | No | Yes |
| **Utilization Tool** | External calculation | Built-in `rt util` |
| **Stats Reset** | Restart app | `rt reset` command |

## Tips and Best Practices

### 1. Start Simple
Begin with the default light workload, familiarize yourself with shell commands before experimenting.

### 2. Use Appropriate Format
- **CSV**: For data collection and graphing (compatible with simple_eval_step1 scripts)
- **JSON**: For automated analysis and integration
- **Human**: For demos, presentations, and understanding behavior
- **Quiet**: For performance-critical testing

### 3. Monitor Utilization
Before making changes, check utilization:
```bash
rt_eval:~$ rt util
```

### 4. Reset Between Tests
When changing parameters significantly:
```bash
rt_eval:~$ rt reset
```

### 5. Watch for Overflow
When increasing load, watch for:
- Deadline misses increasing
- Jitter growing
- Response times diverging from execution times

## Automation

While advanced_eval is designed for interactive use, you can still automate tests by:

1. Pre-configuring workloads in `workloads.h`
2. Using CSV output format
3. Piping output to files
4. Using the same graphing scripts from simple_eval_step1

The CSV format is compatible (with added jitter column).

## Troubleshooting

### Shell Not Responding
- Press Enter a few times
- Type `help` to see available commands
- Check CONFIG_SHELL=y in prj.conf

### Changes Not Taking Effect
- Some parameters take effect at next period
- Reset stats to see immediate effect: `rt reset`

### High Jitter Values
- Normal for simulation environment
- Real hardware will have different characteristics
- Use as relative comparison, not absolute values

## Future Enhancements

Planned features:
- Arrival time jitter simulation
- Resource sharing scenarios
- Priority inheritance testing
- Aperiodic task support
- More detailed CPU utilization tracking

---

**See Also:**
- `README.md` - Feature overview
- `../simple_eval_step1/GUIDE.md` - Basic RT concepts
- `../simple_eval_step1/GRAPHING_GUIDE.md` - Data analysis
