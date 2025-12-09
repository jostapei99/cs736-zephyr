#!/usr/bin/env python3
"""
Plotting script for scheduler evaluation results
Generates comparison plots from CSV data
"""

import sys
import csv
from pathlib import Path

# Check if matplotlib is available
try:
    import matplotlib
    matplotlib.use('Agg')  # Non-interactive backend
    import matplotlib.pyplot as plt
    import numpy as np
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("Warning: matplotlib not installed. Install with: pip install matplotlib")
    sys.exit(1)

def load_csv(csv_file):
    """Load CSV data"""
    data = []
    with open(csv_file, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            row['Threads'] = int(row['Threads'])
            row['Activations'] = int(row['Activations'])
            row['Misses'] = int(row['Misses'])
            row['MissRate'] = float(row['MissRate'])
            # Handle optional latency columns
            row['AvgResponseTime'] = int(row.get('AvgResponseTime', 0))
            row['MaxResponseTime'] = int(row.get('MaxResponseTime', 0))
            row['Jitter'] = int(row.get('Jitter', 0))
            # Handle optional kernel overhead columns
            row['ContextSwitches'] = int(row.get('ContextSwitches', 0))
            row['Preemptions'] = int(row.get('Preemptions', 0))
            row['CSPerActivation'] = float(row.get('CSPerActivation', 0))
            row['OverheadPercent'] = float(row.get('OverheadPercent', 0))
            # Handle optional normalized metrics columns
            row['Utilization'] = float(row.get('Utilization', 0))
            row['NormalizedMissRate'] = float(row.get('NormalizedMissRate', 0))
            data.append(row)
    return data

def get_unique_values(data, key):
    """Get unique sorted values for a key"""
    return sorted(list(set(row[key] for row in data)))

def plot_miss_rate_by_load(data, output_dir):
    """Plot miss rate vs load level for each thread count"""
    schedulers = get_unique_values(data, 'Scheduler')
    thread_counts = get_unique_values(data, 'Threads')
    loads = ['Light', 'Moderate', 'Heavy', 'Overload']
    
    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle('Deadline Miss Rate vs Load Level', fontsize=16, fontweight='bold')
    
    for idx, threads in enumerate(thread_counts):
        ax = axes[idx // 2, idx % 2]
        
        for sched in schedulers:
            miss_rates = []
            for load in loads:
                rows = [r for r in data if r['Scheduler'] == sched and 
                        r['Load'] == load and r['Threads'] == threads]
                if rows:
                    miss_rates.append(rows[0]['MissRate'])
                else:
                    miss_rates.append(0)
            
            ax.plot(loads, miss_rates, marker='o', label=sched, linewidth=2, markersize=8)
        
        ax.set_xlabel('Load Level', fontsize=11)
        ax.set_ylabel('Miss Rate (%)', fontsize=11)
        ax.set_title(f'{threads} Threads', fontsize=12, fontweight='bold')
        ax.legend(loc='best')
        ax.grid(True, alpha=0.3)
        ax.set_ylim(bottom=0)
    
    plt.tight_layout()
    output_file = output_dir / 'miss_rate_by_load.png'
    plt.savefig(output_file, dpi=150, bbox_inches='tight')
    plt.close()
    print(f"  Generated: {output_file.name}")

def plot_scheduler_comparison(data, output_dir):
    """Bar chart comparing average miss rates"""
    schedulers = get_unique_values(data, 'Scheduler')
    
    avg_miss_rates = []
    for sched in schedulers:
        sched_data = [r for r in data if r['Scheduler'] == sched]
        avg = sum(r['MissRate'] for r in sched_data) / len(sched_data) if sched_data else 0
        avg_miss_rates.append(avg)
    
    fig, ax = plt.subplots(figsize=(10, 6))
    bars = ax.bar(schedulers, avg_miss_rates, color='steelblue', edgecolor='black')
    
    # Add value labels on bars
    for bar, val in zip(bars, avg_miss_rates):
        height = bar.get_height()
        ax.text(bar.get_x() + bar.get_width()/2., height,
                f'{val:.1f}%', ha='center', va='bottom', fontsize=10)
    
    ax.set_xlabel('Scheduler', fontsize=12)
    ax.set_ylabel('Average Miss Rate (%)', fontsize=12)
    ax.set_title('Scheduler Performance Comparison', fontsize=14, fontweight='bold')
    ax.grid(True, axis='y', alpha=0.3)
    plt.xticks(rotation=15, ha='right')
    
    plt.tight_layout()
    output_file = output_dir / 'scheduler_comparison.png'
    plt.savefig(output_file, dpi=150, bbox_inches='tight')
    plt.close()
    print(f"  Generated: {output_file.name}")

def plot_scalability(data, output_dir):
    """Plot miss rate vs thread count"""
    schedulers = get_unique_values(data, 'Scheduler')
    thread_counts = get_unique_values(data, 'Threads')
    loads = ['Light', 'Moderate', 'Heavy', 'Overload']
    
    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle('Scalability: Miss Rate vs Thread Count', fontsize=16, fontweight='bold')
    
    for idx, load in enumerate(loads):
        ax = axes[idx // 2, idx % 2]
        
        for sched in schedulers:
            miss_rates = []
            for threads in thread_counts:
                rows = [r for r in data if r['Scheduler'] == sched and 
                        r['Load'] == load and r['Threads'] == threads]
                if rows:
                    miss_rates.append(rows[0]['MissRate'])
                else:
                    miss_rates.append(0)
            
            ax.plot(thread_counts, miss_rates, marker='s', label=sched, linewidth=2, markersize=8)
        
        ax.set_xlabel('Number of Threads', fontsize=11)
        ax.set_ylabel('Miss Rate (%)', fontsize=11)
        ax.set_title(f'{load} Load', fontsize=12, fontweight='bold')
        ax.legend(loc='best')
        ax.grid(True, alpha=0.3)
        ax.set_ylim(bottom=0)
    
    plt.tight_layout()
    output_file = output_dir / 'scalability.png'
    plt.savefig(output_file, dpi=150, bbox_inches='tight')
    plt.close()
    print(f"  Generated: {output_file.name}")

def plot_kernel_overhead(data, output_dir):
    """Plot kernel overhead comparison across schedulers"""
    schedulers = get_unique_values(data, 'Scheduler')
    
    # Calculate average overhead metrics per scheduler
    avg_overhead = []
    avg_cs_per_act = []
    
    for sched in schedulers:
        sched_data = [r for r in data if r['Scheduler'] == sched]
        avg_oh = sum(r['OverheadPercent'] for r in sched_data) / len(sched_data) if sched_data else 0
        avg_cs = sum(r['CSPerActivation'] for r in sched_data) / len(sched_data) if sched_data else 0
        avg_overhead.append(avg_oh)
        avg_cs_per_act.append(avg_cs)
    
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))
    fig.suptitle('Kernel Overhead Comparison', fontsize=16, fontweight='bold')
    
    # Scheduling overhead percentage
    bars1 = ax1.bar(schedulers, avg_overhead, color='coral', edgecolor='black')
    for bar, val in zip(bars1, avg_overhead):
        height = bar.get_height()
        ax1.text(bar.get_x() + bar.get_width()/2., height,
                f'{val:.2f}%', ha='center', va='bottom', fontsize=9)
    
    ax1.set_xlabel('Scheduler', fontsize=12)
    ax1.set_ylabel('Scheduling Overhead (%)', fontsize=12)
    ax1.set_title('Average Scheduling Overhead', fontsize=12, fontweight='bold')
    ax1.grid(True, axis='y', alpha=0.3)
    plt.setp(ax1.xaxis.get_majorticklabels(), rotation=15, ha='right')
    
    # Context switches per activation
    bars2 = ax2.bar(schedulers, avg_cs_per_act, color='lightblue', edgecolor='black')
    for bar, val in zip(bars2, avg_cs_per_act):
        height = bar.get_height()
        ax2.text(bar.get_x() + bar.get_width()/2., height,
                f'{val:.2f}', ha='center', va='bottom', fontsize=9)
    
    ax2.set_xlabel('Scheduler', fontsize=12)
    ax2.set_ylabel('Context Switches per Activation', fontsize=12)
    ax2.set_title('Scheduling Activity', fontsize=12, fontweight='bold')
    ax2.grid(True, axis='y', alpha=0.3)
    plt.setp(ax2.xaxis.get_majorticklabels(), rotation=15, ha='right')
    
    plt.tight_layout()
    output_file = output_dir / 'kernel_overhead.png'
    plt.savefig(output_file, dpi=150, bbox_inches='tight')
    plt.close()
    print(f"  Generated: {output_file.name}")

def plot_response_time_analysis(data, output_dir):
    """Plot response time and jitter analysis"""
    schedulers = get_unique_values(data, 'Scheduler')
    loads = ['Light', 'Moderate', 'Heavy', 'Overload']
    
    fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle('Response Time and Jitter Analysis', fontsize=16, fontweight='bold')
    
    # Average response time by load
    for sched in schedulers:
        avg_rt_by_load = []
        for load in loads:
            rows = [r for r in data if r['Scheduler'] == sched and r['Load'] == load]
            avg_rt = sum(r['AvgResponseTime'] for r in rows) / len(rows) if rows else 0
            avg_rt_by_load.append(avg_rt)
        ax1.plot(loads, avg_rt_by_load, marker='o', label=sched, linewidth=2, markersize=8)
    
    ax1.set_xlabel('Load Level', fontsize=11)
    ax1.set_ylabel('Avg Response Time (ms)', fontsize=11)
    ax1.set_title('Average Response Time vs Load', fontsize=12, fontweight='bold')
    ax1.legend(loc='best')
    ax1.grid(True, alpha=0.3)
    
    # Max response time by load
    for sched in schedulers:
        max_rt_by_load = []
        for load in loads:
            rows = [r for r in data if r['Scheduler'] == sched and r['Load'] == load]
            max_rt = max(r['MaxResponseTime'] for r in rows) if rows else 0
            max_rt_by_load.append(max_rt)
        ax2.plot(loads, max_rt_by_load, marker='s', label=sched, linewidth=2, markersize=8)
    
    ax2.set_xlabel('Load Level', fontsize=11)
    ax2.set_ylabel('Max Response Time (ms)', fontsize=11)
    ax2.set_title('Maximum Response Time vs Load', fontsize=12, fontweight='bold')
    ax2.legend(loc='best')
    ax2.grid(True, alpha=0.3)
    
    # Jitter by load
    for sched in schedulers:
        jitter_by_load = []
        for load in loads:
            rows = [r for r in data if r['Scheduler'] == sched and r['Load'] == load]
            avg_jitter = sum(r['Jitter'] for r in rows) / len(rows) if rows else 0
            jitter_by_load.append(avg_jitter)
        ax3.plot(loads, jitter_by_load, marker='^', label=sched, linewidth=2, markersize=8)
    
    ax3.set_xlabel('Load Level', fontsize=11)
    ax3.set_ylabel('Jitter (ms)', fontsize=11)
    ax3.set_title('Jitter vs Load', fontsize=12, fontweight='bold')
    ax3.legend(loc='best')
    ax3.grid(True, alpha=0.3)
    
    # Response time predictability (bar chart)
    avg_rt_all = []
    for sched in schedulers:
        sched_data = [r for r in data if r['Scheduler'] == sched]
        avg_rt = sum(r['AvgResponseTime'] for r in sched_data) / len(sched_data) if sched_data else 0
        avg_rt_all.append(avg_rt)
    
    bars = ax4.bar(schedulers, avg_rt_all, color='teal', edgecolor='black')
    for bar, val in zip(bars, avg_rt_all):
        height = bar.get_height()
        ax4.text(bar.get_x() + bar.get_width()/2., height,
                f'{val:.1f}', ha='center', va='bottom', fontsize=9)
    
    ax4.set_xlabel('Scheduler', fontsize=12)
    ax4.set_ylabel('Overall Avg Response Time (ms)', fontsize=12)
    ax4.set_title('Scheduler Responsiveness', fontsize=12, fontweight='bold')
    ax4.grid(True, axis='y', alpha=0.3)
    plt.setp(ax4.xaxis.get_majorticklabels(), rotation=15, ha='right')
    
    plt.tight_layout()
    output_file = output_dir / 'response_time_analysis.png'
    plt.savefig(output_file, dpi=150, bbox_inches='tight')
    plt.close()
    print(f"  Generated: {output_file.name}")

def plot_utilization_analysis(data, output_dir):
    """Plot utilization distribution across scenarios"""
    loads = ['Light', 'Moderate', 'Heavy', 'Overload']
    thread_counts = get_unique_values(data, 'Threads')
    
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))
    fig.suptitle('Workload Utilization Analysis', fontsize=16, fontweight='bold')
    
    # Utilization heatmap by threads and load
    utilization_matrix = []
    for threads in thread_counts:
        row = []
        for load in loads:
            rows = [r for r in data if r['Threads'] == threads and r['Load'] == load]
            avg_util = sum(r['Utilization'] for r in rows) / len(rows) if rows else 0
            row.append(avg_util * 100)  # Convert to percentage
        utilization_matrix.append(row)
    
    im = ax1.imshow(utilization_matrix, cmap='YlOrRd', aspect='auto')
    ax1.set_xticks(range(len(loads)))
    ax1.set_yticks(range(len(thread_counts)))
    ax1.set_xticklabels(loads)
    ax1.set_yticklabels(thread_counts)
    ax1.set_xlabel('Load Level', fontsize=12)
    ax1.set_ylabel('Thread Count', fontsize=12)
    ax1.set_title('Theoretical Utilization (%)', fontsize=12, fontweight='bold')
    
    # Add text annotations
    for i in range(len(thread_counts)):
        for j in range(len(loads)):
            text = ax1.text(j, i, f'{utilization_matrix[i][j]:.0f}%',
                           ha="center", va="center", color="black", fontsize=10)
    
    cbar = plt.colorbar(im, ax=ax1)
    cbar.set_label('Utilization (%)', rotation=270, labelpad=15)
    
    # Add 100% threshold line indicator
    ax1.axhline(y=-0.5, color='blue', linestyle='--', linewidth=2, alpha=0.7)
    ax1.text(len(loads)-0.5, -0.7, '← Schedulable (≤100%)', ha='right', color='blue', fontsize=9)
    
    # Utilization distribution histogram
    all_utils = [r['Utilization'] * 100 for r in data]
    ax2.hist(all_utils, bins=20, color='steelblue', edgecolor='black', alpha=0.7)
    ax2.axvline(x=100, color='red', linestyle='--', linewidth=2, label='100% (Schedulability threshold)')
    ax2.set_xlabel('Utilization (%)', fontsize=12)
    ax2.set_ylabel('Number of Scenarios', fontsize=12)
    ax2.set_title('Utilization Distribution', fontsize=12, fontweight='bold')
    ax2.legend()
    ax2.grid(True, alpha=0.3, axis='y')
    
    # Add statistics
    schedulable = len([u for u in all_utils if u <= 100])
    overloaded = len([u for u in all_utils if u > 100])
    ax2.text(0.02, 0.98, f'Schedulable: {schedulable}\nOverloaded: {overloaded}',
             transform=ax2.transAxes, verticalalignment='top',
             bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5))
    
    plt.tight_layout()
    output_file = output_dir / 'utilization_analysis.png'
    plt.savefig(output_file, dpi=150, bbox_inches='tight')
    plt.close()
    print(f"  Generated: {output_file.name}")

