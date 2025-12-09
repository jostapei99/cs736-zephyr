#!/usr/bin/env python3
"""
RT Scheduler Evaluation - Graph Generation Script

Generates comprehensive graphs from CSV data collected during scheduler evaluation.
Creates plots for response times, deadline misses, utilization, and more.
"""

import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np
from pathlib import Path
import sys

# Configuration
RESULTS_DIR = Path(__file__).parent.parent / "results"
GRAPHS_DIR = RESULTS_DIR / "graphs"

SCHEDULERS = ["EDF", "WEIGHTED_EDF", "WSRT", "RMS", "LLF", "PFS"]
WORKLOADS = ["LIGHT", "MEDIUM", "HEAVY", "OVERLOAD"]
TASK_IDS = [1, 2, 3, 4]

# Color schemes
SCHEDULER_COLORS = {
    "EDF": "#1f77b4",
    "WEIGHTED_EDF": "#ff7f0e",
    "WSRT": "#2ca02c",
    "RMS": "#d62728",
    "LLF": "#9467bd",
    "PFS": "#8c564b"
}

WORKLOAD_COLORS = {
    "LIGHT": "#90EE90",
    "MEDIUM": "#FFD700",
    "HEAVY": "#FF8C00",
    "OVERLOAD": "#FF0000"
}

# Set style
sns.set_style("whitegrid")
plt.rcParams['figure.figsize'] = (12, 8)
plt.rcParams['font.size'] = 10


def load_csv_data(scheduler, workload):
    """Load CSV data for a specific scheduler and workload combination."""
    csv_file = RESULTS_DIR / f"{scheduler}_{workload}.csv"
    
    if not csv_file.exists():
        print(f"⚠ Warning: {csv_file.name} not found")
        return None
    
    try:
        # Read CSV with proper column names
        df = pd.read_csv(csv_file, header=None, names=[
            'type', 'timestamp', 'task_id', 'activation', 'response_time',
            'deadline_met', 'lateness', 'period', 'deadline', 'weight'
        ])
        
        # Add metadata columns
        df['scheduler'] = scheduler
        df['workload'] = workload
        
        return df
    except Exception as e:
        print(f"✗ Error loading {csv_file.name}: {e}")
        return None


def load_all_data():
    """Load all CSV files into a single DataFrame."""
    all_data = []
    
    for scheduler in SCHEDULERS:
        for workload in WORKLOADS:
            df = load_csv_data(scheduler, workload)
            if df is not None:
                all_data.append(df)
                print(f"✓ Loaded {scheduler}_{workload}: {len(df)} records")
    
    if not all_data:
        print("✗ No data loaded!")
        return None
    
    combined = pd.concat(all_data, ignore_index=True)
    print(f"\n✓ Total records loaded: {len(combined)}")
    return combined


def plot_response_time_by_scheduler(df):
    """Plot average response time for each scheduler across workloads."""
    fig, axes = plt.subplots(2, 2, figsize=(16, 12))
    axes = axes.flatten()
    
    for idx, task_id in enumerate(TASK_IDS):
        ax = axes[idx]
        task_data = df[df['task_id'] == task_id]
        
        # Calculate mean response time for each scheduler/workload combo
        summary = task_data.groupby(['scheduler', 'workload'])['response_time'].mean().reset_index()
        
        # Create grouped bar chart
        x = np.arange(len(WORKLOADS))
        width = 0.2
        
        for i, scheduler in enumerate(SCHEDULERS):
            sched_data = summary[summary['scheduler'] == scheduler]
            values = [sched_data[sched_data['workload'] == wl]['response_time'].values[0] 
                     if len(sched_data[sched_data['workload'] == wl]) > 0 else 0
                     for wl in WORKLOADS]
            
            ax.bar(x + i * width, values, width, label=scheduler, 
                   color=SCHEDULER_COLORS[scheduler], alpha=0.8)
        
        ax.set_xlabel('Workload')
        ax.set_ylabel('Avg Response Time (ms)')
        ax.set_title(f'Task {task_id} - Average Response Time')
        ax.set_xticks(x + width * 1.5)
        ax.set_xticklabels(WORKLOADS)
        ax.legend()
        ax.grid(axis='y', alpha=0.3)
    
    plt.tight_layout()
    output_file = GRAPHS_DIR / "response_time_by_scheduler.png"
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"✓ Saved: {output_file.name}")
    plt.close()


