#!/usr/bin/env python3
"""
Simple Eval Step 2 - Graph Generation Script with Dynamic Weighting

Generates comprehensive graphs from CSV data including dynamic weighting comparison.
"""

import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np
from pathlib import Path
import sys
import glob

# Configuration
def find_results_dir():
    """Find the results directory containing CSV files."""
    base_results = Path(__file__).parent.parent / "results"
    
    # If command line argument provided, use that
    if len(sys.argv) > 1:
        results_dir = Path(sys.argv[1])
        if results_dir.exists():
            return results_dir
    
    # Look for timestamped run directories
    run_dirs = sorted(base_results.glob("run_*"), reverse=True)
    if run_dirs:
        # Use most recent run directory
        return run_dirs[0]
    
    # Fall back to base results directory
    return base_results

RESULTS_DIR = find_results_dir()
GRAPHS_DIR = Path(__file__).parent.parent / "results" / "graphs"

SCHEDULERS = ["EDF", "WEIGHTED_EDF", "WSRT", "RMS"]
WORKLOADS = ["LIGHT", "MEDIUM", "HEAVY", "OVERLOAD"]
DW_OPTIONS = ["OFF", "ON"]
TASK_IDS = [1, 2, 3, 4]

# Color schemes
SCHEDULER_COLORS = {
    "EDF": "#1f77b4",
    "WEIGHTED_EDF": "#ff7f0e",
    "WSRT": "#2ca02c",
    "RMS": "#d62728"
}

DW_COLORS = {
    "OFF": "#3498db",
    "ON": "#e74c3c"
}

# Set style
sns.set_style("whitegrid")
plt.rcParams['figure.figsize'] = (12, 8)
plt.rcParams['font.size'] = 10


def load_csv_data(scheduler, workload, dw):
    """Load CSV data for a specific scheduler, workload, and DW configuration."""
    csv_file = RESULTS_DIR / f"{scheduler}_{workload}_DW{dw}.csv"
    
    if not csv_file.exists():
        print(f"⚠ Warning: {csv_file.name} not found")
        return None
    
    try:
        # CSV format: type,timestamp,task_id,activation,response_time,deadline_met,lateness,period,deadline,weight
        df = pd.read_csv(csv_file, header=None, names=[
            'type', 'timestamp', 'task_id', 'activation', 'response_time',
            'deadline_met', 'lateness', 'period', 'deadline', 'weight'
        ])
        
        # Convert response_time from microseconds to milliseconds
        df['response_time'] = df['response_time'] / 1000.0
        
        # Add metadata columns
        df['scheduler'] = scheduler
        df['workload'] = workload
        df['dynamic_weighting'] = dw
        
        return df
    except Exception as e:
        print(f"✗ Error loading {csv_file.name}: {e}")
        return None


def load_all_data():
    """Load all CSV files into a single DataFrame."""
    all_data = []
    
    for scheduler in SCHEDULERS:
        for workload in WORKLOADS:
            for dw in DW_OPTIONS:
                df = load_csv_data(scheduler, workload, dw)
                if df is not None:
                    all_data.append(df)
                    print(f"✓ Loaded {scheduler}_{workload}_DW{dw}: {len(df)} records")
    
    if not all_data:
        print("✗ No data loaded!")
        return None
    
    combined = pd.concat(all_data, ignore_index=True)
    print(f"\n✓ Total records loaded: {len(combined)}")
    return combined