def plot_normalized_miss_rate(data, output_dir):
    """Plot normalized miss rate comparison (efficiency metric)"""
    schedulers = get_unique_values(data, 'Scheduler')
    
    # Calculate average normalized miss rate per scheduler
    avg_normalized = []
    for sched in schedulers:
        sched_data = [r for r in data if r['Scheduler'] == sched]
        avg = sum(r['NormalizedMissRate'] for r in sched_data) / len(sched_data) if sched_data else 0
        avg_normalized.append(avg)
    
    fig, ax = plt.subplots(figsize=(10, 6))
    
    # Sort schedulers by normalized miss rate for better visualization
    sorted_data = sorted(zip(schedulers, avg_normalized), key=lambda x: x[1])
    sorted_scheds = [x[0] for x in sorted_data]
    sorted_norms = [x[1] for x in sorted_data]
    
    # Color code based on efficiency
    colors = []
    for val in sorted_norms:
        if val < 10:
            colors.append('darkgreen')  # Excellent
        elif val < 25:
            colors.append('green')  # Good
        elif val < 50:
            colors.append('orange')  # Acceptable
        else:
            colors.append('red')  # Poor
    
    bars = ax.bar(sorted_scheds, sorted_norms, color=colors, edgecolor='black', alpha=0.8)
    
    # Add value labels on bars
    for bar, val in zip(bars, sorted_norms):
        height = bar.get_height()
        ax.text(bar.get_x() + bar.get_width()/2., height,
                f'{val:.1f}', ha='center', va='bottom', fontsize=10, fontweight='bold')
    
    ax.set_xlabel('Scheduler', fontsize=12)
    ax.set_ylabel('Normalized Miss Rate (Miss% / Util)', fontsize=12)
    ax.set_title('Scheduler Efficiency Comparison\n(Lower is Better - Shows Efficiency Independent of Load)', 
                 fontsize=14, fontweight='bold')
    ax.grid(True, axis='y', alpha=0.3)
    plt.xticks(rotation=15, ha='right')
    
    # Add efficiency zones
    ax.axhline(y=10, color='green', linestyle='--', linewidth=1, alpha=0.5)
    ax.axhline(y=25, color='orange', linestyle='--', linewidth=1, alpha=0.5)
    ax.axhline(y=50, color='red', linestyle='--', linewidth=1, alpha=0.5)
    
    ax.text(len(sorted_scheds)-0.5, 10, 'Excellent (<10)', ha='right', va='bottom', fontsize=8, color='green')
    ax.text(len(sorted_scheds)-0.5, 25, 'Good (<25)', ha='right', va='bottom', fontsize=8, color='orange')
    ax.text(len(sorted_scheds)-0.5, 50, 'Acceptable (<50)', ha='right', va='bottom', fontsize=8, color='red')
    
    plt.tight_layout()
    output_file = output_dir / 'normalized_miss_rate.png'
    plt.savefig(output_file, dpi=150, bbox_inches='tight')
    plt.close()
    print(f"  Generated: {output_file.name}")

