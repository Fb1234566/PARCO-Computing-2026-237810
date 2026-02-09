#!/usr/bin/env python3
"""
Strong Scaling Analysis Script

Analyzes strong scaling performance for real-world matrices.
Strong scaling: fixed problem size, varying number of processors.
"""

import os
import sys
import pandas as pd
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from pathlib import Path
from datetime import datetime


def load_serial_data(folder_path):
    """Load serial statistics."""
    serial_files = list(folder_path.glob("stats_serial_*.csv"))
    if not serial_files:
        return None
    df = pd.read_csv(serial_files[0])
    # Execution_time is already in milliseconds, no conversion needed
    return df


def load_openmp_data(folder_path):
    """Load OpenMP statistics."""
    openmp_files = list(folder_path.glob("stats_OpenMP_*.csv"))
    if not openmp_files:
        return None
    df = pd.read_csv(openmp_files[0])
    # Execution_time is already in milliseconds, no conversion needed
    return df


def load_mpi_data(folder_path):
    """Load MPI statistics."""
    mpi_files = list(folder_path.glob("stats_mpi_*.csv"))
    if not mpi_files:
        return None
    df = pd.read_csv(mpi_files[0])
    # MPI Time is in seconds, convert to milliseconds for consistency
    if 'Time' in df.columns:
        df['Time'] = df['Time'] * 1000.0
    return df


def compute_stats(times, percentile=80):
    """Compute statistics using top percentile (fastest times)."""
    threshold = np.percentile(times, 100 - percentile)
    filtered_times = times[times <= threshold]
    return {
        'mean': np.mean(filtered_times),
        'median': np.median(filtered_times),
        'std': np.std(filtered_times),
        'min': np.min(filtered_times),
        'max': np.max(filtered_times),
        'count': len(filtered_times)
    }


def analyze_matrix(matrix_name, base_path, percentile=80):
    """Analyze strong scaling for a single matrix."""
    folder_path = Path(base_path) / matrix_name
    
    if not folder_path.exists():
        print(f"Folder {folder_path} does not exist")
        return None
    
    print(f"\nAnalyzing {matrix_name}...")
    
    results = {
        'matrix_name': matrix_name,
        'serial': None,
        'openmp': [],
        'mpi': []
    }
    
    # Load Serial
    serial_df = load_serial_data(folder_path)
    if serial_df is not None:
        times = serial_df['Execution_time'].values
        stats = compute_stats(times, percentile)
        results['serial'] = {
            'nproc': 1,
            'mean_time': stats['mean'],
            'median_time': stats['median'],
            'std_time': stats['std']
        }
        print(f"  Serial: {stats['mean']:.4f}ms (baseline)")
    
    # Load OpenMP
    openmp_df = load_openmp_data(folder_path)
    if openmp_df is not None and 'Num_Threads' in openmp_df.columns:
        for nthreads in sorted(openmp_df['Num_Threads'].unique()):
            thread_data = openmp_df[openmp_df['Num_Threads'] == nthreads]
            times = thread_data['Execution_time'].values
            if len(times) > 0:
                stats = compute_stats(times, percentile)
                results['openmp'].append({
                    'nproc': int(nthreads),
                    'mean_time': stats['mean'],
                    'median_time': stats['median'],
                    'std_time': stats['std']
                })
        print(f"  OpenMP: {len(results['openmp'])} thread configurations")
    
    # Load MPI
    mpi_df = load_mpi_data(folder_path)
    if mpi_df is not None:
        for nproc in sorted(mpi_df['NProc'].unique()):
            proc_data = mpi_df[mpi_df['NProc'] == nproc]
            
            # Check correctness
            if 'Status' in proc_data.columns:
                correct_runs = proc_data[proc_data['Status'] == 1]
                correctness_rate = (len(correct_runs) / len(proc_data)) * 100 if len(proc_data) > 0 else 0
            else:
                correct_runs = proc_data
                correctness_rate = 100.0
            
            if len(correct_runs) > 0:
                times = correct_runs['Time'].values
                stats = compute_stats(times, percentile)
                
                # Extract additional metrics
                comm_times = correct_runs['CommTime'].values if 'CommTime' in correct_runs.columns else None
                overheads = correct_runs['Overhead'].values if 'Overhead' in correct_runs.columns else None
                
                results['mpi'].append({
                    'nproc': int(nproc),
                    'mean_time': stats['mean'],
                    'median_time': stats['median'],
                    'std_time': stats['std'],
                    'correctness_rate': correctness_rate,
                    'mean_comm_time': np.mean(comm_times) if comm_times is not None else np.nan,
                    'mean_overhead': np.mean(overheads) if overheads is not None else np.nan
                })
        print(f"  MPI: {len(results['mpi'])} processor configurations")
    
    return results