def plot_dw_impact_on_deadline_misses(df):
    """Compare deadline miss rates with dynamic weighting ON vs OFF."""
    fig, axes = plt.subplots(2, 2, figsize=(16, 12))
    axes = axes.flatten()
    
    for idx, scheduler in enumerate(SCHEDULERS):
        ax = axes[idx]
        sched_data = df[df['scheduler'] == scheduler]
        
        summary = sched_data.groupby(['workload', 'dynamic_weighting']).agg({
            'deadline_met': lambda x: 100 * (1 - x.mean())
        }).reset_index()
        summary.rename(columns={'deadline_met': 'miss_rate'}, inplace=True)
        
        x = np.arange(len(WORKLOADS))
        width = 0.35
        
        off_data = summary[summary['dynamic_weighting'] == 'OFF']
        on_data = summary[summary['dynamic_weighting'] == 'ON']
        
        off_values = [off_data[off_data['workload'] == wl]['miss_rate'].values[0]
                     if len(off_data[off_data['workload'] == wl]) > 0 else 0
                     for wl in WORKLOADS]
        
        on_values = [on_data[on_data['workload'] == wl]['miss_rate'].values[0]
                    if len(on_data[on_data['workload'] == wl]) > 0 else 0
                    for wl in WORKLOADS]
        
        ax.bar(x - width/2, off_values, width, label='DW OFF', 
               color=DW_COLORS['OFF'], alpha=0.8)
        ax.bar(x + width/2, on_values, width, label='DW ON', 
               color=DW_COLORS['ON'], alpha=0.8)
        
        ax.set_xlabel('Workload')
        ax.set_ylabel('Deadline Miss Rate (%)')
        ax.set_title(f'{scheduler} - Dynamic Weighting Impact', fontweight='bold')
        ax.set_xticks(x)
        ax.set_xticklabels(WORKLOADS)
        ax.legend()
        ax.grid(axis='y', alpha=0.3)
    
    plt.tight_layout()
    output_file = GRAPHS_DIR / "dw_deadline_miss_comparison.png"
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"✓ Saved: {output_file.name}")
    plt.close()


def plot_dw_impact_on_response_time(df):
    """Compare average response times with dynamic weighting ON vs OFF."""
    fig, axes = plt.subplots(2, 2, figsize=(16, 12))
    axes = axes.flatten()
    
    for idx, scheduler in enumerate(SCHEDULERS):
        ax = axes[idx]
        sched_data = df[df['scheduler'] == scheduler]
        
        summary = sched_data.groupby(['workload', 'dynamic_weighting'])['response_time'].mean().reset_index()
        
        x = np.arange(len(WORKLOADS))
        width = 0.35
        
        off_data = summary[summary['dynamic_weighting'] == 'OFF']
        on_data = summary[summary['dynamic_weighting'] == 'ON']
        
        off_values = [off_data[off_data['workload'] == wl]['response_time'].values[0]
                     if len(off_data[off_data['workload'] == wl]) > 0 else 0
                     for wl in WORKLOADS]
        
        on_values = [on_data[on_data['workload'] == wl]['response_time'].values[0]
                    if len(on_data[on_data['workload'] == wl]) > 0 else 0
                    for wl in WORKLOADS]
        
        ax.bar(x - width/2, off_values, width, label='DW OFF', 
               color=DW_COLORS['OFF'], alpha=0.8)
        ax.bar(x + width/2, on_values, width, label='DW ON', 
               color=DW_COLORS['ON'], alpha=0.8)
        
        ax.set_xlabel('Workload')
        ax.set_ylabel('Avg Response Time (ms)')
        ax.set_title(f'{scheduler} - Response Time Comparison', fontweight='bold')
        ax.set_xticks(x)
        ax.set_xticklabels(WORKLOADS)
        ax.legend()
        ax.grid(axis='y', alpha=0.3)
    
    plt.tight_layout()
    output_file = GRAPHS_DIR / "dw_response_time_comparison.png"
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"✓ Saved: {output_file.name}")
    plt.close()


def plot_weight_evolution(df):
    """Plot how task weights evolve over time with dynamic weighting ON."""
    dw_on_data = df[df['dynamic_weighting'] == 'ON']
    
    for scheduler in SCHEDULERS:
        for workload in WORKLOADS:
            fig, axes = plt.subplots(2, 2, figsize=(16, 12))
            axes = axes.flatten()
            
            subset = dw_on_data[(dw_on_data['scheduler'] == scheduler) & 
                               (dw_on_data['workload'] == workload)]
            
            if len(subset) == 0:
                plt.close()
                continue
            
            for idx, task_id in enumerate(TASK_IDS):
                ax = axes[idx]
                task_data = subset[subset['task_id'] == task_id].sort_values('timestamp')
                
                if len(task_data) > 0:
                    ax.plot(task_data['timestamp'] / 1000.0, task_data['weight'],
                           marker='o', markersize=3, linewidth=1.5, alpha=0.7,
                           color=SCHEDULER_COLORS.get(scheduler, '#333'))
                    
                    ax.set_xlabel('Time (s)')
                    ax.set_ylabel('Weight')
                    ax.set_title(f'Task {task_id} - Weight Evolution')
                    ax.grid(alpha=0.3)
            
            plt.suptitle(f'{scheduler} - {workload} - Dynamic Weight Evolution', 
                        fontsize=14, fontweight='bold', y=1.00)
            plt.tight_layout()
            
            output_file = GRAPHS_DIR / f"weight_evolution_{scheduler}_{workload}.png"
            plt.savefig(output_file, dpi=300, bbox_inches='tight')
            print(f"✓ Saved: {output_file.name}")
            plt.close()