def plot_efficiency_vs_load(data, output_dir):
    """Plot normalized miss rate vs load level"""
    schedulers = get_unique_values(data, 'Scheduler')
    loads = ['Light', 'Moderate', 'Heavy', 'Overload']
    
    fig, ax = plt.subplots(figsize=(12, 6))
    
    for sched in schedulers:
        normalized_by_load = []
        for load in loads:
            rows = [r for r in data if r['Scheduler'] == sched and r['Load'] == load]
            avg_norm = sum(r['NormalizedMissRate'] for r in rows) / len(rows) if rows else 0
            normalized_by_load.append(avg_norm)
        
        ax.plot(loads, normalized_by_load, marker='o', label=sched, linewidth=2.5, markersize=10)
    
    ax.set_xlabel('Load Level', fontsize=12)
    ax.set_ylabel('Normalized Miss Rate (Efficiency)', fontsize=12)
    ax.set_title('Scheduler Efficiency Across Load Levels\n(Lower = More Efficient per Unit of Utilization)', 
                 fontsize=14, fontweight='bold')
    ax.legend(loc='best', fontsize=10)
    ax.grid(True, alpha=0.3)
    ax.set_ylim(bottom=0)
    
    plt.tight_layout()
    output_file = output_dir / 'efficiency_vs_load.png'
    plt.savefig(output_file, dpi=150, bbox_inches='tight')
    plt.close()
    print(f"  Generated: {output_file.name}")

