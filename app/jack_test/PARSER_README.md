# Zephyr RTOS Output Parser Guide

This directory contains tools for parsing and analyzing Zephyr RTOS simulation output.

## Available Tools

### 1. `parse_output.py` - Main Parser
Comprehensive parser that extracts all data from Zephyr logs.

**Usage:**
```bash
# Parse and show summary
python3 parse_output.py logfile.log --summary

# Export to JSON
python3 parse_output.py logfile.log --output data.json

# Parse from stdin
west build -t run | python3 parse_output.py --summary
```

### 2. `monitor.py` - Real-time Monitor
Monitors Zephyr output in real-time and shows live metrics.

**Usage:**
```bash
# Real-time monitoring
west build -t run | python3 monitor.py

# Monitor saved log file
cat logfile.log | python3 monitor.py
```

### 3. `performance_analyzer.py` - Detailed Analysis
Generates detailed performance reports and statistics.

**Usage:**
```bash
python3 performance_analyzer.py logfile.log
```

### 4. `run_and_analyze.sh` - Complete Workflow
Builds, runs, and analyzes the Zephyr application automatically.

**Usage:**
```bash
./run_and_analyze.sh
```

## Quick Examples

### Capture and Parse Output
```bash
# Method 1: Direct parsing
timeout 35s west build -t run | python3 parse_output.py --summary

# Method 2: Save then parse
timeout 35s west build -t run > output.log 2>&1
python3 parse_output.py output.log --summary

# Method 3: Real-time monitoring
timeout 35s west build -t run | python3 monitor.py
```

### Extract Specific Data

**Get task completion statistics:**
```bash
python3 parse_output.py output.log --output data.json
# Then use jq or Python to query JSON:
jq '.summary.task_performance' data.json
```

**Monitor safety violations:**
```bash
grep "Safety violation" output.log | wc -l
```

**Extract timing data:**
```bash
grep "Simulation Time Elapsed" output.log
```

**Find emergency events:**
```bash
grep "EMERGENCY:" output.log
```

## Output Data Structure

The JSON export contains:
- `summary`: High-level metrics
- `tasks`: Task performance data
- `timing_data`: Simulation timing information
- `scheduler_events`: Context switches and scheduler state
- `memory_data`: Memory usage information
- `emergency_events`: Emergency system events
- `safety_violations`: Safety violation occurrences

## Common Analysis Tasks

### 1. Task Performance Analysis
```python
import json
with open('analysis.json') as f:
    data = json.load(f)

for task, info in data['summary']['task_performance'].items():
    if isinstance(info, dict) and 'count' in info:
        print(f"{task}: {info['count']} {info['unit']}")
```

### 2. Safety Analysis
```python
violations = data['safety_violations']
print(f"Total safety violations: {len(violations)}")
for v in violations:
    print(f"Violation at {v['simulation_time']}s: {v['timestamp']}")
```

### 3. Timeline Analysis
```bash
# Extract simulation progress
grep "Simulation Time Elapsed" output.log | \
    sed 's/.*Elapsed: \([0-9]*\) seconds.*/\1/' | \
    sort -n
```

## Performance Metrics

The parser extracts:
- **Task Execution Counts**: How many cycles/operations each task completed
- **Timing Information**: Simulation time progression
- **Scheduler Activity**: Context switches and thread priorities
- **Safety Events**: Violations and emergency responses
- **Resource Usage**: Memory and system resource utilization

## Integration with Other Tools

### Export to CSV for Spreadsheet Analysis
```python
import json, csv
with open('analysis.json') as f:
    data = json.load(f)

# Export scheduler events to CSV
with open('scheduler_events.csv', 'w', newline='') as csvfile:
    fieldnames = ['timestamp', 'current_thread', 'priority', 'simulation_time']
    writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
    writer.writeheader()
    for event in data['scheduler_events']:
        if 'current_thread' in event:
            writer.writerow(event)
```

### Create Graphs with Matplotlib
```python
import json, matplotlib.pyplot as plt

with open('analysis.json') as f:
    data = json.load(f)

# Plot simulation time progression
times = [t['simulation_seconds'] for t in data['timing_data'] if 'simulation_seconds' in t]
plt.plot(times)
plt.title('Simulation Time Progress')
plt.xlabel('Measurement Point')
plt.ylabel('Simulation Time (seconds)')
plt.show()
```