def plot_scheduler_comparison_heatmap(df):
    """Create heatmaps showing scheduler performance with DW ON vs OFF."""
    fig, axes = plt.subplots(2, 2, figsize=(20, 16))
    
    for dw_idx, dw in enumerate(DW_OPTIONS):
        dw_data = df[df['dynamic_weighting'] == dw]
        
        # Heatmap 1: Average Response Time
        ax1 = axes[dw_idx, 0]
        summary_rt = dw_data.groupby(['scheduler', 'workload'])['response_time'].mean().reset_index()
        pivot_rt = summary_rt.pivot(index='scheduler', columns='workload', values='response_time')
        pivot_rt = pivot_rt.reindex(SCHEDULERS)
        pivot_rt = pivot_rt[WORKLOADS]
        
        sns.heatmap(pivot_rt, annot=True, fmt='.1f', cmap='YlOrRd', ax=ax1,
                    cbar_kws={'label': 'Avg Response Time (ms)'})
        ax1.set_title(f'Avg Response Time - DW {dw}', fontweight='bold', fontsize=12)
        ax1.set_ylabel('Scheduler')
        ax1.set_xlabel('Workload')
        
        # Heatmap 2: Deadline Miss Rate
        ax2 = axes[dw_idx, 1]
        summary_dm = dw_data.groupby(['scheduler', 'workload']).agg({
            'deadline_met': lambda x: 100 * (1 - x.mean())
        }).reset_index()
        summary_dm.rename(columns={'deadline_met': 'miss_rate'}, inplace=True)
        pivot_dm = summary_dm.pivot(index='scheduler', columns='workload', values='miss_rate')
        pivot_dm = pivot_dm.reindex(SCHEDULERS)
        pivot_dm = pivot_dm[WORKLOADS]
        
        sns.heatmap(pivot_dm, annot=True, fmt='.1f', cmap='RdYlGn_r', ax=ax2,
                    cbar_kws={'label': 'Miss Rate (%)'})
        ax2.set_title(f'Deadline Miss Rate - DW {dw}', fontweight='bold', fontsize=12)
        ax2.set_ylabel('Scheduler')
        ax2.set_xlabel('Workload')
    
    plt.tight_layout()
    output_file = GRAPHS_DIR / "scheduler_comparison_heatmap.png"
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"✓ Saved: {output_file.name}")
    plt.close()


def plot_response_time_by_scheduler(df):
    """Plot average response time for each scheduler across workloads (DW OFF vs ON)."""
    for task_id in TASK_IDS:
        fig, axes = plt.subplots(2, 2, figsize=(16, 12))
        axes = axes.flatten()
        
        task_data = df[df['task_id'] == task_id]
        
        for idx, scheduler in enumerate(SCHEDULERS):
            ax = axes[idx]
            sched_data = task_data[task_data['scheduler'] == scheduler]
            
            summary = sched_data.groupby(['workload', 'dynamic_weighting'])['response_time'].mean().reset_index()
            
            x = np.arange(len(WORKLOADS))
            width = 0.35
            
            off_data = summary[summary['dynamic_weighting'] == 'OFF']
            on_data = summary[summary['dynamic_weighting'] == 'ON']
            
            off_values = [off_data[off_data['workload'] == wl]['response_time'].values[0]
                         if len(off_data[off_data['workload'] == wl]) > 0 else 0
                         for wl in WORKLOADS]
            
            on_values = [on_data[on_data['workload'] == wl]['response_time'].values[0]
                        if len(on_data[on_data['workload'] == wl]) > 0 else 0
                        for wl in WORKLOADS]
            
            ax.bar(x - width/2, off_values, width, label='DW OFF', 
                   color=DW_COLORS['OFF'], alpha=0.8)
            ax.bar(x + width/2, on_values, width, label='DW ON', 
                   color=DW_COLORS['ON'], alpha=0.8)
            
            ax.set_xlabel('Workload')
            ax.set_ylabel('Avg Response Time (ms)')
            ax.set_title(f'{scheduler}', fontweight='bold')
            ax.set_xticks(x)
            ax.set_xticklabels(WORKLOADS)
            ax.legend()
            ax.grid(axis='y', alpha=0.3)
        
        plt.suptitle(f'Task {task_id} - Average Response Time Comparison', 
                    fontsize=14, fontweight='bold', y=1.00)
        plt.tight_layout()
        output_file = GRAPHS_DIR / f"response_time_task{task_id}.png"
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        print(f"✓ Saved: {output_file.name}")
        plt.close()