def plot_raw_vs_normalized(data, output_dir):
    """Scatter plot showing raw miss rate vs normalized miss rate"""
    schedulers = get_unique_values(data, 'Scheduler')
    
    fig, ax = plt.subplots(figsize=(10, 8))
    
    # Color map for schedulers
    colors_map = plt.cm.tab10(range(len(schedulers)))
    
    for idx, sched in enumerate(schedulers):
        sched_data = [r for r in data if r['Scheduler'] == sched]
        raw_miss = [r['MissRate'] for r in sched_data]
        normalized = [r['NormalizedMissRate'] for r in sched_data]
        
        ax.scatter(raw_miss, normalized, label=sched, alpha=0.6, s=100, 
                  color=colors_map[idx], edgecolors='black', linewidth=1)
    
    ax.set_xlabel('Raw Miss Rate (%)', fontsize=12)
    ax.set_ylabel('Normalized Miss Rate (Miss% / Util)', fontsize=12)
    ax.set_title('Raw vs Normalized Miss Rate\n(How Workload Difficulty Affects Performance)', 
                 fontsize=14, fontweight='bold')
    ax.legend(loc='best', fontsize=9)
    ax.grid(True, alpha=0.3)
    
    # Add diagonal reference lines
    ax.plot([0, 100], [0, 100], 'k--', alpha=0.3, linewidth=1, label='1:1 (100% util)')
    ax.plot([0, 100], [0, 50], 'k:', alpha=0.3, linewidth=1, label='1:2 (200% util)')
    
    plt.tight_layout()
    output_file = output_dir / 'raw_vs_normalized.png'
    plt.savefig(output_file, dpi=150, bbox_inches='tight')
    plt.close()
    print(f"  Generated: {output_file.name}")

