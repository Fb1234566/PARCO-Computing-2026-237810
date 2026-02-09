#!/usr/bin/env python3
"""
Weak Scaling Analysis Script

Analyzes weak scaling performance from serial, OpenMP, and MPI benchmark data.
Each approach shows a single line where matrix size scales with processor count.
Uses top 90th percentile of execution times for analysis.
"""

import os
import sys
import pandas as pd
import numpy as np
import matplotlib
matplotlib.use('Agg')  # Use non-interactive backend for servers
import matplotlib.pyplot as plt
from pathlib import Path


def extract_matrix_size(folder_name):
    """Extract matrix dimensions from folder name."""
    if folder_name.startswith("matrix_weak_scaling_"):
        parts = folder_name.replace("matrix_weak_scaling_", "").split("_")
        if len(parts) >= 3:
            return int(parts[0]), int(parts[1]), int(parts[2])
    return None, None, None


def load_serial_data(folder_path):
    """Load serial statistics from a weak scaling folder."""
    serial_files = list(folder_path.glob("stats_serial_*.csv"))
    if not serial_files:
        return None
    df = pd.read_csv(serial_files[0])
    # Execution_time is in milliseconds, convert to seconds
    if 'Execution_time' in df.columns:
        df['Execution_time'] = df['Execution_time']
    return df


def load_openmp_data(folder_path):
    """Load OpenMP statistics from a weak scaling folder."""
    openmp_files = list(folder_path.glob("stats_OpenMP_*.csv"))
    if not openmp_files:
        return None
    df = pd.read_csv(openmp_files[0])
    if 'Execution_time' in df.columns:
        df['Execution_time'] = df['Execution_time']
    return df


def load_mpi_data(folder_path):
    """Load MPI statistics from a weak scaling folder."""
    mpi_files = list(folder_path.glob("stats_mpi_*.csv"))
    if not mpi_files:
        return None
    df = pd.read_csv(mpi_files[0])
    # MPI Time is already in seconds (MPI_Wtime)
    return df


def compute_percentile_time(times, percentile=90):
    """Compute mean of top percentile (fastest times)."""
    threshold = np.percentile(times, 100 - percentile)
    filtered_times = times[times <= threshold]
    return np.mean(filtered_times), np.std(filtered_times)