def plot_dw_improvement_summary(df):
    """Plot percentage improvement from enabling dynamic weighting."""
    fig, axes = plt.subplots(1, 2, figsize=(16, 6))
    
    # Calculate improvements for deadline miss rate
    ax1 = axes[0]
    improvements_dm = []
    labels = []
    
    for scheduler in SCHEDULERS:
        for workload in WORKLOADS:
            off_data = df[(df['scheduler'] == scheduler) & 
                         (df['workload'] == workload) & 
                         (df['dynamic_weighting'] == 'OFF')]
            on_data = df[(df['scheduler'] == scheduler) & 
                        (df['workload'] == workload) & 
                        (df['dynamic_weighting'] == 'ON')]
            
            if len(off_data) > 0 and len(on_data) > 0:
                off_miss_rate = 100 * (1 - off_data['deadline_met'].mean())
                on_miss_rate = 100 * (1 - on_data['deadline_met'].mean())
                
                if off_miss_rate > 0:
                    improvement = ((off_miss_rate - on_miss_rate) / off_miss_rate) * 100
                    improvements_dm.append(improvement)
                    labels.append(f"{scheduler}\n{workload}")
    
    x = np.arange(len(improvements_dm))
    colors = ['green' if imp > 0 else 'red' for imp in improvements_dm]
    ax1.bar(x, improvements_dm, color=colors, alpha=0.7)
    ax1.axhline(y=0, color='black', linestyle='-', linewidth=0.5)
    ax1.set_ylabel('Improvement (%)')
    ax1.set_title('Deadline Miss Rate Improvement with DW ON', fontweight='bold')
    ax1.set_xticks(x)
    ax1.set_xticklabels(labels, rotation=45, ha='right', fontsize=8)
    ax1.grid(axis='y', alpha=0.3)
    
    # Calculate improvements for response time
    ax2 = axes[1]
    improvements_rt = []
    labels_rt = []
    
    for scheduler in SCHEDULERS:
        for workload in WORKLOADS:
            off_data = df[(df['scheduler'] == scheduler) & 
                         (df['workload'] == workload) & 
                         (df['dynamic_weighting'] == 'OFF')]
            on_data = df[(df['scheduler'] == scheduler) & 
                        (df['workload'] == workload) & 
                        (df['dynamic_weighting'] == 'ON')]
            
            if len(off_data) > 0 and len(on_data) > 0:
                off_rt = off_data['response_time'].mean()
                on_rt = on_data['response_time'].mean()
                
                if off_rt > 0:
                    improvement = ((off_rt - on_rt) / off_rt) * 100
                    improvements_rt.append(improvement)
                    labels_rt.append(f"{scheduler}\n{workload}")
    
    x = np.arange(len(improvements_rt))
    colors = ['green' if imp > 0 else 'red' for imp in improvements_rt]
    ax2.bar(x, improvements_rt, color=colors, alpha=0.7)
    ax2.axhline(y=0, color='black', linestyle='-', linewidth=0.5)
    ax2.set_ylabel('Improvement (%)')
    ax2.set_title('Response Time Improvement with DW ON', fontweight='bold')
    ax2.set_xticks(x)
    ax2.set_xticklabels(labels_rt, rotation=45, ha='right', fontsize=8)
    ax2.grid(axis='y', alpha=0.3)
    
    plt.tight_layout()
    output_file = GRAPHS_DIR / "dw_improvement_summary.png"
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"✓ Saved: {output_file.name}")
    plt.close()