def generate_text_report(data, output_dir):
    """Generate text summary report"""
    schedulers = get_unique_values(data, 'Scheduler')
    
    output_file = output_dir / 'analysis.txt'
    with open(output_file, 'w') as f:
        f.write("="*80 + "\n")
        f.write("SCHEDULER EVALUATION ANALYSIS\n")
        f.write("="*80 + "\n\n")
        
        for sched in schedulers:
            sched_data = [r for r in data if r['Scheduler'] == sched]
            
            avg_miss = sum(r['MissRate'] for r in sched_data) / len(sched_data)
            max_miss = max(r['MissRate'] for r in sched_data)
            min_miss = min(r['MissRate'] for r in sched_data)
            
            # Kernel overhead metrics
            avg_overhead = sum(r['OverheadPercent'] for r in sched_data) / len(sched_data)
            avg_cs = sum(r['CSPerActivation'] for r in sched_data) / len(sched_data)
            total_cs = sum(r['ContextSwitches'] for r in sched_data)
            
            # Latency metrics
            avg_response = sum(r['AvgResponseTime'] for r in sched_data) / len(sched_data)
            max_response = max(r['MaxResponseTime'] for r in sched_data)
            avg_jitter = sum(r['Jitter'] for r in sched_data) / len(sched_data)
            max_jitter = max(r['Jitter'] for r in sched_data)
            
            # Count zero-miss scenarios
            zero_miss = len([r for r in sched_data if r['Misses'] == 0])
            
            # Normalized metrics
            avg_util = sum(r['Utilization'] for r in sched_data) / len(sched_data)
            avg_normalized = sum(r['NormalizedMissRate'] for r in sched_data) / len(sched_data)
            
            f.write(f"{sched}:\n")
            f.write("-" * 80 + "\n")
            f.write(f"  Average Miss Rate:  {avg_miss:6.2f}%\n")
            f.write(f"  Min Miss Rate:      {min_miss:6.2f}%\n")
            f.write(f"  Max Miss Rate:      {max_miss:6.2f}%\n")
            f.write(f"  Zero-miss scenarios: {zero_miss}/{len(sched_data)}\n")
            f.write(f"\n")
            f.write(f"  Normalized Performance (Efficiency):\n")
            f.write(f"    Avg utilization:        {avg_util*100:6.1f}%\n")
            f.write(f"    Normalized miss rate:   {avg_normalized:6.2f} (miss%/util)\n")
            if avg_normalized < 10:
                efficiency = "EXCELLENT - Very efficient"
            elif avg_normalized < 25:
                efficiency = "GOOD - Reasonably efficient"
            elif avg_normalized < 50:
                efficiency = "ACCEPTABLE - Moderate efficiency"
            else:
                efficiency = "POOR - Low efficiency"
            f.write(f"    Efficiency rating:      {efficiency}\n")
            f.write(f"\n")
            f.write(f"  Latency Metrics:\n")
            f.write(f"    Avg response time:      {avg_response:6.1f} ms\n")
            f.write(f"    Max response time:      {max_response:6} ms\n")
            f.write(f"    Avg jitter:             {avg_jitter:6.1f} ms\n")
            f.write(f"    Max jitter:             {max_jitter:6} ms\n")
            f.write(f"\n")
            f.write(f"  Kernel Overhead:\n")
            f.write(f"    Scheduling overhead:    {avg_overhead:6.2f}%\n")
            f.write(f"    CS per activation:      {avg_cs:6.2f}\n")
            f.write(f"    Total context switches: {total_cs}\n")
            
            # Rating
            if avg_miss < 1.0:
                rating = "★★★★★ EXCELLENT"
            elif avg_miss < 5.0:
                rating = "★★★★☆ GOOD"
            elif avg_miss < 15.0:
                rating = "★★★☆☆ ACCEPTABLE"
            elif avg_miss < 30.0:
                rating = "★★☆☆☆ POOR"
            else:
                rating = "★☆☆☆☆ INADEQUATE"
            
            f.write(f"  Rating:             {rating}\n")
            f.write("\n")
        
        f.write("="*80 + "\n")
        f.write("END OF ANALYSIS\n")
        f.write("="*80 + "\n")
    
    print(f"  Generated: {output_file.name}")

