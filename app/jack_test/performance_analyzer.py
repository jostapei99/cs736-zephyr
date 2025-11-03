#!/usr/bin/env python3
"""
Zephyr Performance Report Generator
Analyzes Zephyr output and generates detailed performance reports
"""

import re
import json
import matplotlib.pyplot as plt
import pandas as pd
from datetime import datetime
import sys

class PerformanceAnalyzer:
    def __init__(self, log_file):
        self.log_file = log_file
        self.data = {
            'tasks': {},
            'timing': [],
            'scheduler': [],
            'resources': {}
        }
    
    def parse_logs(self):
        """Parse the entire log file"""
        with open(self.log_file, 'r') as f:
            lines = f.readlines()
        
        for line in lines:
            self._extract_task_cycles(line)
            self._extract_timing_data(line)
            self._extract_scheduler_info(line)
    
    def _extract_task_cycles(self, line):
        """Extract task execution cycles from logs"""
        patterns = {
            'mission_control': r'Mission Control: Cycle (\d+)',
            'navigation': r'Navigation: Updating position estimate (\d+)',
            'communication': r'Communication: Sending telemetry message (\d+)',
            'safety_monitor': r'Safety Monitor: Cycle (\d+)',
            'fault_detection': r'Fault Detection: Scanning cycle (\d+)',
            'housekeeping': r'Housekeeping: Performing maintenance cycle (\d+)'
        }
        
        timestamp_match = re.search(r'\[(\d{2}:\d{2}:\d{2}\.\d{3}),\d{3}\]', line)
        timestamp = timestamp_match.group(1) if timestamp_match else None
        
        for task, pattern in patterns.items():
            match = re.search(pattern, line)
            if match:
                cycle = int(match.group(1))
                if task not in self.data['tasks']:
                    self.data['tasks'][task] = []
                
                self.data['tasks'][task].append({
                    'timestamp': timestamp,
                    'cycle': cycle
                })
    
    def _extract_timing_data(self, line):
        """Extract timing analysis data"""
        if 'Simulation Time Elapsed' in line:
            match = re.search(r'Simulation Time Elapsed: (\d+) seconds', line)
            timestamp_match = re.search(r'\[(\d{2}:\d{2}:\d{2}\.\d{3}),\d{3}\]', line)
            
            if match and timestamp_match:
                self.data['timing'].append({
                    'timestamp': timestamp_match.group(1),
                    'simulation_time': int(match.group(1))
                })
    
    def _extract_scheduler_info(self, line):
        """Extract scheduler state information"""
        if 'Scheduler State' in line:
            match = re.search(r'Current Thread (0x[0-9a-f]+), Priority (\d+)', line)
            timestamp_match = re.search(r'\[(\d{2}:\d{2}:\d{2}\.\d{3}),\d{3}\]', line)
            
            if match and timestamp_match:
                self.data['scheduler'].append({
                    'timestamp': timestamp_match.group(1),
                    'thread': match.group(1),
                    'priority': int(match.group(2))
                })
    
    def generate_task_frequency_report(self):
        """Generate task execution frequency analysis"""
        print("\n" + "="*60)
        print("TASK EXECUTION FREQUENCY ANALYSIS")
        print("="*60)
        
        for task_name, cycles in self.data['tasks'].items():
            if len(cycles) < 2:
                continue
                
            # Calculate average cycle interval
            intervals = []
            for i in range(1, len(cycles)):
                time_diff = self._time_diff(cycles[i-1]['timestamp'], cycles[i]['timestamp'])
                if time_diff > 0:
                    intervals.append(time_diff)
            
            if intervals:
                avg_interval = sum(intervals) / len(intervals)
                frequency = 1.0 / avg_interval if avg_interval > 0 else 0
                
                print(f"\n{task_name.replace('_', ' ').title()}:")
                print(f"  Total Cycles: {len(cycles)}")
                print(f"  Avg Interval: {avg_interval:.2f}s")
                print(f"  Frequency: {frequency:.2f} Hz")
                print(f"  Max Cycle: {max(c['cycle'] for c in cycles)}")
    
    def _time_diff(self, time1, time2):
        """Calculate time difference in seconds"""
        try:
            # Parse timestamps like "00:50:04.004"
            def parse_time(time_str):
                parts = time_str.split(':')
                hours = int(parts[0])
                minutes = int(parts[1])
                sec_parts = parts[2].split('.')
                seconds = int(sec_parts[0])
                milliseconds = int(sec_parts[1])
                
                total_seconds = hours * 3600 + minutes * 60 + seconds + milliseconds / 1000.0
                return total_seconds
            
            t1 = parse_time(time1)
            t2 = parse_time(time2)
            
            # Handle day rollover
            if t2 < t1:
                t2 += 24 * 3600
            
            return t2 - t1
        except:
            return 0
    
    def generate_resource_utilization_report(self):
        """Generate resource utilization report"""
        print("\n" + "="*60)
        print("RESOURCE UTILIZATION ANALYSIS")
        print("="*60)
        
        # Scheduler analysis
        if self.data['scheduler']:
            priorities = [s['priority'] for s in self.data['scheduler']]
            unique_priorities = set(priorities)
            
            print(f"\nScheduler Activity:")
            print(f"  Total State Changes: {len(self.data['scheduler'])}")
            print(f"  Priority Levels Used: {len(unique_priorities)}")
            print(f"  Priority Range: {min(priorities)} - {max(priorities)}")
            
            # Priority distribution with categorization
            priority_counts = {}
            for p in priorities:
                priority_counts[p] = priority_counts.get(p, 0) + 1
            
            print(f"\nPriority Distribution:")
            for priority in sorted(priority_counts.keys()):
                count = priority_counts[priority]
                percentage = (count / len(priorities)) * 100
                
                # Categorize priority types
                if priority < 0:
                    ptype = "(COOPERATIVE)"
                elif priority <= 15:
                    ptype = "(PREEMPTIVE)"
                else:
                    ptype = "(SYSTEM/IDLE)"
                    
                print(f"  Priority {priority:2d}: {count:3d} times ({percentage:5.1f}%) {ptype}")
    
    def generate_timeline_report(self):
        """Generate timeline analysis"""
        print("\n" + "="*60)
        print("SIMULATION TIMELINE ANALYSIS")
        print("="*60)
        
        if self.data['timing']:
            total_duration = max(t['simulation_time'] for t in self.data['timing'])
            print(f"\nSimulation Duration: {total_duration} seconds")
            
            # Calculate simulation speed (virtual vs real time)
            if len(self.data['timing']) >= 2:
                first_entry = self.data['timing'][0]
                last_entry = self.data['timing'][-1]
                
                real_time_diff = self._time_diff(first_entry['timestamp'], last_entry['timestamp'])
                virtual_time_diff = last_entry['simulation_time'] - first_entry['simulation_time']
                
                if real_time_diff > 0:
                    speed_factor = virtual_time_diff / real_time_diff
                    print(f"Real Time Elapsed: {real_time_diff:.1f} seconds")
                    print(f"Virtual Time Elapsed: {virtual_time_diff} seconds")
                    print(f"Simulation Speed Factor: {speed_factor:.1f}x")
    
    def export_csv(self, filename):
        """Export data to CSV for further analysis"""
        # Create a comprehensive dataset
        rows = []
        
        for task_name, cycles in self.data['tasks'].items():
            for cycle_data in cycles:
                rows.append({
                    'timestamp': cycle_data['timestamp'],
                    'task': task_name,
                    'cycle': cycle_data['cycle'],
                    'type': 'task_cycle'
                })
        
        for timing_data in self.data['timing']:
            rows.append({
                'timestamp': timing_data['timestamp'],
                'task': 'system',
                'cycle': timing_data['simulation_time'],
                'type': 'simulation_time'
            })
        
        df = pd.DataFrame(rows)
        df.to_csv(filename, index=False)
        print(f"\nData exported to {filename}")
    
    def run_analysis(self):
        """Run complete performance analysis"""
        print("🔍 Parsing Zephyr RTOS logs...")
        self.parse_logs()
        
        print("📊 Generating performance reports...")
        self.generate_task_frequency_report()
        self.generate_resource_utilization_report()
        self.generate_timeline_report()


def main():
    if len(sys.argv) != 2:
        print("Usage: python3 performance_analyzer.py <log_file>")
        sys.exit(1)
    
    log_file = sys.argv[1]
    analyzer = PerformanceAnalyzer(log_file)
    analyzer.run_analysis()
    
    # Optionally export to CSV
    csv_file = log_file.replace('.log', '_analysis.csv').replace('.txt', '_analysis.csv')
    analyzer.export_csv(csv_file)

if __name__ == '__main__':
    main()