def generate_summary_report(df):
    """Generate a text summary report."""
    report_file = GRAPHS_DIR / "summary_report.txt"
    
    with open(report_file, 'w') as f:
        f.write("=" * 80 + "\n")
        f.write("SIMPLE EVAL STEP 2 - DYNAMIC WEIGHTING EVALUATION SUMMARY\n")
        f.write("=" * 80 + "\n\n")
        
        for scheduler in SCHEDULERS:
            f.write(f"\n{'─' * 80}\n")
            f.write(f"SCHEDULER: {scheduler}\n")
            f.write(f"{'─' * 80}\n\n")
            
            sched_data = df[df['scheduler'] == scheduler]
            
            for workload in WORKLOADS:
                wl_data = sched_data[sched_data['workload'] == workload]
                
                if len(wl_data) == 0:
                    continue
                
                f.write(f"  Workload: {workload}\n")
                f.write(f"  {'-' * 70}\n")
                
                for dw in DW_OPTIONS:
                    dw_data = wl_data[wl_data['dynamic_weighting'] == dw]
                    
                    if len(dw_data) == 0:
                        continue
                    
                    total_activations = len(dw_data)
                    total_misses = (dw_data['deadline_met'] == 0).sum()
                    miss_rate = 100 * total_misses / total_activations if total_activations > 0 else 0
                    avg_response = dw_data['response_time'].mean()
                    max_response = dw_data['response_time'].max()
                    min_response = dw_data['response_time'].min()
                    
                    f.write(f"    Dynamic Weighting: {dw}\n")
                    f.write(f"      Total Activations: {total_activations}\n")
                    f.write(f"      Deadline Misses: {total_misses} ({miss_rate:.2f}%)\n")
                    f.write(f"      Response Time (avg/min/max): {avg_response:.1f} / {min_response} / {max_response} ms\n")
                    
                    # Per-task breakdown
                    f.write(f"      Per-Task:\n")
                    for task_id in TASK_IDS:
                        task_data = dw_data[dw_data['task_id'] == task_id]
                        if len(task_data) > 0:
                            task_misses = (task_data['deadline_met'] == 0).sum()
                            task_miss_rate = 100 * task_misses / len(task_data)
                            task_avg_rt = task_data['response_time'].mean()
                            avg_weight = task_data['weight'].mean()
                            f.write(f"        Task{task_id}: {len(task_data)} acts, "
                                   f"{task_misses} misses ({task_miss_rate:.1f}%), "
                                   f"avg RT={task_avg_rt:.1f}ms, avg weight={avg_weight:.2f}\n")
                    
                    f.write("\n")
                
                f.write("\n")
        
        f.write("=" * 80 + "\n")
    
    print(f"✓ Saved: {report_file.name}")


def main():
    """Main execution function."""
    print("=" * 80)
    print("Simple Eval Step 2 - Graph Generation with Dynamic Weighting Analysis")
    print("=" * 80)
    print()
    
    # Show which directory we're using
    print(f"✓ Results directory: {RESULTS_DIR}")
    
    # Create graphs directory
    GRAPHS_DIR.mkdir(parents=True, exist_ok=True)
    print(f"✓ Output directory: {GRAPHS_DIR}\n")
    
    # Load data
    print("Loading CSV data...")
    df = load_all_data()
    
    if df is None or len(df) == 0:
        print("\n✗ No data available to plot!")
        print(f"  Searched in: {RESULTS_DIR}")
        print(f"  Make sure CSV files exist!")
        print(f"\nUsage: python3 {sys.argv[0]} [results_directory]")
        print(f"Example: python3 {sys.argv[0]} results/run_2025-12-04_05-35-47")
        sys.exit(1)
    
    print()
    
    # Generate all graphs
    print("Generating graphs...")
    print("-" * 80)
    
    plot_dw_impact_on_deadline_misses(df)
    plot_dw_impact_on_response_time(df)
    plot_dw_improvement_summary(df)
    plot_scheduler_comparison_heatmap(df)
    plot_response_time_by_scheduler(df)
    plot_weight_evolution(df)
    
    generate_summary_report(df)
    
    print("-" * 80)
    print()
    print("=" * 80)
    print("✓ All graphs generated successfully!")
    print("=" * 80)
    print(f"Output location: {GRAPHS_DIR}")
    print()
    print("Generated graphs:")
    print("  • dw_deadline_miss_comparison.png")
    print("  • dw_response_time_comparison.png")
    print("  • dw_improvement_summary.png")
    print("  • scheduler_comparison_heatmap.png")
    print("  • response_time_task*.png (4 files)")
    print("  • weight_evolution_*.png (multiple files)")
    print("  • summary_report.txt")
    print()


if __name__ == "__main__":
    main()