def main():
    if len(sys.argv) < 3:
        print("Usage: python3 plot_results.py <csv_file> <output_dir>")
        sys.exit(1)
    
    csv_file = Path(sys.argv[1])
    output_dir = Path(sys.argv[2])
    
    if not csv_file.exists():
        print(f"Error: CSV file not found: {csv_file}")
        sys.exit(1)
    
    output_dir.mkdir(parents=True, exist_ok=True)
    
    print(f"Loading data from {csv_file.name}...")
    data = load_csv(csv_file)
    print(f"Loaded {len(data)} test results\n")
    
    print("Generating plots...")
    plot_miss_rate_by_load(data, output_dir)
    plot_scheduler_comparison(data, output_dir)
    plot_scalability(data, output_dir)
    plot_kernel_overhead(data, output_dir)
    plot_response_time_analysis(data, output_dir)
    
    # New normalized metric plots
    print("Generating normalized metric plots...")
    plot_utilization_analysis(data, output_dir)
    plot_normalized_miss_rate(data, output_dir)
    plot_efficiency_vs_load(data, output_dir)
    plot_raw_vs_normalized(data, output_dir)
    
    generate_text_report(data, output_dir)
    
    print(f"\n✓ Analysis complete! Results in: {output_dir}")

if __name__ == "__main__":
    main()