def plot_deadline_miss_rate(df):
    """Plot deadline miss rate for each scheduler across workloads."""
    fig, ax = plt.subplots(figsize=(14, 8))
    
    # Calculate miss rate for each scheduler/workload combo
    summary = df.groupby(['scheduler', 'workload']).agg({
        'deadline_met': lambda x: 100 * (1 - x.mean())  # Convert to miss percentage
    }).reset_index()
    summary.rename(columns={'deadline_met': 'miss_rate'}, inplace=True)
    
    # Create grouped bar chart
    x = np.arange(len(WORKLOADS))
    width = 0.2
    
    for i, scheduler in enumerate(SCHEDULERS):
        sched_data = summary[summary['scheduler'] == scheduler]
        values = [sched_data[sched_data['workload'] == wl]['miss_rate'].values[0]
                 if len(sched_data[sched_data['workload'] == wl]) > 0 else 0
                 for wl in WORKLOADS]
        
        ax.bar(x + i * width, values, width, label=scheduler,
               color=SCHEDULER_COLORS[scheduler], alpha=0.8)
    
    ax.set_xlabel('Workload', fontsize=12)
    ax.set_ylabel('Deadline Miss Rate (%)', fontsize=12)
    ax.set_title('Deadline Miss Rate by Scheduler and Workload', fontsize=14, fontweight='bold')
    ax.set_xticks(x + width * 1.5)
    ax.set_xticklabels(WORKLOADS)
    ax.legend(fontsize=11)
    ax.grid(axis='y', alpha=0.3)
    
    plt.tight_layout()
    output_file = GRAPHS_DIR / "deadline_miss_rate.png"
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"✓ Saved: {output_file.name}")
    plt.close()


def plot_response_time_distribution(df):
    """Plot response time distribution using box plots."""
    fig, axes = plt.subplots(2, 2, figsize=(16, 12))
    axes = axes.flatten()
    
    for idx, workload in enumerate(WORKLOADS):
        ax = axes[idx]
        workload_data = df[df['workload'] == workload]
        
        # Prepare data for box plot
        data_to_plot = []
        labels = []
        colors = []
        
        for scheduler in SCHEDULERS:
            sched_data = workload_data[workload_data['scheduler'] == scheduler]['response_time']
            if len(sched_data) > 0:
                data_to_plot.append(sched_data)
                labels.append(scheduler)
                colors.append(SCHEDULER_COLORS[scheduler])
        
        bp = ax.boxplot(data_to_plot, tick_labels=labels, patch_artist=True,
                        widths=0.6, showmeans=True)
        
        # Color the boxes
        for patch, color in zip(bp['boxes'], colors):
            patch.set_facecolor(color)
            patch.set_alpha(0.7)
        
        ax.set_ylabel('Response Time (ms)')
        ax.set_title(f'Response Time Distribution - {workload}', fontweight='bold')
        ax.grid(axis='y', alpha=0.3)
    
    plt.tight_layout()
    output_file = GRAPHS_DIR / "response_time_distribution.png"
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"✓ Saved: {output_file.name}")
    plt.close()