def calculate_speedup_efficiency(data, baseline_time):
    """Calculate speedup and efficiency metrics."""
    if data is None or len(data) == 0:
        return data
    
    for item in data:
        item['speedup'] = baseline_time / item['mean_time']
        item['efficiency'] = (item['speedup'] / item['nproc']) * 100
        item['ideal_time'] = baseline_time / item['nproc']
    
    return data


def plot_strong_scaling(matrix1_results, matrix2_results, output_dir="."):
    """Generate publication-quality strong scaling plots."""
    
    # Set publication style for 1-column format
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
    
    fig, axes = plt.subplots(1, 2, figsize=(7, 2.5))
    
    matrices = [matrix1_results, matrix2_results]
    matrix_names = [m['matrix_name'] for m in matrices]
    
    # Calculate speedup/efficiency for EACH matrix using its OWN baseline
    for matrix in matrices:
        # Get baseline for THIS matrix
        baseline = None
        if matrix['serial'] is not None:
            baseline = matrix['serial']['mean_time']
        elif len(matrix['openmp']) > 0:
            # Find nproc=1 in OpenMP data
            omp_1 = [x for x in matrix['openmp'] if x['nproc'] == 1]
            if omp_1:
                baseline = omp_1[0]['mean_time']
            else:
                baseline = matrix['openmp'][0]['mean_time']
        elif len(matrix['mpi']) > 0:
            # Find nproc=1 in MPI data
            mpi_1 = [x for x in matrix['mpi'] if x['nproc'] == 1]
            if mpi_1:
                baseline = mpi_1[0]['mean_time']
            else:
                baseline = matrix['mpi'][0]['mean_time']
        else:
            baseline = 1.0
        
        # Calculate speedup/efficiency using this matrix's baseline
        matrix['openmp'] = calculate_speedup_efficiency(matrix['openmp'], baseline)
        matrix['mpi'] = calculate_speedup_efficiency(matrix['mpi'], baseline)
    
    # Colors for OpenMP and MPI
    color_omp = '#A23B72'  # Magenta
    color_mpi = '#F18F01'  # Orange
    
    # Plot 1: Matrix 1 speedup
    ax = axes[0]
    m = matrices[0]
    
    if len(m['openmp']) > 0:
        omp_df = pd.DataFrame(m['openmp']).sort_values('nproc')
        ax.plot(omp_df['nproc'], omp_df['speedup'], 
                's-', color=color_omp, label='OpenMP', linewidth=1.5, markersize=5)
    
    if len(m['mpi']) > 0:
        mpi_df = pd.DataFrame(m['mpi']).sort_values('nproc')
        ax.plot(mpi_df['nproc'], mpi_df['speedup'], 
                '^-', color=color_mpi, label='MPI', linewidth=1.5, markersize=5)
    
    # Add ideal speedup line
    max_proc = max([d['nproc'] for d in m['openmp']] + [d['nproc'] for d in m['mpi']]) if (len(m['openmp']) > 0 or len(m['mpi']) > 0) else 256
    ideal_procs = [1, 2, 4, 8, 16, 32, 64, 128, 256]
    ideal_procs = [p for p in ideal_procs if p <= max_proc]
    ax.plot(ideal_procs, ideal_procs, 'k--', linewidth=1.2, alpha=0.7, label='Ideal')
    
    ax.set_xlabel('Number of Processors/Threads')
    ax.set_ylabel('Speedup')
    ax.set_title(f'{matrix_names[0]}')
    ax.set_xscale('log', base=2)
    ax.set_yscale('log', base=2)
    ax.grid(True, linewidth=0.5)
    ax.legend(loc='best', frameon=True, fancybox=False, edgecolor='black', framealpha=0.9)
    
    # Plot 2: Matrix 2 speedup
    ax = axes[1]
    m = matrices[1]
    
    if len(m['openmp']) > 0:
        omp_df = pd.DataFrame(m['openmp']).sort_values('nproc')
        ax.plot(omp_df['nproc'], omp_df['speedup'], 
                's-', color=color_omp, label='OpenMP', linewidth=1.5, markersize=5)
    
    if len(m['mpi']) > 0:
        mpi_df = pd.DataFrame(m['mpi']).sort_values('nproc')
        ax.plot(mpi_df['nproc'], mpi_df['speedup'], 
                '^-', color=color_mpi, label='MPI', linewidth=1.5, markersize=5)
    
    # Add ideal speedup line
    max_proc = max([d['nproc'] for d in m['openmp']] + [d['nproc'] for d in m['mpi']]) if (len(m['openmp']) > 0 or len(m['mpi']) > 0) else 256
    ideal_procs = [1, 2, 4, 8, 16, 32, 64, 128, 256]
    ideal_procs = [p for p in ideal_procs if p <= max_proc]
    ax.plot(ideal_procs, ideal_procs, 'k--', linewidth=1.2, alpha=0.7, label='Ideal')
    
    ax.set_xlabel('Number of Processors/Threads')
    ax.set_ylabel('Speedup')
    ax.set_title(f'{matrix_names[1]}')
    ax.set_xscale('log', base=2)
    ax.set_yscale('log', base=2)
    ax.grid(True, linewidth=0.5)
    ax.legend(loc='best', frameon=True, fancybox=False, edgecolor='black', framealpha=0.9)
    
    # Add main title
    fig.suptitle('Strong Scaling', fontsize=12, fontweight='bold', y=0.99)
    
    plt.tight_layout(rect=[0, 0, 1, 0.94])  # Leave space for suptitle
    output_path = Path(output_dir) / "strong_scaling_analysis.png"
    plt.savefig(output_path, dpi=300, bbox_inches='tight')
    print(f"Plots saved to: {output_path}")
    plt.close()


