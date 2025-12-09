#!/usr/bin/env python3
"""
Compare scheduler performance across workloads

Reads summary.csv and generates comparison reports
"""

import sys
import csv
from pathlib import Path
from collections import defaultdict

def load_results(csv_path):
    """Load results from summary CSV"""
    results = []
    with open(csv_path, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            results.append(row)
    return results

def compare_by_workload(results):
    """Compare schedulers for each workload"""
    workloads = defaultdict(list)
    
    for result in results:
        workloads[result['workload']].append(result)
    
    print("=" * 80)
    print("COMPARISON BY WORKLOAD")
    print("=" * 80)
    print()
    
    for workload_name, workload_results in workloads.items():
        print(f"Workload: {workload_name}")
        print("-" * 80)
        print(f"{'Scheduler':<15} {'Miss Rate %':<12} {'Avg Resp (ms)':<15} {'Jitter (ms)':<12}")
        print("-" * 80)
        
        for result in sorted(workload_results, key=lambda x: float(x['miss_rate_percent'])):
            sched = result['scheduler']
            miss_rate = float(result['miss_rate_percent'])
            avg_resp = float(result['avg_response_ms'])
            jitter = float(result['jitter_ms'])
            
            print(f"{sched:<15} {miss_rate:<12.2f} {avg_resp:<15.2f} {jitter:<12.2f}")
        
        print()

def compare_by_scheduler(results):
    """Compare workloads for each scheduler"""
    schedulers = defaultdict(list)
    
    for result in results:
        schedulers[result['scheduler']].append(result)
    
    print("=" * 80)
    print("COMPARISON BY SCHEDULER")
    print("=" * 80)
    print()
    
    for sched_name, sched_results in schedulers.items():
        print(f"Scheduler: {sched_name}")
        print("-" * 80)
        print(f"{'Workload':<25} {'Miss Rate %':<12} {'Avg Resp (ms)':<15}")
        print("-" * 80)
        
        for result in sorted(sched_results, key=lambda x: x['workload']):
            workload = result['workload']
            miss_rate = float(result['miss_rate_percent'])
            avg_resp = float(result['avg_response_ms'])
            
            print(f"{workload:<25} {miss_rate:<12.2f} {avg_resp:<15.2f}")
        
        print()

def find_best_scheduler(results):
    """Find best scheduler for each metric"""
    print("=" * 80)
    print("BEST SCHEDULER BY METRIC")
    print("=" * 80)
    print()
    
    workloads = defaultdict(list)
    for result in results:
        workloads[result['workload']].append(result)
    
    for workload_name, workload_results in workloads.items():
        print(f"Workload: {workload_name}")
        
        # Lowest miss rate
        best_miss = min(workload_results, key=lambda x: float(x['miss_rate_percent']))
        print(f"  Lowest Miss Rate: {best_miss['scheduler']} ({best_miss['miss_rate_percent']}%)")
        
        # Lowest avg response time
        best_resp = min(workload_results, key=lambda x: float(x['avg_response_ms']))
        print(f"  Lowest Avg Response: {best_resp['scheduler']} ({best_resp['avg_response_ms']} ms)")
        
        # Lowest jitter
        best_jitter = min(workload_results, key=lambda x: float(x['jitter_ms']))
        print(f"  Lowest Jitter: {best_jitter['scheduler']} ({best_jitter['jitter_ms']} ms)")
        
        print()

def main():
    if len(sys.argv) > 1:
        csv_path = Path(sys.argv[1])
    else:
        # Default: look for results/summary.csv
        script_dir = Path(__file__).parent
        csv_path = script_dir.parent / "results" / "summary.csv"
    
    if not csv_path.exists():
        print(f"ERROR: Results file not found: {csv_path}")
        print("Run run_all_workloads.sh first to generate results")
        sys.exit(1)
    
    results = load_results(csv_path)
    
    if not results:
        print("ERROR: No results found in CSV")
        sys.exit(1)
    
    print()
    compare_by_workload(results)
    compare_by_scheduler(results)
    find_best_scheduler(results)
    
    print("=" * 80)
    print(f"Analysis complete. Data from: {csv_path}")
    print("=" * 80)
    print()

if __name__ == "__main__":
    main()
