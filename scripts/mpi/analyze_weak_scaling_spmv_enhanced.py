#!/usr/bin/env python3
"""
Enhanced Weak Scaling Analysis for SpMV - Synthetic Matrices

Analyzes weak scaling performance from raw OpenMP and MPI benchmark data
with 90th percentile filtering for outlier removal.

Reads actual matrix dimensions from Matrix Market (.mtx) files.

Hardware: 4 physical cores (PBS allocation)
"""

import pandas as pd
import numpy as np
import glob
import os
import sys
from pathlib import Path


def read_matrix_market_header(mtx_file):
    """
    Read Matrix Market file header to extract dimensions.
    
    Returns:
        (rows, cols, nnz) tuple
    """
    with open(mtx_file, 'r') as f:
        # Skip comment lines starting with %
        for line in f:
            if not line.startswith('%'):
                # First non-comment line contains: rows cols nnz
                parts = line.strip().split()
                rows = int(parts[0])
                cols = int(parts[1])
                nnz = int(parts[2])
                return rows, cols, nnz
    return None, None, None


def get_matrix_dimensions(datasets_dir, matrix_name):
    """
    Get matrix dimensions from .mtx file in datasets directory.
    
    Args:
        datasets_dir: Path to datasets folder
        matrix_name: Name of matrix file (e.g., 'matrix_weak_scaling_10000_10000_200000')
    
    Returns:
        (rows, cols, nnz) tuple
    """
    mtx_file = Path(datasets_dir) / f"{matrix_name}.mtx"
    
    if not mtx_file.exists():
        print(f"Warning: Matrix file not found: {mtx_file}")
        return None, None, None
    
    return read_matrix_market_header(mtx_file)


def load_mpi_raw_data(base_dir, datasets_dir):
    """
    Load raw MPI data from all weak scaling folders.
    Returns DataFrame with all individual measurements.
    
    For weak scaling: P (scaling factor) = NProc (number of processes)
    Each process should handle the same workload as the base case.
    """
    mpi_data = []
    
    weak_scaling_dirs = sorted(glob.glob(f"{base_dir}/matrix_weak_scaling_*"))
    
    if not weak_scaling_dirs:
        print(f"No weak scaling directories found in {base_dir}")
        return None
    
    # Get base matrix dimensions (P=1)
    base_matrix_name = os.path.basename(weak_scaling_dirs[0])
    base_rows, base_cols, base_nnz = get_matrix_dimensions(datasets_dir, base_matrix_name)
    
    if base_rows is None:
        print(f"Error: Could not read base matrix dimensions from {datasets_dir}")
        return None
    
    print(f"Base matrix (P=1): {base_rows}x{base_cols} with {base_nnz:,} non-zeros")
    
    for dir_path in weak_scaling_dirs:
        dirname = os.path.basename(dir_path)
        rows, cols, nnz = get_matrix_dimensions(datasets_dir, dirname)
        
        if rows is None:
            print(f"Warning: Skipping {dirname} - could not read matrix dimensions")
            continue
        
        # Find MPI statistics file
        mpi_files = glob.glob(f"{dir_path}/stats_mpi_*.csv")
        if mpi_files:
            df = pd.read_csv(mpi_files[0])
            df['rows'] = rows
            df['cols'] = cols
            df['nnz'] = nnz
            # For weak scaling: P = NProc (match matrix to process count)
            # Only keep rows where P = NProc (proper weak scaling)
            df['P'] = df['NProc']
            # Filter: only keep data where nnz matches P * base_nnz
            df = df[df['nnz'] == df['P'] * base_nnz]
            if not df.empty:
                mpi_data.append(df)
    
    if mpi_data:
        return pd.concat(mpi_data, ignore_index=True)
    return None