def analyze_weak_scaling(base_path, percentile=90):
    """Analyze weak scaling across all matrix sizes for serial, OpenMP, and MPI."""
    base_path = Path(base_path)
    
    # Find all weak scaling folders
    scaling_folders = sorted([d for d in base_path.iterdir() 
                             if d.is_dir() and d.name.startswith("matrix_weak_scaling_")])
    
    if not scaling_folders:
        print(f"No weak scaling folders found in {base_path}")
        return None, None, None
    
    print(f"Found {len(scaling_folders)} weak scaling folders")
    print(f"Using top {percentile}th percentile of execution times\n")
    
    # Build mapping: matrix_size -> folder_path
    matrix_to_folder = {}
    for folder in scaling_folders:
        rows, cols, nnz = extract_matrix_size(folder.name)
        if rows is not None:
            matrix_to_folder[rows] = (folder, rows, cols, nnz)
    
    # Determine the work per processor (should be constant for weak scaling)
    # Find the smallest matrix size and assume that's the work per processor
    sorted_sizes = sorted(matrix_to_folder.keys())
    base_size = sorted_sizes[0]
    
    print(f"Base matrix size: {base_size}x{base_size}")
    print(f"Matrix sizes available: {sorted_sizes}\n")
    
    serial_results = []
    openmp_results = []
    mpi_results = []
    
    # For serial: collect data for all matrix sizes (treating it as nproc=1,2,4,8,... based on matrix size scaling)
    for folder, rows, cols, nnz in matrix_to_folder.values():
        serial_df = load_serial_data(folder)
        if serial_df is not None:
            times = serial_df['Execution_time'].values
            mean_time, std_time = compute_percentile_time(times, percentile)
            # Calculate effective "nproc" based on matrix size scaling
            effective_nproc = rows // base_size
            serial_results.append({
                'nproc': effective_nproc,
                'matrix_rows': rows,
                'matrix_cols': cols,
                'matrix_nnz': nnz,
                'mean_time': mean_time,
                'std_time': std_time
            })
            print(f"Serial (matrix {rows}x{cols}): {mean_time:.6f}s")
    
    # For OpenMP and MPI: match matrix size to processor count
    # Expected: matrix_size = base_size * sqrt(nproc) for 2D problems
    # Or more simply: rows should be proportional to nproc
    
    # Collect all OpenMP data points
    # For proper weak scaling: match P threads with P×base_size matrix
    for folder, rows, cols, nnz in matrix_to_folder.values():
        openmp_df = load_openmp_data(folder)
        if openmp_df is not None and 'Num_Threads' in openmp_df.columns:
            for nthreads in sorted(openmp_df['Num_Threads'].unique()):
                nthreads = int(nthreads)
                # Calculate expected matrix size for this thread count
                expected_size = base_size * nthreads  # rows scale linearly with threads
                
                # Check if current matrix matches expected size (filters out wrong combinations)
                # This ensures: 1 thread -> 10000x10000, 2 threads -> 20000x20000, etc.
                if rows == expected_size:
                    thread_data = openmp_df[openmp_df['Num_Threads'] == nthreads]
                    times = thread_data['Execution_time'].values
                    if len(times) > 0:
                        mean_time, std_time = compute_percentile_time(times, percentile)
                        openmp_results.append({
                            'nproc': nthreads,
                            'matrix_rows': rows,
                            'matrix_cols': cols,
                            'matrix_nnz': nnz,
                            'mean_time': mean_time,
                            'std_time': std_time
                        })
                        print(f"OpenMP (nproc={nthreads}): Matrix {rows}x{cols} -> {mean_time:.6f}s")
    
    # Collect all MPI data points
    # For proper weak scaling: match P processes with P×base_size matrix
    for folder, rows, cols, nnz in matrix_to_folder.values():
        mpi_df = load_mpi_data(folder)
        if mpi_df is not None:
            for nproc in sorted(mpi_df['NProc'].unique()):
                nproc_int = int(nproc)
                # Calculate expected matrix size for this processor count
                expected_size = base_size * nproc_int  # rows scale linearly with processors
                
                # Check if current matrix matches expected size (filters out wrong combinations)
                # This ensures: 1 proc -> 10000x10000, 2 procs -> 20000x20000, etc.
                if rows == expected_size:
                    proc_data = mpi_df[mpi_df['NProc'] == nproc]
                    times = proc_data['Time'].values
                    if len(times) > 0:
                        mean_time, std_time = compute_percentile_time(times, percentile)
                        mpi_results.append({
                            'nproc': nproc_int,
                            'matrix_rows': rows,
                            'matrix_cols': cols,
                            'matrix_nnz': nnz,
                            'mean_time': mean_time,
                            'std_time': std_time
                        })
                        print(f"MPI (nproc={nproc_int}): Matrix {rows}x{cols} -> {mean_time:.6f}s")
    
    print(f"\nCollected: {len(serial_results)} serial, {len(openmp_results)} OpenMP, {len(mpi_results)} MPI data points")
    
    return (pd.DataFrame(serial_results) if serial_results else None,
            pd.DataFrame(openmp_results) if openmp_results else None,
            pd.DataFrame(mpi_results) if mpi_results else None)


def plot_weak_scaling(serial_df, openmp_df, mpi_df, output_dir="."):
    """Generate publication-quality weak scaling plots."""
    
    # Set publication style for 2-column format
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
    
    # Create figure optimized for 1-column format (side by side)
    fig, axes = plt.subplots(1, 2, figsize=(7, 2.5))
    
    # Plot 1: Execution time in milliseconds
    ax = axes[0]
    
    if openmp_df is not None and not openmp_df.empty:
        openmp_sorted = openmp_df.sort_values('nproc')
        time_ms = openmp_sorted['mean_time']  # Already in milliseconds
        ax.plot(openmp_sorted['nproc'], time_ms, 's-', 
                label='OpenMP', color='#A23B72')
    
    if mpi_df is not None and not mpi_df.empty:
        mpi_sorted = mpi_df.sort_values('nproc')
        time_ms = mpi_sorted['mean_time'] * 1000  # Convert to milliseconds
        ax.plot(mpi_sorted['nproc'], time_ms, '^-', 
                label='MPI', color='#F18F01')
    
    ax.set_xlabel('Number of Processors/Threads')
    ax.set_ylabel('Execution Time (ms)')
    ax.set_title('Execution Time')
    ax.set_xscale('log', base=2)
    ax.set_yscale('log')
    ax.grid(True, linewidth=0.5)
    ax.legend(loc='best', frameon=True, fancybox=False, edgecolor='black', framealpha=0.9)
    
    # Plot 2: Weak scaling efficiency
    # For weak scaling: efficiency = time_at_nproc_1 / time_at_nproc_N
    # Each point compares nproc=N handling N×workload vs nproc=1 handling 1×workload
    ax = axes[1]
    
    if openmp_df is not None and not openmp_df.empty:
        openmp_sorted = openmp_df.sort_values('nproc')
        # Baseline: nproc=1 with base problem size
        openmp_baseline = openmp_sorted[openmp_sorted['nproc'] == 1]['mean_time'].values[0]
        efficiency = (openmp_baseline / openmp_sorted['mean_time']) * 100
        ax.plot(openmp_sorted['nproc'], efficiency, 's-', 
                label='OpenMP', color='#A23B72')
    
    if mpi_df is not None and not mpi_df.empty:
        mpi_sorted = mpi_df.sort_values('nproc')
        # Baseline: nproc=1 with base problem size
        mpi_baseline = mpi_sorted[mpi_sorted['nproc'] == 1]['mean_time'].values[0]
        efficiency = (mpi_baseline / mpi_sorted['mean_time']) * 100
        ax.plot(mpi_sorted['nproc'], efficiency, '^-', 
                label='MPI', color='#F18F01')
    
    ax.axhline(y=100, color='black', linestyle='--', linewidth=1.2, alpha=0.7, 
               label=f'Ideal')
    ax.set_xlabel('Number of Processors/Threads')
    ax.set_ylabel('Efficiency (%)')
    ax.set_title('Efficiency')
    ax.set_xscale('log', base=2)
    ax.grid(True, linewidth=0.5)
    ax.legend(loc='best', frameon=True, fancybox=False, edgecolor='black', framealpha=0.9)
    
    # Add main title
    fig.suptitle('Weak Scaling', fontsize=12, fontweight='bold', y=0.99)
    
    plt.tight_layout(rect=[0, 0, 1, 0.94])  # Leave space for suptitle
    output_path = Path(output_dir) / "weak_scaling_analysis.png"
    plt.savefig(output_path, dpi=300, bbox_inches='tight')
    print(f"Plots saved to: {output_path}")
    plt.close()


