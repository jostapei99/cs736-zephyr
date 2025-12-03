#!/usr/bin/env python3
"""
Zephyr RTOS Real-time Output Parser
Parses timing analysis and task performance data from Zephyr application output
"""

import re
import sys
import json
from datetime import datetime
from collections import defaultdict
import argparse

class ZephyrOutputParser:
    def __init__(self):
        self.tasks = {}
        self.timing_data = []
        self.scheduler_events = []
        self.memory_data = []
        self.emergency_events = []
        self.safety_violations = []
        self.simulation_time = 0
        
        # Regex patterns for different log types
        self.patterns = {
            'timestamp': r'\[(\d{2}:\d{2}:\d{2}\.\d{3},\d{3})\]',
            'task_completion': r'<inf> \w+: (\w+(?:\s+\w+)*) Task completed (\d+) (\w+)',
            'simulation_time': r'<inf> mission_critical: Simulation Time Elapsed: (\d+) seconds',
            'scheduler_state': r'<inf> mission_critical: Scheduler State: Current Thread (0x[0-9a-f]+), Priority (\d+)',
            'context_switch': r'<dbg> timing_analysis: Context switch: (0x[0-9a-f]+) -> (0x[0-9a-f]+) \(total: (\d+)\)',
            'task_execution': r'<inf> \w+: (\w+): ([^-]+) - Thread (0x[0-9a-f]+)',
            'emergency': r'<err> critical_tasks: EMERGENCY: (.+)',
            'safety_violation': r'<wrn> critical_tasks: Safety Monitor: Safety violation detected!',
            'timing_analysis': r'<inf> timing_analysis: === TIMING ANALYSIS REPORT ===',
            'memory_analysis': r'<inf> timing_analysis: Task (\d+): monitored \(thread ptr: (0x[0-9a-f]+)\)',
            'fault_detection': r'<inf> critical_tasks: FAULT: Detection cycle (\d+) - Thread (0x[0-9a-f]+)',
        }
    
    def parse_line(self, line):
        """Parse a single line of output"""
        # Extract timestamp
        timestamp_match = re.search(self.patterns['timestamp'], line)
        timestamp = timestamp_match.group(1) if timestamp_match else None
        
        # Parse different types of events
        if 'Task completed' in line:
            self._parse_task_completion(line, timestamp)
        elif 'Simulation Time Elapsed' in line:
            self._parse_simulation_time(line, timestamp)
        elif 'Scheduler State' in line:
            self._parse_scheduler_state(line, timestamp)
        elif 'Context switch' in line:
            self._parse_context_switch(line, timestamp)
        elif 'EMERGENCY:' in line:
            self._parse_emergency(line, timestamp)
        elif 'Safety violation detected' in line:
            self._parse_safety_violation(line, timestamp)
        elif 'TIMING ANALYSIS REPORT' in line:
            self._parse_timing_report_start(line, timestamp)
        elif re.search(self.patterns['memory_analysis'], line):
            self._parse_memory_data(line, timestamp)
        elif re.search(self.patterns['fault_detection'], line):
            self._parse_fault_detection(line, timestamp)
    
    def _parse_task_completion(self, line, timestamp):
        match = re.search(self.patterns['task_completion'], line)
        if match:
            task_name = match.group(1)
            count = int(match.group(2))
            unit = match.group(3)
            
            self.tasks[task_name] = {
                'count': count,
                'unit': unit,
                'completion_time': timestamp
            }
    
    def _parse_simulation_time(self, line, timestamp):
        match = re.search(self.patterns['simulation_time'], line)
        if match:
            self.simulation_time = int(match.group(1))
            self.timing_data.append({
                'timestamp': timestamp,
                'simulation_seconds': self.simulation_time
            })
    
    def _parse_scheduler_state(self, line, timestamp):
        match = re.search(self.patterns['scheduler_state'], line)
        if match:
            thread_id = match.group(1)
            priority = int(match.group(2))
            
            self.scheduler_events.append({
                'timestamp': timestamp,
                'current_thread': thread_id,
                'priority': priority,
                'simulation_time': self.simulation_time
            })
    
    def _parse_context_switch(self, line, timestamp):
        match = re.search(self.patterns['context_switch'], line)
        if match:
            from_thread = match.group(1)
            to_thread = match.group(2)
            total_switches = int(match.group(3))
            
            self.scheduler_events.append({
                'timestamp': timestamp,
                'type': 'context_switch',
                'from_thread': from_thread,
                'to_thread': to_thread,
                'total_switches': total_switches
            })
    
    def _parse_emergency(self, line, timestamp):
        match = re.search(self.patterns['emergency'], line)
        if match:
            message = match.group(1)
            self.emergency_events.append({
                'timestamp': timestamp,
                'message': message,
                'simulation_time': self.simulation_time
            })
    
    def _parse_safety_violation(self, line, timestamp):
        self.safety_violations.append({
            'timestamp': timestamp,
            'simulation_time': self.simulation_time
        })
    
    def _parse_timing_report_start(self, line, timestamp):
        # Mark when timing analysis reports occur
        self.timing_data.append({
            'timestamp': timestamp,
            'event': 'timing_report',
            'simulation_time': self.simulation_time
        })
    
    def _parse_memory_data(self, line, timestamp):
        match = re.search(self.patterns['memory_analysis'], line)
        if match:
            task_id = int(match.group(1))
            thread_ptr = match.group(2)
            
            self.memory_data.append({
                'timestamp': timestamp,
                'task_id': task_id,
                'thread_ptr': thread_ptr,
                'simulation_time': self.simulation_time
            })
    
    def _parse_fault_detection(self, line, timestamp):
        match = re.search(self.patterns['fault_detection'], line)
        if match:
            cycle = int(match.group(1))
            thread_id = match.group(2)
            
            # Track fault detection cycles for analysis
            if 'fault_cycles' not in self.tasks:
                self.tasks['fault_cycles'] = []
            
            self.tasks['fault_cycles'].append({
                'timestamp': timestamp,
                'cycle': cycle,
                'thread_id': thread_id,
                'simulation_time': self.simulation_time
            })
    
    def get_summary(self):
        """Generate a summary of parsed data"""
        summary = {
            'simulation_duration': self.simulation_time,
            'total_tasks': len(self.tasks),
            'task_performance': self.tasks,
            'scheduler_events_count': len(self.scheduler_events),
            'emergency_events_count': len(self.emergency_events),
            'safety_violations_count': len(self.safety_violations),
            'timing_reports_count': len([t for t in self.timing_data if 'event' in t]),
            'memory_entries_count': len(self.memory_data)
        }
        return summary
    
    def export_json(self, filename):
        """Export all parsed data to JSON"""
        data = {
            'summary': self.get_summary(),
            'tasks': self.tasks,
            'timing_data': self.timing_data,
            'scheduler_events': self.scheduler_events,
            'memory_data': self.memory_data,
            'emergency_events': self.emergency_events,
            'safety_violations': self.safety_violations
        }
        
        with open(filename, 'w') as f:
            json.dump(data, f, indent=2)
        
        print(f"Data exported to {filename}")
    
    def print_analysis(self):
        """Print analysis of the parsed data"""
        print("\n" + "="*60)
        print("ZEPHYR RTOS SIMULATION ANALYSIS")
        print("="*60)
        
        summary = self.get_summary()
        print(f"Simulation Duration: {summary['simulation_duration']} seconds")
        print(f"Total Tasks Monitored: {summary['total_tasks']}")
        print(f"Scheduler Events: {summary['scheduler_events_count']}")
        print(f"Emergency Events: {summary['emergency_events_count']}")
        print(f"Safety Violations: {summary['safety_violations_count']}")
        
        print("\nTask Performance:")
        print("-" * 40)
        for task_name, data in self.tasks.items():
            if isinstance(data, dict) and 'count' in data:
                print(f"  {task_name}: {data['count']} {data['unit']}")
        
        if self.emergency_events:
            print("\nEmergency Events:")
            print("-" * 40)
            for event in self.emergency_events:
                print(f"  [{event['timestamp']}] {event['message']}")
        
        if self.safety_violations:
            print(f"\nSafety Violations: {len(self.safety_violations)} total")
            print("-" * 40)
            for violation in self.safety_violations:
                print(f"  [{violation['timestamp']}] At simulation time {violation['simulation_time']}s")


def main():
    parser = argparse.ArgumentParser(description='Parse Zephyr RTOS output')
    parser.add_argument('input', nargs='?', help='Input file (default: stdin)')
    parser.add_argument('--output', '-o', help='Output JSON file')
    parser.add_argument('--summary', '-s', action='store_true', help='Print summary analysis')
    
    args = parser.parse_args()
    
    zephyr_parser = ZephyrOutputParser()
    
    # Read input
    if args.input:
        with open(args.input, 'r') as f:
            lines = f.readlines()
    else:
        lines = sys.stdin.readlines()
    
    # Parse each line
    for line in lines:
        zephyr_parser.parse_line(line.strip())
    
    # Output results
    if args.summary:
        zephyr_parser.print_analysis()
    
    if args.output:
        zephyr_parser.export_json(args.output)
    
    if not args.summary and not args.output:
        # Default: print summary
        zephyr_parser.print_analysis()


if __name__ == '__main__':
    main()