def load_omp_raw_data(base_dir, datasets_dir):
    """
    Load raw OpenMP data from all weak scaling folders.
    Returns DataFrame with all individual measurements.
    
    For weak scaling: P (scaling factor) = Num_Threads
    Each thread should handle the same workload as the base case.
    """
    omp_data = []
    
    weak_scaling_dirs = sorted(glob.glob(f"{base_dir}/matrix_weak_scaling_*"))
    
    if not weak_scaling_dirs:
        return None
    
    # Get base matrix dimensions (P=1)
    base_matrix_name = os.path.basename(weak_scaling_dirs[0])
    base_rows, base_cols, base_nnz = get_matrix_dimensions(datasets_dir, base_matrix_name)
    
    if base_rows is None:
        return None
    
    for dir_path in weak_scaling_dirs:
        dirname = os.path.basename(dir_path)
        rows, cols, nnz = get_matrix_dimensions(datasets_dir, dirname)
        
        if rows is None:
            continue
        
        # Find OpenMP statistics file
        omp_files = glob.glob(f"{dir_path}/stats_OpenMP_*.csv")
        if omp_files:
            df = pd.read_csv(omp_files[0])
            df['rows'] = rows
            df['cols'] = cols
            df['nnz'] = nnz
            # For weak scaling: P = Num_Threads (match matrix to thread count)
            df['P'] = df['Num_Threads']
            # Filter: only keep data where nnz matches P * base_nnz
            df = df[df['nnz'] == df['P'] * base_nnz]
            if not df.empty:
                omp_data.append(df)
    
    if omp_data:
        return pd.concat(omp_data, ignore_index=True)
    return None


def apply_percentile_filter(times, percentile=90):
    """
    Filter times using percentile threshold.
    Keeps only times within the fastest percentile (below threshold).
    
    Args:
        times: Array of execution times
        percentile: Percentile threshold (default 90)
    
    Returns:
        Filtered times (fastest percentile)
    """
    threshold = np.percentile(times, percentile)
    return times[times <= threshold]


def calculate_gflops(time_ms, nnz):
    """
    Calculate GFLOP/s for SpMV operation.
    
    FLOP = 2 * nnz (one multiply + one add per non-zero element)
    GFLOP/s = FLOP / (time_ms * 10^6)
    
    Args:
        time_ms: Execution time in milliseconds
        nnz: Number of non-zero elements
    
    Returns:
        GFLOP/s value
    """
    flops = 2 * nnz
    gflops = flops / (time_ms * 1e6)
    return gflops


def analyze_mpi_data(df_mpi, percentile=90):
    """
    Analyze MPI data with percentile filtering.
    
    MPI measures time with MPI_Wtime() which returns seconds.
    This function converts to milliseconds for consistency with OpenMP output.
    Uses the GFLOPS column directly from the MPI data.
    
    Returns:
        DataFrame with columns: NProc, P, N, nnz, Time_avg_ms, GFLOPS, Efficiency
    """
    results = []
    
    # Group by NProc and P (scaling factor)
    for (nproc, p), group in df_mpi.groupby(['NProc', 'P']):
        times = group['Time'].values  # Time is in seconds
        gflops_values = group['GFLOPS'].values
        
        # Apply percentile filter to times
        threshold = np.percentile(times, percentile)
        mask = times <= threshold
        
        filtered_times = times[mask]
        filtered_gflops = gflops_values[mask]
        
        if len(filtered_times) == 0:
            continue
        
        mean_time_sec = np.mean(filtered_times)
        mean_time_ms = mean_time_sec * 1000  # Convert to milliseconds
        mean_gflops = np.mean(filtered_gflops)
        
        nnz = group['nnz'].iloc[0]
        rows = group['rows'].iloc[0]
        
        results.append({
            'NProc': int(nproc),
            'P': int(p),
            'N': int(rows),
            'nnz': int(nnz),
            'Time_avg': mean_time_ms,
            'GFLOPS': mean_gflops
        })
    
    df_results = pd.DataFrame(results).sort_values('NProc')
    
    # Calculate efficiency relative to P=1
    if len(df_results) > 0:
        time_base = df_results[df_results['P'] == 1]['Time_avg'].values[0]
        df_results['Efficiency'] = (time_base / df_results['Time_avg']) * 100
    
    return df_results