def print_summary(serial_df, openmp_df, mpi_df):
    """Print summary statistics."""
    print("\n" + "="*80)
    print("WEAK SCALING ANALYSIS SUMMARY")
    print("="*80)
    
    def print_approach(df, name):
        if df is None or df.empty:
            print(f"\n{name}: No data")
            return
        
        print(f"\n{name}:")
        print("-" * 80)
        print(f"{'NProc':<8} {'Matrix Size':<20} {'NNZ':<12} {'Mean Time (s)':<15} {'Std Dev':<12}")
        print("-" * 80)
        
        df_sorted = df.sort_values('nproc')
        for _, row in df_sorted.iterrows():
            print(f"{row['nproc']:<8} {int(row['matrix_rows'])}x{int(row['matrix_cols']):<14} "
                  f"{int(row['matrix_nnz']):<12} {row['mean_time']:<15.6f} {row['std_time']:<12.6f}")
        
        # Calculate weak scaling efficiency relative to nproc=1 baseline
        baseline_row = df_sorted[df_sorted['nproc'] == 1]
        if not baseline_row.empty:
            baseline = baseline_row.iloc[0]['mean_time']
            print(f"\nWeak Scaling Efficiency (relative to nproc=1 baseline = {baseline:.6f}s):")
            print("-" * 80)
            print(f"{'NProc':<8} {'Efficiency (%)':<15} {'Relative Time':<15}")
            print("-" * 80)
            for _, row in df_sorted.iterrows():
                efficiency = (baseline / row['mean_time']) * 100
                relative_time = row['mean_time'] / baseline
                print(f"{row['nproc']:<8} {efficiency:<15.2f} {relative_time:<15.2f}x")
        else:
            print("\nNo baseline (nproc=1) data available for efficiency calculation")
    
    print_approach(serial_df, "SERIAL")
    print_approach(openmp_df, "OPENMP")
    print_approach(mpi_df, "MPI")


def main():
    if len(sys.argv) < 2:
        print("Usage: python analyze_weak_scaling.py <data_path> [output_dir] [percentile]")
        print("\nExample: python analyze_weak_scaling.py . ./results 80")
        print("\nThe data_path should contain 'matrix_weak_scaling_*' subdirectories")
        sys.exit(1)
    
    data_path = sys.argv[1]
    output_dir = sys.argv[2] if len(sys.argv) > 2 else "."
    percentile = int(sys.argv[3]) if len(sys.argv) > 3 else 80
    
    # Create output directory if it doesn't exist
    Path(output_dir).mkdir(parents=True, exist_ok=True)
    
    if not os.path.exists(data_path):
        print(f"Error: Path '{data_path}' does not exist")
        sys.exit(1)
    
    # Analyze weak scaling
    serial_df, openmp_df, mpi_df = analyze_weak_scaling(data_path, percentile)
    
    if (serial_df is None or serial_df.empty) and \
       (openmp_df is None or openmp_df.empty) and \
       (mpi_df is None or mpi_df.empty):
        print("No data to analyze")
        sys.exit(1)
    
    # Generate plots only
    plot_weak_scaling(serial_df, openmp_df, mpi_df, output_dir)


if __name__ == "__main__":
    main()
