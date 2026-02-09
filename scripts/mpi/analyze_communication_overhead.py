#!/usr/bin/env python3
"""
Communication Overhead Analysis for MPI
Analyzes the percentage of communication time vs execution time as processors increase.
"""

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path
import sys
import os


def load_mpi_data(matrix_folder, percentile=80):
    """Load MPI data from a matrix folder."""
    matrix_path = Path(matrix_folder)
    
    # Find MPI CSV file
    mpi_files = list(matrix_path.glob("stats_mpi_*.csv"))
    if not mpi_files:
        print(f"Error: No MPI data found in {matrix_folder}")
        return None
    
    mpi_file = mpi_files[0]
    print(f"Loading: {mpi_file.name}")
    
    # Load data
    df = pd.read_csv(mpi_file)
    
    # Filter by status (only correct computations)
    if 'Status' in df.columns:
        df = df[df['Status'] == 1]
    
    # Group by number of processors
    results = []
    for nproc in sorted(df['NProc'].unique()):
        proc_data = df[df['NProc'] == nproc]
        
        # Use percentile filtering
        exec_times = proc_data['Time']
        comm_times = proc_data['CommTime']
        
        # Calculate percentile threshold
        threshold = np.percentile(exec_times, percentile)
        mask = exec_times <= threshold
        
        filtered_exec = exec_times[mask]
        filtered_comm = comm_times[mask]
        
        if len(filtered_exec) > 0:
            mean_exec = filtered_exec.mean()
            mean_comm = filtered_comm.mean()
            
            # Total time = execution + communication
            total_time = mean_exec + mean_comm
            
            # Communication percentage of total time
            comm_percentage = (mean_comm / total_time) * 100 if total_time > 0 else 0
            comp_percentage = (mean_exec / total_time) * 100 if total_time > 0 else 0
            
            results.append({
                'nproc': nproc,
                'exec_time': mean_exec,
                'comm_time': mean_comm,
                'total_time': total_time,
                'comm_percentage': comm_percentage,
                'computation_percentage': comp_percentage
            })
    
    if not results:
        return None
    
    return pd.DataFrame(results)


def plot_communication_overhead(df, matrix_name, output_dir="."):
    """Generate communication overhead plot."""
    
    # Set publication style
    plt.rcParams.update({
        'font.size': 9,
        'font.family': 'serif',
        'axes.labelsize': 9,
        'axes.titlesize': 10,
        'xtick.labelsize': 8,
        'ytick.labelsize': 8,
        'legend.fontsize': 8,
        'figure.titlesize': 10,
        'lines.linewidth': 1.5,
        'lines.markersize': 5,
        'grid.alpha': 0.3,
        'axes.linewidth': 0.8
    })
    
    # Create figure
    fig, axes = plt.subplots(1, 2, figsize=(7, 2.5))
    
    # Plot 1: Communication percentage vs processors
    ax = axes[0]
    ax.plot(df['nproc'], df['comm_percentage'], 'o-', 
            color='#E63946', label='Communication', linewidth=1.5, markersize=5)
    ax.plot(df['nproc'], df['computation_percentage'], 's-', 
            color='#06A77D', label='Computation', linewidth=1.5, markersize=5)
    
    ax.set_xlabel('Number of Processors')
    ax.set_ylabel('Percentage (%)')
    ax.set_title('Time Distribution')
    ax.set_xscale('log', base=2)
    ax.grid(True, linewidth=0.5)
    ax.legend(loc='best', frameon=True, fancybox=False, edgecolor='black', framealpha=0.9)
    ax.set_ylim([0, 100])
    
    # Plot 2: Absolute times
    ax = axes[1]
    ax.plot(df['nproc'], df['exec_time'], 'o-', 
            color='#2E86AB', label='Execution Time', linewidth=1.5, markersize=5)
    ax.plot(df['nproc'], df['comm_time'], '^-', 
            color='#E63946', label='Communication Time', linewidth=1.5, markersize=5)
    
    ax.set_xlabel('Number of Processors')
    ax.set_ylabel('Time (s)')
    ax.set_title('Absolute Times')
    ax.set_xscale('log', base=2)
    ax.set_yscale('log')
    ax.grid(True, linewidth=0.5)
    ax.legend(loc='best', frameon=True, fancybox=False, edgecolor='black', framealpha=0.9)
    
    # Add main title
    fig.suptitle(f'MPI Communication Overhead - {matrix_name}', fontsize=12, fontweight='bold', y=0.99)
    
    plt.tight_layout(rect=[0, 0, 1, 0.94])  # Leave space for suptitle
    
    # Save
    output_path = Path(output_dir) / "communication_overhead_analysis.png"
    plt.savefig(output_path, dpi=300, bbox_inches='tight')
    print(f"Plots saved to: {output_path}")
    plt.close()


def print_summary(df, matrix_name):
    """Print summary statistics."""
    print(f"\n{matrix_name} - Communication Overhead Analysis:")
    print("="*90)
    print(f"{'Procs':<10} {'Exec (s)':<12} {'Comm (s)':<12} {'Total (s)':<12} {'Comm %':<10} {'Comp %':<10}")
    print("-"*90)
    
    for _, row in df.iterrows():
        print(f"{int(row['nproc']):<10} {row['exec_time']:<12.6f} {row['comm_time']:<12.6f} "
              f"{row['total_time']:<12.6f} {row['comm_percentage']:<10.2f} {row['computation_percentage']:<10.2f}")
    
    print("\nKey Observations:")
    print(f"  - Communication overhead at 1 proc:   {df.iloc[0]['comm_percentage']:.2f}%")
    print(f"  - Communication overhead at max procs: {df.iloc[-1]['comm_percentage']:.2f}%")
    print(f"  - Overhead increase: {df.iloc[-1]['comm_percentage'] - df.iloc[0]['comm_percentage']:.2f} percentage points")


def main():
    if len(sys.argv) < 2:
        print("Usage: python analyze_communication_overhead.py <matrix_folder> [output_dir] [percentile]")
        print("\nExample: python analyze_communication_overhead.py inline_1 ./results 80")
        print("\nAvailable matrices: Ga41As41H72, inline_1, largebasis, nd24k, pre2")
        sys.exit(1)
    
    matrix_folder = sys.argv[1]
    output_dir = sys.argv[2] if len(sys.argv) > 2 else "."
    percentile = int(sys.argv[3]) if len(sys.argv) > 3 else 80
    
    # Create output directory
    Path(output_dir).mkdir(parents=True, exist_ok=True)
    
    if not os.path.exists(matrix_folder):
        print(f"Error: Matrix folder '{matrix_folder}' does not exist")
        sys.exit(1)
    
    # Extract matrix name from folder
    matrix_name = Path(matrix_folder).name
    
    # Load and analyze data
    df = load_mpi_data(matrix_folder, percentile)
    
    if df is None or df.empty:
        print("No data to analyze")
        sys.exit(1)
    
    print(f"\nAnalyzing {len(df)} processor configurations")
    
    # Generate plots
    plot_communication_overhead(df, matrix_name, output_dir)
    
    # Print summary
    print_summary(df, matrix_name)


if __name__ == "__main__":
    main()