def analyze_omp_data(df_omp, percentile=90):
    """
    Analyze OpenMP data with percentile filtering.
    
    OpenMP measures time with C++ steady_clock using std::chrono::duration<double, std::milli>
    which returns milliseconds directly.
    
    Returns:
        DataFrame with columns: Threads, P, N, nnz, Time_avg_ms, GFLOPS, Efficiency
    """
    results = []
    
    # Group by Num_Threads and P (scaling factor)
    for (nthreads, p), group in df_omp.groupby(['Num_Threads', 'P']):
        times = group['Execution_time'].values  # Time is in milliseconds (C++ chrono::milli)
        
        # Apply percentile filter
        threshold = np.percentile(times, percentile)
        filtered_times = times[times <= threshold]
        
        if len(filtered_times) == 0:
            continue
        
        mean_time_ms = np.mean(filtered_times)  # Already in milliseconds
        
        nnz = group['nnz'].iloc[0]
        rows = group['rows'].iloc[0]
        
        results.append({
            'Threads': int(nthreads),
            'P': int(p),
            'N': int(rows),
            'nnz': int(nnz),
            'Time_avg': mean_time_ms,
            'GFLOPS': calculate_gflops(mean_time_ms, nnz)
        })
    
    df_results = pd.DataFrame(results).sort_values('Threads')
    
    # Calculate efficiency relative to P=1
    if len(df_results) > 0:
        time_base = df_results[df_results['P'] == 1]['Time_avg'].values[0]
        df_results['Efficiency'] = (time_base / df_results['Time_avg']) * 100
    
    return df_results


def generate_markdown_report(df_mpi, df_omp, percentile):
    """
    Generate comprehensive Markdown report with 4 tables.
    """
    lines = []
    
    # Get base matrix info from data
    base_rows = df_mpi[df_mpi['P'] == 1]['N'].iloc[0]
    base_nnz = df_mpi[df_mpi['P'] == 1]['nnz'].iloc[0]
    
    # Header
    lines.append("# Weak Scaling Analysis - SpMV (Synthetic Matrices)\n")
    lines.append("## Analysis Configuration\n")
    lines.append(f"- **Base Matrix (P=1)**: {base_rows:,} × {base_rows:,} with {base_nnz:,} non-zeros")
    lines.append(f"- **Scaling Law**: Total nnz = {base_nnz:,} × P")
    lines.append(f"- **Hardware**: 4 physical cores (PBS allocation)")
    lines.append(f"- **Filtering**: Top {percentile}th percentile (outlier removal)")
    lines.append(f"- **GFLOP/s Formula**: (2 × nnz) / (Time_ms × 10⁶)\n")
    
    # Table 1: Dataset Characteristics
    lines.append("## Table 1: Dataset Characteristics\n")
    lines.append("Shows how matrix size scales with processor count P.\n")
    lines.append("| P | N (rows) | nnz (non-zeros) |")
    lines.append("|--:|----------:|----------------:|")
    
    # Get unique P values from both datasets and their actual dimensions
    all_p_values = sorted(set(list(df_mpi['P'].unique()) + list(df_omp['P'].unique())))
    
    for p in all_p_values:
        # Get actual dimensions from data
        mpi_row = df_mpi[df_mpi['P'] == p]
        if not mpi_row.empty:
            n = mpi_row['N'].iloc[0]
            nnz = mpi_row['nnz'].iloc[0]
        else:
            omp_row = df_omp[df_omp['P'] == p]
            n = omp_row['N'].iloc[0]
            nnz = omp_row['nnz'].iloc[0]
        lines.append(f"| {p:3d} | {n:9,} | {nnz:15,} |")
    
    # Table 2: MPI Performance
    lines.append("\n## Table 2: MPI Performance\n")
    lines.append("| P | Processes | Mean Time (ms) | GFLOP/s | Efficiency (%) |")
    lines.append("|--:|----------:|---------------:|--------:|---------------:|")
    
    for _, row in df_mpi.iterrows():
        lines.append(f"| {int(row['P']):3d} | {int(row['NProc']):9d} | {row['Time_avg']:14.6f} | {row['GFLOPS']:7.4f} | {row['Efficiency']:14.2f} |")
    
    # Table 3: OpenMP Performance
    lines.append("\n## Table 3: OpenMP Performance\n")
    lines.append("| P | Threads | Mean Time (ms) | GFLOP/s | Efficiency (%) |")
    lines.append("|--:|--------:|---------------:|--------:|---------------:|")
    
    for _, row in df_omp.iterrows():
        lines.append(f"| {int(row['P']):3d} | {int(row['Threads']):7d} | {row['Time_avg']:14.6f} | {row['GFLOPS']:7.4f} | {row['Efficiency']:14.2f} |")
    
    # Table 4: OpenMP vs MPI Comparison
    lines.append("\n## Table 4: OpenMP vs MPI Comparison\n")
    lines.append("Direct comparison for common P values (Threads/Processes).\n")
    lines.append("| P | OMP Time (ms) | OMP GFLOP/s | MPI Time (ms) | MPI GFLOP/s | Relative Speedup |")
    lines.append("|--:|--------------:|------------:|--------------:|------------:|-----------------:|")
    
    # Find common P values
    common_p = sorted(set(df_mpi['P'].values) & set(df_omp['P'].values))
    
    for p in common_p:
        omp_row = df_omp[df_omp['P'] == p].iloc[0]
        mpi_row = df_mpi[df_mpi['P'] == p].iloc[0]
        
        omp_time = omp_row['Time_avg']
        omp_gflops = omp_row['GFLOPS']
        mpi_time = mpi_row['Time_avg']
        mpi_gflops = mpi_row['GFLOPS']
        
        # Relative speedup: Time_OMP / Time_MPI
        # > 1.0 means MPI is faster
        speedup = omp_time / mpi_time
        
        lines.append(f"| {p:3d} | {omp_time:13.6f} | {omp_gflops:11.4f} | "
                    f"{mpi_time:13.6f} | {mpi_gflops:11.4f} | {speedup:16.2f} |")
    
    # Analysis Notes
    lines.append("\n## Key Observations\n")
    lines.append("### Relative Speedup Interpretation\n")
    lines.append("- **Speedup < 1.0**: OpenMP is faster than MPI")
    lines.append("- **Speedup = 1.0**: Equal performance")
    lines.append("- **Speedup > 1.0**: MPI is faster than OpenMP (MPI overtakes)\n")
    
    lines.append("### Weak Scaling Behavior\n")
    lines.append("- **Ideal weak scaling**: Efficiency = 100% (constant time as problem scales)")
    lines.append("- **Efficiency > 100%**: Super-linear speedup (cache effects, memory bandwidth)")
    lines.append("- **Efficiency < 100%**: Performance degradation (oversubscription, communication overhead)\n")
    
    lines.append("### Hardware Constraints\n")
    lines.append(f"- **Physical cores**: 4")
    lines.append(f"- **P ≤ 4**: Optimal parallelization (no oversubscription)")
    lines.append(f"- **P > 4**: Oversubscription (multiple threads/processes per core)")
    lines.append(f"  - Context switching overhead")
    lines.append(f"  - Cache thrashing")
    lines.append(f"  - Memory bandwidth contention\n")
    
    return "\n".join(lines)