def plot_response_time_over_time(df):
    """Plot response time evolution over time for each scheduler."""
    for workload in WORKLOADS:
        fig, axes = plt.subplots(2, 2, figsize=(16, 12))
        axes = axes.flatten()
        
        workload_data = df[df['workload'] == workload]
        
        for idx, task_id in enumerate(TASK_IDS):
            ax = axes[idx]
            task_data = workload_data[workload_data['task_id'] == task_id]
            
            for scheduler in SCHEDULERS:
                sched_data = task_data[task_data['scheduler'] == scheduler].sort_values('timestamp')
                if len(sched_data) > 0:
                    ax.plot(sched_data['timestamp'] / 1000.0,  # Convert to seconds
                           sched_data['response_time'],
                           label=scheduler, color=SCHEDULER_COLORS[scheduler],
                           alpha=0.7, linewidth=1.5)
            
            ax.set_xlabel('Time (s)')
            ax.set_ylabel('Response Time (ms)')
            ax.set_title(f'Task {task_id} - Response Time Over Time')
            ax.legend()
            ax.grid(alpha=0.3)
        
        plt.suptitle(f'Response Time Evolution - {workload} Workload', 
                    fontsize=14, fontweight='bold', y=1.00)
        plt.tight_layout()
        
        output_file = GRAPHS_DIR / f"response_time_timeline_{workload}.png"
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        print(f"✓ Saved: {output_file.name}")
        plt.close()


def plot_scheduler_comparison_heatmap(df):
    """Create heatmap showing scheduler performance across workloads."""
    fig, axes = plt.subplots(1, 2, figsize=(16, 6))
    
    # Heatmap 1: Average Response Time
    ax1 = axes[0]
    summary_rt = df.groupby(['scheduler', 'workload'])['response_time'].mean().reset_index()
    pivot_rt = summary_rt.pivot(index='scheduler', columns='workload', values='response_time')
    pivot_rt = pivot_rt.reindex(SCHEDULERS)  # Ensure order
    pivot_rt = pivot_rt[WORKLOADS]  # Ensure column order
    
    sns.heatmap(pivot_rt, annot=True, fmt='.1f', cmap='YlOrRd', ax=ax1,
                cbar_kws={'label': 'Avg Response Time (ms)'})
    ax1.set_title('Average Response Time Heatmap', fontweight='bold')
    ax1.set_ylabel('Scheduler')
    ax1.set_xlabel('Workload')
    
    # Heatmap 2: Deadline Miss Rate
    ax2 = axes[1]
    summary_dm = df.groupby(['scheduler', 'workload']).agg({
        'deadline_met': lambda x: 100 * (1 - x.mean())
    }).reset_index()
    summary_dm.rename(columns={'deadline_met': 'miss_rate'}, inplace=True)
    pivot_dm = summary_dm.pivot(index='scheduler', columns='workload', values='miss_rate')
    pivot_dm = pivot_dm.reindex(SCHEDULERS)
    pivot_dm = pivot_dm[WORKLOADS]
    
    sns.heatmap(pivot_dm, annot=True, fmt='.1f', cmap='RdYlGn_r', ax=ax2,
                cbar_kws={'label': 'Miss Rate (%)'})
    ax2.set_title('Deadline Miss Rate Heatmap', fontweight='bold')
    ax2.set_ylabel('Scheduler')
    ax2.set_xlabel('Workload')
    
    plt.tight_layout()
    output_file = GRAPHS_DIR / "scheduler_comparison_heatmap.png"
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"✓ Saved: {output_file.name}")
    plt.close()


def plot_lateness_analysis(df):
    """Plot lateness distribution for missed deadlines."""
    fig, axes = plt.subplots(2, 2, figsize=(16, 12))
    axes = axes.flatten()
    
    # Filter only missed deadlines
    missed = df[df['deadline_met'] == 0]
    
    for idx, workload in enumerate(WORKLOADS):
        ax = axes[idx]
        workload_data = missed[missed['workload'] == workload]
        
        if len(workload_data) == 0:
            ax.text(0.5, 0.5, 'No Deadline Misses', ha='center', va='center',
                   fontsize=14, color='green')
            ax.set_title(f'{workload} - Lateness Distribution', fontweight='bold')
            ax.set_xlim(0, 1)
            ax.set_ylim(0, 1)
            continue
        
        # Create histogram for each scheduler
        for scheduler in SCHEDULERS:
            sched_data = workload_data[workload_data['scheduler'] == scheduler]['lateness']
            if len(sched_data) > 0:
                ax.hist(sched_data, bins=20, alpha=0.6, label=scheduler,
                       color=SCHEDULER_COLORS[scheduler], edgecolor='black')
        
        ax.set_xlabel('Lateness (ms)')
        ax.set_ylabel('Frequency')
        ax.set_title(f'{workload} - Lateness Distribution', fontweight='bold')
        ax.legend()
        ax.grid(axis='y', alpha=0.3)
    
    plt.tight_layout()
    output_file = GRAPHS_DIR / "lateness_distribution.png"
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"✓ Saved: {output_file.name}")
    plt.close()


