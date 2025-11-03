#!/usr/bin/env python3
"""
Example script showing how to use the parsed JSON data for analysis.
"""

import json
import sys
from datetime import datetime

def analyze_json_output(json_file):
    """Load and analyze parsed Zephyr output data."""
    
    with open(json_file, 'r') as f:
        data = json.load(f)
    
    print("=== Zephyr RTOS Simulation Analysis ===\n")
    
    # Summary overview
    summary = data.get('summary', {})
    print("Simulation Overview:")
    print(f"  Duration: {summary.get('simulation_duration', 'Unknown')} seconds")
    print(f"  Total Tasks: {summary.get('total_tasks', 0)}")
    
    # Count actual data
    scheduler_events = len(data.get('scheduler_events', []))
    emergency_events = len(data.get('emergency_events', []))
    safety_violations = len(data.get('safety_violations', []))
    timing_data = len(data.get('timing_data', []))
    
    print(f"  Scheduler Events: {scheduler_events}")
    print(f"  Emergency Events: {emergency_events}")
    print(f"  Safety Violations: {safety_violations}")
    print(f"  Timing Checkpoints: {timing_data}")
    print()
    
    # Task performance
    task_perf = summary.get('task_performance', {})
    if task_perf:
        print("Task Performance:")
        for task_name, metrics in task_perf.items():
            if isinstance(metrics, list):
                print(f"  {task_name}: {len(metrics)} entries")
            elif isinstance(metrics, dict) and 'count' in metrics:
                print(f"  {task_name}: {metrics['count']} {metrics['unit']}")
            else:
                print(f"  {task_name}: {metrics}")
        print()
    
    # Safety analysis
    safety_violations = data.get('safety_violations', [])
    if safety_violations:
        print("Safety Violations Timeline:")
        for i, violation in enumerate(safety_violations[:5]):  # Show first 5
            sim_time = violation.get('simulation_time', 'Unknown')
            timestamp = violation.get('timestamp', 'Unknown')
            print(f"  {i+1}. At {sim_time}s: {timestamp}")
        if len(safety_violations) > 5:
            print(f"  ... and {len(safety_violations) - 5} more violations")
        print()
    
    # Emergency events
    emergency_events = data.get('emergency_events', [])
    if emergency_events:
        print("Emergency Events:")
        for event in emergency_events:
            sim_time = event.get('simulation_time', 'Unknown')
            message = event.get('message', 'Unknown event')
            print(f"  At {sim_time}s: {message}")
        print()
    
    # Timing progression
    timing_data = data.get('timing_data', [])
    if timing_data:
        print("Simulation Timing Checkpoints:")
        for i, timing in enumerate(timing_data[:5]):  # Show first 5
            sim_seconds = timing.get('simulation_seconds', 'Unknown')
            real_time = timing.get('timestamp', 'Unknown')
            print(f"  Checkpoint {i+1}: {sim_seconds}s simulation time at {real_time}")
        print()
    
    # System health summary
    print("System Health Assessment:")
    
    # Calculate safety violation rate
    duration = summary.get('simulation_duration', 30)
    if duration > 0:
        violation_rate = len(data.get('safety_violations', [])) / duration
        print(f"  Safety violation rate: {violation_rate:.2f} violations/second")
    
    # Task efficiency
    if duration > 0:
        scheduling_rate = len(data.get('scheduler_events', [])) / duration
        print(f"  Scheduling activity: {scheduling_rate:.2f} events/second")
    
    # Emergency frequency
    emergency_count = len(data.get('emergency_events', []))
    if emergency_count > 0:
        print(f"  Emergency frequency: {emergency_count} events in {duration}s")
    else:
        print("  No emergency events detected ✓")
    
    print()

def main():
    if len(sys.argv) != 2:
        print("Usage: python3 example_analysis.py analysis.json")
        sys.exit(1)
    
    json_file = sys.argv[1]
    
    try:
        analyze_json_output(json_file)
    except FileNotFoundError:
        print(f"Error: File {json_file} not found")
        print("Run this first to generate analysis data:")
        print("  python3 parse_output.py your_log_file.log --output analysis.json")
        sys.exit(1)
    except json.JSONDecodeError:
        print(f"Error: {json_file} is not valid JSON")
        sys.exit(1)
    except Exception as e:
        print(f"Error analyzing data: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()