def print_console_summary(df_mpi, df_omp):
    """Print summary to console for quick analysis."""
    print("\n" + "="*100)
    print("WEAK SCALING ANALYSIS SUMMARY - SpMV (Synthetic Matrices)")
    print("="*100)
    
    print("\n" + "-"*100)
    print("MPI PERFORMANCE")
    print("-"*100)
    print(f"{'P':>4} {'NProc':>6} {'Time (ms)':>14} {'GFLOP/s':>10} {'Efficiency (%)':>16}")
    print("-"*100)
    for _, row in df_mpi.iterrows():
        print(f"{int(row['P']):4d} {int(row['NProc']):6d} {row['Time_avg']:14.6f} "
              f"{row['GFLOPS']:10.4f} {row['Efficiency']:16.2f}")
    
    print("\n" + "-"*100)
    print("OPENMP PERFORMANCE")
    print("-"*100)
    print(f"{'P':>4} {'Threads':>8} {'Time (ms)':>14} {'GFLOP/s':>10} {'Efficiency (%)':>16}")
    print("-"*100)
    for _, row in df_omp.iterrows():
        print(f"{int(row['P']):4d} {int(row['Threads']):8d} {row['Time_avg']:14.6f} "
              f"{row['GFLOPS']:10.4f} {row['Efficiency']:16.2f}")
    
    print("\n" + "-"*100)
    print("OPENMP vs MPI COMPARISON")
    print("-"*100)
    print(f"{'P':>4} {'OMP Time':>14} {'MPI Time':>14} {'Speedup':>12} {'Winner':>10}")
    print("-"*100)
    
    common_p = sorted(set(df_mpi['P'].values) & set(df_omp['P'].values))
    for p in common_p:
        omp_row = df_omp[df_omp['P'] == p].iloc[0]
        mpi_row = df_mpi[df_mpi['P'] == p].iloc[0]
        
        omp_time = omp_row['Time_avg']
        mpi_time = mpi_row['Time_avg']
        speedup = omp_time / mpi_time
        winner = "MPI" if speedup > 1.0 else ("OMP" if speedup < 1.0 else "TIE")
        
        print(f"{p:4d} {omp_time:14.6f} {mpi_time:14.6f} {speedup:12.2f}x {winner:>10}")
    
    print("="*100)