def generate_text_report(matrix1_results, matrix2_results, output_dir="."):
    """Generate detailed text report."""
    
    output_path = Path(output_dir) / "strong_scaling_report.txt"
    with open(output_path, 'w') as f:
        f.write("="*80 + "\n")
        f.write("STRONG SCALING ANALYSIS REPORT\n")
        f.write(f"Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        f.write("="*80 + "\n\n")
        f.write("Strong Scaling: Fixed problem size, varying processors\n")
        f.write("Ideal: Execution time should decrease linearly with processor count\n\n")
        
        for matrix_results in [matrix1_results, matrix2_results]:
            matrix_name = matrix_results['matrix_name']
            
            f.write("\n" + "="*80 + "\n")
            f.write(f"MATRIX: {matrix_name}\n")
            f.write("="*80 + "\n\n")
            
            # Serial baseline
            if matrix_results['serial']:
                baseline = matrix_results['serial']['mean_time']
                f.write(f"Serial Baseline: {baseline:.4f}ms\n\n")
            else:
                # Find nproc=1 in parallel data
                baseline = None
                if matrix_results['openmp']:
                    omp_1 = [x for x in matrix_results['openmp'] if x['nproc'] == 1]
                    if omp_1:
                        baseline = omp_1[0]['mean_time']
                    else:
                        baseline = matrix_results['openmp'][0]['mean_time']
                elif matrix_results['mpi']:
                    mpi_1 = [x for x in matrix_results['mpi'] if x['nproc'] == 1]
                    if mpi_1:
                        baseline = mpi_1[0]['mean_time']
                    else:
                        baseline = matrix_results['mpi'][0]['mean_time']
                f.write(f"Baseline (from parallel): {baseline:.4f}ms\n\n")
            
            # OpenMP results
            if matrix_results['openmp']:
                f.write("OPENMP RESULTS:\n")
                f.write("-"*80 + "\n")
                f.write(f"{'Threads':<10} {'Time (ms)':<12} {'Speedup':<10} {'Efficiency %':<14} {'Ideal Time (ms)':<16}\n")
                f.write("-"*80 + "\n")
                
                for item in sorted(matrix_results['openmp'], key=lambda x: x['nproc']):
                    f.write(f"{item['nproc']:<10} {item['mean_time']:<12.4f} "
                           f"{item['speedup']:<10.2f} {item['efficiency']:<14.2f} "
                           f"{item['ideal_time']:<16.4f}\n")
                
                # Best performance
                best = max(matrix_results['openmp'], key=lambda x: x['speedup'])
                f.write(f"\nBest OpenMP: {best['nproc']} threads, {best['speedup']:.2f}x speedup\n")
            
            # MPI results
            if matrix_results['mpi']:
                f.write("\n\nMPI RESULTS:\n")
                f.write("-"*80 + "\n")
                f.write(f"{'NProc':<10} {'Time (ms)':<12} {'Speedup':<10} {'Efficiency %':<14} "
                       f"{'Comm Time (s)':<14} {'Overhead %':<12}\n")
                f.write("-"*80 + "\n")
                
                for item in sorted(matrix_results['mpi'], key=lambda x: x['nproc']):
                    f.write(f"{item['nproc']:<10} {item['mean_time']:<12.4f} "
                           f"{item['speedup']:<10.2f} {item['efficiency']:<14.2f} "
                           f"{item.get('mean_comm_time', 0):<14.6f} "
                           f"{item.get('mean_overhead', 0):<12.2f}\n")
                
                # Best performance
                best = max(matrix_results['mpi'], key=lambda x: x['speedup'])
                f.write(f"\nBest MPI: {best['nproc']} processors, {best['speedup']:.2f}x speedup\n")
                
                # Correctness check
                all_correct = all(item.get('correctness_rate', 100) == 100 for item in matrix_results['mpi'])
                f.write(f"Correctness: {'✓ ALL CORRECT' if all_correct else '✗ SOME ERRORS'}\n")
            
            # Summary
            f.write("\n" + "-"*80 + "\n")
            f.write("SUMMARY:\n")
            if matrix_results['openmp'] and matrix_results['mpi']:
                best_omp = max(matrix_results['openmp'], key=lambda x: x['speedup'])
                best_mpi = max(matrix_results['mpi'], key=lambda x: x['speedup'])
                
                f.write(f"  Best OpenMP speedup: {best_omp['speedup']:.2f}x ({best_omp['nproc']} threads)\n")
                f.write(f"  Best MPI speedup: {best_mpi['speedup']:.2f}x ({best_mpi['nproc']} procs)\n")
                
                if best_omp['speedup'] > best_mpi['speedup']:
                    f.write(f"  Winner: OpenMP ({best_omp['speedup']:.2f}x vs {best_mpi['speedup']:.2f}x)\n")
                else:
                    f.write(f"  Winner: MPI ({best_mpi['speedup']:.2f}x vs {best_omp['speedup']:.2f}x)\n")
        
        f.write("\n" + "="*80 + "\n")
        f.write("END OF REPORT\n")
        f.write("="*80 + "\n")
    
    print(f"Text report saved to: {output_path}")


def main():
    if len(sys.argv) < 3:
        print("Usage: python analyze_strong_scaling.py <data_path> <matrix1> <matrix2> [output_dir] [percentile]")
        print("\nExample: python analyze_strong_scaling.py . inline_1 largebasis ./results 80")
        print("\nAvailable matrices: Ga41As41H72, inline_1, largebasis, nd24k, pre2")
        sys.exit(1)
    
    data_path = sys.argv[1]
    matrix1 = sys.argv[2]
    matrix2 = sys.argv[3]
    output_dir = sys.argv[4] if len(sys.argv) > 4 else "."
    percentile = int(sys.argv[5]) if len(sys.argv) > 5 else 80
    
    # Create output directory if it doesn't exist
    Path(output_dir).mkdir(parents=True, exist_ok=True)
    
    # Analyze both matrices
    matrix1_results = analyze_matrix(matrix1, data_path, percentile)
    matrix2_results = analyze_matrix(matrix2, data_path, percentile)
    
    if matrix1_results is None or matrix2_results is None:
        print("Error: Could not analyze matrices")
        sys.exit(1)
    
    # Generate plots only
    plot_strong_scaling(matrix1_results, matrix2_results, output_dir)


if __name__ == "__main__":
    main()
