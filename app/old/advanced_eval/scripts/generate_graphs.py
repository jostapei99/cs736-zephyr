#!/usr/bin/env python3
"""
Advanced RT Scheduler Evaluation - Enhanced Graph Generation Script

Generates comprehensive graphs from CSV data including jitter analysis.
Enhanced version for advanced_eval application.
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
        # Read CSV with proper column names (including jitter)
        df = pd.read_csv(csv_file, header=None, names=[
            'type', 'timestamp', 'task_id', 'activation', 'response_time', 'exec_time',
            'deadline_met', 'lateness', 'period', 'deadline', 'weight', 'jitter'
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
        
        summary = task_data.groupby(['scheduler', 'workload'])['response_time'].mean().reset_index()
        
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


def plot_jitter_comparison(df):
    """Plot jitter (std dev of response time) comparison - NEW for advanced_eval."""
    fig, axes = plt.subplots(2, 2, figsize=(16, 12))
    axes = axes.flatten()
    
    for idx, task_id in enumerate(TASK_IDS):
        ax = axes[idx]
        task_data = df[df['task_id'] == task_id]
        
        # Get max jitter value for each scheduler/workload combo
        summary = task_data.groupby(['scheduler', 'workload'])['jitter'].max().reset_index()
        
        x = np.arange(len(WORKLOADS))
        width = 0.2
        
        for i, scheduler in enumerate(SCHEDULERS):
            sched_data = summary[summary['scheduler'] == scheduler]
            values = [sched_data[sched_data['workload'] == wl]['jitter'].values[0] 
                     if len(sched_data[sched_data['workload'] == wl]) > 0 else 0
                     for wl in WORKLOADS]
            
            ax.bar(x + i * width, values, width, label=scheduler, 
                   color=SCHEDULER_COLORS[scheduler], alpha=0.8)
        
        ax.set_xlabel('Workload')
        ax.set_ylabel('Max Jitter (ms)')
        ax.set_title(f'Task {task_id} - Response Time Jitter')
        ax.set_xticks(x + width * 1.5)
        ax.set_xticklabels(WORKLOADS)
        ax.legend()
        ax.grid(axis='y', alpha=0.3)
    
    plt.tight_layout()
    output_file = GRAPHS_DIR / "jitter_comparison.png"
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"✓ Saved: {output_file.name}")
    plt.close()


def plot_jitter_over_time(df):
    """Plot how jitter evolves over time - NEW for advanced_eval."""
    for workload in WORKLOADS:
        fig, axes = plt.subplots(2, 2, figsize=(16, 12))
        axes = axes.flatten()
        
        workload_data = df[df['workload'] == workload]
        
        for idx, task_id in enumerate(TASK_IDS):
            ax = axes[idx]
            task_data = workload_data[workload_data['task_id'] == task_id]
            
            for scheduler in SCHEDULERS:
                sched_data = task_data[task_data['scheduler'] == scheduler].sort_values('activation')
                if len(sched_data) > 0:
                    ax.plot(sched_data['activation'],
                           sched_data['jitter'],
                           label=scheduler, color=SCHEDULER_COLORS[scheduler],
                           alpha=0.7, linewidth=1.5, marker='o', markersize=3)
            
            ax.set_xlabel('Activation Number')
            ax.set_ylabel('Jitter (ms)')
            ax.set_title(f'Task {task_id} - Jitter Evolution')
            ax.legend()
            ax.grid(alpha=0.3)
        
        plt.suptitle(f'Jitter Evolution Over Time - {workload} Workload', 
                    fontsize=14, fontweight='bold', y=1.00)
        plt.tight_layout()
        
        output_file = GRAPHS_DIR / f"jitter_evolution_{workload}.png"
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        print(f"✓ Saved: {output_file.name}")
        plt.close()


def plot_exec_time_accuracy(df):
    """Plot execution time accuracy (actual vs target) - NEW for advanced_eval."""
    fig, axes = plt.subplots(2, 2, figsize=(16, 12))
    axes = axes.flatten()
    
    for idx, workload in enumerate(WORKLOADS):
        ax = axes[idx]
        workload_data = df[df['workload'] == workload]
        
        # Calculate average execution time deviation for each scheduler
        data_to_plot = []
        labels = []
        colors = []
        
        for scheduler in SCHEDULERS:
            sched_data = workload_data[workload_data['scheduler'] == scheduler]
            if len(sched_data) > 0:
                # Deviation from target (period-based estimate)
                deviations = sched_data['exec_time'] - (sched_data['period'] * 0.2)  # Rough estimate
                data_to_plot.append(deviations)
                labels.append(scheduler)
                colors.append(SCHEDULER_COLORS[scheduler])
        
        if data_to_plot:
            bp = ax.boxplot(data_to_plot, tick_labels=labels, patch_artist=True,
                            widths=0.6, showmeans=True)
            
            for patch, color in zip(bp['boxes'], colors):
                patch.set_facecolor(color)
                patch.set_alpha(0.7)
        
        ax.set_ylabel('Exec Time Deviation (ms)')
        ax.set_title(f'Execution Time Accuracy - {workload}', fontweight='bold')
        ax.axhline(y=0, color='red', linestyle='--', alpha=0.5, label='Target')
        ax.grid(axis='y', alpha=0.3)
        ax.legend()
    
    plt.tight_layout()
    output_file = GRAPHS_DIR / "exec_time_accuracy.png"
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"✓ Saved: {output_file.name}")
    plt.close()


def plot_deadline_miss_rate(df):
    """Plot deadline miss rate for each scheduler across workloads."""
    fig, ax = plt.subplots(figsize=(14, 8))
    
    summary = df.groupby(['scheduler', 'workload']).agg({
        'deadline_met': lambda x: 100 * (1 - x.mean())
    }).reset_index()
    summary.rename(columns={'deadline_met': 'miss_rate'}, inplace=True)
    
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
                    ax.plot(sched_data['timestamp'] / 1000.0,
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
    fig, axes = plt.subplots(1, 3, figsize=(20, 6))
    
    # Heatmap 1: Average Response Time
    ax1 = axes[0]
    summary_rt = df.groupby(['scheduler', 'workload'])['response_time'].mean().reset_index()
    pivot_rt = summary_rt.pivot(index='scheduler', columns='workload', values='response_time')
    pivot_rt = pivot_rt.reindex(SCHEDULERS)
    pivot_rt = pivot_rt[WORKLOADS]
    
    sns.heatmap(pivot_rt, annot=True, fmt='.1f', cmap='YlOrRd', ax=ax1,
                cbar_kws={'label': 'Avg Response Time (ms)'})
    ax1.set_title('Average Response Time', fontweight='bold')
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
    ax2.set_title('Deadline Miss Rate', fontweight='bold')
    ax2.set_ylabel('Scheduler')
    ax2.set_xlabel('Workload')
    
    # Heatmap 3: Max Jitter - NEW for advanced_eval
    ax3 = axes[2]
    summary_jit = df.groupby(['scheduler', 'workload'])['jitter'].max().reset_index()
    pivot_jit = summary_jit.pivot(index='scheduler', columns='workload', values='jitter')
    pivot_jit = pivot_jit.reindex(SCHEDULERS)
    pivot_jit = pivot_jit[WORKLOADS]
    
    sns.heatmap(pivot_jit, annot=True, fmt='.2f', cmap='YlOrRd', ax=ax3,
                cbar_kws={'label': 'Max Jitter (ms)'})
    ax3.set_title('Maximum Jitter', fontweight='bold')
    ax3.set_ylabel('Scheduler')
    ax3.set_xlabel('Workload')
    
    plt.tight_layout()
    output_file = GRAPHS_DIR / "scheduler_comparison_heatmap.png"
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"✓ Saved: {output_file.name}")
    plt.close()


def plot_lateness_analysis(df):
    """Plot lateness distribution for missed deadlines."""
    fig, axes = plt.subplots(2, 2, figsize=(16, 12))
    axes = axes.flatten()
    
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
    """Generate a text summary report with jitter information."""
    report_file = GRAPHS_DIR / "summary_report.txt"
    
    with open(report_file, 'w') as f:
        f.write("=" * 80 + "\n")
        f.write("ADVANCED RT SCHEDULER EVALUATION - SUMMARY REPORT\n")
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
                
                # Overall statistics
                total_activations = len(wl_data)
                total_misses = (wl_data['deadline_met'] == 0).sum()
                miss_rate = 100 * total_misses / total_activations if total_activations > 0 else 0
                avg_response = wl_data['response_time'].mean()
                max_response = wl_data['response_time'].max()
                min_response = wl_data['response_time'].min()
                avg_exec = wl_data['exec_time'].mean()
                max_jitter = wl_data['jitter'].max()
                avg_jitter = wl_data['jitter'].mean()
                
                f.write(f"    Total Activations: {total_activations}\n")
                f.write(f"    Deadline Misses: {total_misses} ({miss_rate:.2f}%)\n")
                f.write(f"    Response Time (avg/min/max): {avg_response:.1f} / {min_response} / {max_response} ms\n")
                f.write(f"    Execution Time (avg): {avg_exec:.1f} ms\n")
                f.write(f"    Jitter (avg/max): {avg_jitter:.2f} / {max_jitter:.2f} ms\n")
                
                # Per-task breakdown
                f.write(f"    Per-Task Breakdown:\n")
                for task_id in TASK_IDS:
                    task_data = wl_data[wl_data['task_id'] == task_id]
                    if len(task_data) > 0:
                        task_misses = (task_data['deadline_met'] == 0).sum()
                        task_miss_rate = 100 * task_misses / len(task_data)
                        task_avg_rt = task_data['response_time'].mean()
                        task_max_jit = task_data['jitter'].max()
                        f.write(f"      Task{task_id}: {len(task_data)} activations, "
                               f"{task_misses} misses ({task_miss_rate:.1f}%), "
                               f"avg RT={task_avg_rt:.1f}ms, max jitter={task_max_jit:.2f}ms\n")
                
                f.write("\n")
        
        f.write("=" * 80 + "\n")
    
    print(f"✓ Saved: {report_file.name}")


def main():
    """Main execution function."""
    print("=" * 80)
    print("Advanced RT Scheduler Evaluation - Graph Generation")
    print("=" * 80)
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
    print("-" * 80)
    
    plot_response_time_by_scheduler(df)
    plot_deadline_miss_rate(df)
    plot_response_time_distribution(df)
    plot_response_time_over_time(df)
    plot_scheduler_comparison_heatmap(df)
    plot_lateness_analysis(df)
    
    # NEW graphs for advanced_eval
    plot_jitter_comparison(df)
    plot_jitter_over_time(df)
    plot_exec_time_accuracy(df)
    
    generate_summary_report(df)
    
    print("-" * 80)
    print()
    print("=" * 80)
    print("✓ All graphs generated successfully!")
    print("=" * 80)
    print(f"Output location: {GRAPHS_DIR}")
    print()
    print("Generated graphs:")
    print("  • response_time_by_scheduler.png")
    print("  • deadline_miss_rate.png")
    print("  • response_time_distribution.png")
    print("  • response_time_timeline_*.png (4 files)")
    print("  • scheduler_comparison_heatmap.png")
    print("  • lateness_distribution.png")
    print("  • jitter_comparison.png (NEW)")
    print("  • jitter_evolution_*.png (4 files, NEW)")
    print("  • exec_time_accuracy.png (NEW)")
    print("  • summary_report.txt")
    print()


if __name__ == "__main__":
    main()