def generate_summary_report(df):
    """Generate a text summary report."""
    report_file = GRAPHS_DIR / "summary_report.txt"
    
    with open(report_file, 'w') as f:
        f.write("=" * 70 + "\n")
        f.write("RT SCHEDULER EVALUATION - SUMMARY REPORT\n")
        f.write("=" * 70 + "\n\n")
        
        for scheduler in SCHEDULERS:
            f.write(f"\n{'─' * 70}\n")
            f.write(f"SCHEDULER: {scheduler}\n")
            f.write(f"{'─' * 70}\n\n")
            
            sched_data = df[df['scheduler'] == scheduler]
            
            for workload in WORKLOADS:
                wl_data = sched_data[sched_data['workload'] == workload]
                
                if len(wl_data) == 0:
                    continue
                
                f.write(f"  Workload: {workload}\n")
                f.write(f"  {'-' * 60}\n")
                
                # Overall statistics
                total_activations = len(wl_data)
                total_misses = (wl_data['deadline_met'] == 0).sum()
                miss_rate = 100 * total_misses / total_activations if total_activations > 0 else 0
                avg_response = wl_data['response_time'].mean()
                max_response = wl_data['response_time'].max()
                min_response = wl_data['response_time'].min()
                
                f.write(f"    Total Activations: {total_activations}\n")
                f.write(f"    Deadline Misses: {total_misses} ({miss_rate:.2f}%)\n")
                f.write(f"    Response Time (avg/min/max): {avg_response:.1f} / {min_response} / {max_response} ms\n")
                
                # Per-task breakdown
                f.write(f"    Per-Task Breakdown:\n")
                for task_id in TASK_IDS:
                    task_data = wl_data[wl_data['task_id'] == task_id]
                    if len(task_data) > 0:
                        task_misses = (task_data['deadline_met'] == 0).sum()
                        task_miss_rate = 100 * task_misses / len(task_data)
                        task_avg_rt = task_data['response_time'].mean()
                        f.write(f"      Task{task_id}: {len(task_data)} activations, "
                               f"{task_misses} misses ({task_miss_rate:.1f}%), "
                               f"avg RT={task_avg_rt:.1f}ms\n")
                
                f.write("\n")
        
        f.write("=" * 70 + "\n")
    
    print(f"✓ Saved: {report_file.name}")


def main():
    """Main execution function."""
    print("=" * 70)
    print("RT Scheduler Evaluation - Graph Generation")
    print("=" * 70)
    print()
    
    # Create graphs directory
    GRAPHS_DIR.mkdir(parents=True, exist_ok=True)
    print(f"✓ Output directory: {GRAPHS_DIR}\n")
    
    # Load data
    print("Loading CSV data...")
    df = load_all_data()
    
    if df is None or len(df) == 0:
        print("\n✗ No data available to plot!")
        print(f"  Make sure CSV files exist in: {RESULTS_DIR}")
        print(f"  Run 'bash scripts/run_all_tests.sh' first!")
        sys.exit(1)
    
    print()
    
    # Generate all graphs
    print("Generating graphs...")
    print("-" * 70)
    
    plot_response_time_by_scheduler(df)
    plot_deadline_miss_rate(df)
    plot_response_time_distribution(df)
    plot_response_time_over_time(df)
    plot_scheduler_comparison_heatmap(df)
    plot_lateness_analysis(df)
    generate_summary_report(df)
    
    print("-" * 70)
    print()
    print("=" * 70)
    print("✓ All graphs generated successfully!")
    print("=" * 70)
    print(f"Output location: {GRAPHS_DIR}")
    print()


if __name__ == "__main__":
    main()