def main():
    if len(sys.argv) < 3:
        print("Usage: python analyze_weak_scaling_spmv_enhanced.py <data_path> <datasets_dir> [output_dir] [percentile]")
        print("\nExample: python analyze_weak_scaling_spmv_enhanced.py . ./datasets ./results 90")
        print("\nArguments:")
        print("  data_path    : Directory containing 'matrix_weak_scaling_*' subdirectories")
        print("  datasets_dir : Directory containing .mtx matrix files")
        print("  output_dir   : Output directory for report (default: current directory)")
        print("  percentile   : Percentile threshold for filtering (default: 90)")
        sys.exit(1)
    
    data_path = sys.argv[1]
    datasets_dir = sys.argv[2]
    output_dir = sys.argv[3] if len(sys.argv) > 3 else "."
    percentile = int(sys.argv[4]) if len(sys.argv) > 4 else 90
    
    # Validate inputs
    if not os.path.exists(data_path):
        print(f"Error: Data path '{data_path}' does not exist")
        sys.exit(1)
    
    if not os.path.exists(datasets_dir):
        print(f"Error: Datasets directory '{datasets_dir}' does not exist")
        sys.exit(1)
    
    if percentile < 0 or percentile > 100:
        print(f"Error: Percentile must be between 0 and 100 (got {percentile})")
        sys.exit(1)
    
    # Create output directory
    Path(output_dir).mkdir(parents=True, exist_ok=True)
    
    print(f"Loading raw MPI data from {data_path}...")
    print(f"Reading matrix dimensions from {datasets_dir}...")
    df_mpi_raw = load_mpi_raw_data(data_path, datasets_dir)
    
    print(f"\nLoading raw OpenMP data from {data_path}...")
    df_omp_raw = load_omp_raw_data(data_path, datasets_dir)
    
    if df_mpi_raw is None or df_omp_raw is None:
        print("ERROR: Unable to load data")
        sys.exit(1)
    
    print(f"✓ MPI data loaded: {len(df_mpi_raw)} measurements")
    print(f"✓ OpenMP data loaded: {len(df_omp_raw)} measurements")
    
    # Analyze with percentile filtering
    print(f"\nAnalyzing with {percentile}th percentile filtering...")
    df_mpi = analyze_mpi_data(df_mpi_raw, percentile)
    df_omp = analyze_omp_data(df_omp_raw, percentile)
    
    print(f"✓ MPI results: {len(df_mpi)} configurations")
    print(f"✓ OpenMP results: {len(df_omp)} configurations")
    
    # Generate Markdown report
    print("\nGenerating Markdown report...")
    markdown_output = generate_markdown_report(df_mpi, df_omp, percentile)
    
    # Save report
    output_file = Path(output_dir) / "weak_scaling_spmv_enhanced_report.md"
    with open(output_file, 'w') as f:
        f.write(markdown_output)
    
    print(f"✓ Report saved to: {output_file}")
    
    # Save processed data to CSV
    df_mpi.to_csv(Path(output_dir) / "mpi_weak_scaling_processed.csv", index=False)
    df_omp.to_csv(Path(output_dir) / "omp_weak_scaling_processed.csv", index=False)
    print(f"✓ Processed data saved to CSV files")
    
    # Print console summary
    print_console_summary(df_mpi, df_omp)
    
    print("\n✓ Analysis completed successfully!")


if __name__ == "__main__":
    main()
