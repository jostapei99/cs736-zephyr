#!/usr/bin/env python3
"""
Real-time Zephyr Output Monitor
Monitors Zephyr output in real-time and shows key metrics
"""

import re
import sys
import time
from collections import defaultdict

class RealTimeMonitor:
    def __init__(self):
        self.task_counts = defaultdict(int)
        self.last_simulation_time = 0
        self.context_switches = 0
        self.emergency_count = 0
        self.safety_violations = 0
        
    def process_line(self, line):
        # Track simulation time
        if 'Simulation Time Elapsed' in line:
            match = re.search(r'Simulation Time Elapsed: (\d+) seconds', line)
            if match:
                self.last_simulation_time = int(match.group(1))
                self.print_status()
        
        # Track task completions
        elif 'Task completed' in line:
            if 'Mission Control' in line:
                match = re.search(r'completed (\d+) cycles', line)
                if match:
                    self.task_counts['Mission Control'] = int(match.group(1))
            elif 'Navigation' in line:
                match = re.search(r'completed (\d+) updates', line)
                if match:
                    self.task_counts['Navigation'] = int(match.group(1))
            elif 'Communication' in line:
                match = re.search(r'sent (\d+) messages', line)
                if match:
                    self.task_counts['Communication'] = int(match.group(1))
            elif 'Safety Monitor' in line:
                match = re.search(r'completed (\d+) cycles', line)
                if match:
                    self.task_counts['Safety Monitor'] = int(match.group(1))
            elif 'Fault Detection' in line:
                match = re.search(r'completed (\d+) cycles', line)
                if match:
                    self.task_counts['Fault Detection'] = int(match.group(1))
            elif 'Housekeeping' in line:
                match = re.search(r'completed (\d+) cycles', line)
                if match:
                    self.task_counts['Housekeeping'] = int(match.group(1))
            elif 'Emergency Response' in line:
                match = re.search(r'handled (\d+) emergencies', line)
                if match:
                    self.task_counts['Emergency Response'] = int(match.group(1))
        
        # Track context switches
        elif 'Context switch' in line:
            match = re.search(r'total: (\d+)', line)
            if match:
                self.context_switches = int(match.group(1))
        
        # Track emergencies
        elif 'EMERGENCY:' in line:
            self.emergency_count += 1
            print(f"\n🚨 EMERGENCY DETECTED: {line.split('EMERGENCY: ')[1].strip()}")
        
        # Track safety violations
        elif 'Safety violation detected' in line:
            self.safety_violations += 1
            print(f"\n⚠️  SAFETY VIOLATION #{self.safety_violations}")
    
    def print_status(self):
        print(f"\n{'='*60}")
        print(f"SIMULATION TIME: {self.last_simulation_time} seconds")
        print(f"{'='*60}")
        
        if self.task_counts:
            print("Task Performance:")
            for task, count in self.task_counts.items():
                print(f"  {task:<20}: {count}")
        
        print(f"\nSystem Metrics:")
        print(f"  Context Switches     : {self.context_switches}")
        print(f"  Emergency Events     : {self.emergency_count}")
        print(f"  Safety Violations    : {self.safety_violations}")
        
        if self.last_simulation_time >= 30:
            print("\n✅ SIMULATION COMPLETE!")
    
    def run(self):
        print("🔍 Real-time Zephyr Output Monitor")
        print("Waiting for input... (Ctrl+C to exit)")
        
        try:
            for line in sys.stdin:
                self.process_line(line.strip())
        except KeyboardInterrupt:
            print("\n\nMonitoring stopped.")
            self.print_status()

if __name__ == '__main__':
    monitor = RealTimeMonitor()
    monitor.run()