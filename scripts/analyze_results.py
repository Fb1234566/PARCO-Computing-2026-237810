import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np
import os
import sys
import re
from pathlib import Path

def process_matrix_data(matrix_name, files, output_dir):
    """
    Loads, processes, and plots comprehensive data for a single matrix.

    Args:
        matrix_name (str): The name of the matrix (e.g., "largebasis").
        files (dict): Dictionary with paths to serial and OpenMP variant CSVs.
        output_dir (str): The directory where plots should be saved.
    """
    try:
        print(f"--- Processing matrix: {matrix_name} ---")

        # --- 1. Load Data ---
        df_serial = pd.read_csv(files['serial'])
        variants = {}

        for key, path in files.items():
            if key != 'serial':
                variants[key] = pd.read_csv(path)

        # --- 2. Process Data ---
        avg_serial_time = df_serial['Execution_time'].mean()
        std_serial_time = df_serial['Execution_time'].std()

        processed_data = {}
        all_threads = set()

        for variant_name, df in variants.items():
            grouped = df.groupby('Num_Threads')['Execution_time']
            avg_time = grouped.mean().reset_index()
            std_time = grouped.std().reset_index()

            avg_time['Execution_time_std'] = std_time['Execution_time']
            avg_time['Speedup'] = avg_serial_time / avg_time['Execution_time']
            avg_time['Efficiency'] = avg_time['Speedup'] / avg_time['Num_Threads'] * 100

            # Calculate variance for speedup and efficiency
            avg_time['Speedup_std'] = (std_serial_time / avg_time['Execution_time']) + \
                                      (avg_serial_time * std_time['Execution_time'] / (avg_time['Execution_time']**2))
            avg_time['Efficiency_std'] = avg_time['Speedup_std'] / avg_time['Num_Threads'] * 100

            processed_data[variant_name] = avg_time
            all_threads.update(avg_time['Num_Threads'])

        all_threads = sorted(list(all_threads))

        markers = ['o', 's', '^', 'd', 'v', 'p', '*', 'h']

        # --- 3. Plot 1: Execution Time (Log-Log) with variance ---
        fig, ax = plt.subplots(figsize=(12, 7))

        for idx, (variant_name, data) in enumerate(processed_data.items()):
            ax.errorbar(data['Num_Threads'], data['Execution_time'],
                        yerr=data['Execution_time_std'],
                        label=variant_name.replace('_', ' ').title(),
                        marker=markers[idx % len(markers)], linewidth=2, markersize=8,
                        capsize=5, capthick=2, alpha=0.8)

        ax.axhline(y=avg_serial_time, color='r', linestyle='--',
                   label=f'Serial (Avg: {avg_serial_time:.4f}s ± {std_serial_time:.4f}s)', linewidth=2)

        ax.set_title(f'Log-Log Execution Time Comparison ({matrix_name})', fontsize=14, fontweight='bold')
        ax.set_xlabel('Number of Threads (Log Scale)', fontsize=12)
        ax.set_ylabel('Average Execution Time (s) - Log Scale', fontsize=12)
        ax.set_xscale('log')
        ax.set_yscale('log')
        ax.set_xticks(all_threads)
        ax.xaxis.set_major_formatter(ticker.ScalarFormatter())
        ax.legend(fontsize=9, loc='best')
        ax.grid(True, which='both', linestyle='--', linewidth=0.5, alpha=0.7)
        plt.tight_layout()
        plt.savefig(os.path.join(output_dir, f"plot_time_loglog_{matrix_name}.png"), dpi=300)
        plt.close(fig)

        # --- 4. Plot 2: Execution Time (Linear) with variance ---
        fig, ax = plt.subplots(figsize=(12, 7))

        for idx, (variant_name, data) in enumerate(processed_data.items()):
            ax.errorbar(data['Num_Threads'], data['Execution_time'],
                        yerr=data['Execution_time_std'],
                        label=variant_name.replace('_', ' ').title(),
                        marker=markers[idx % len(markers)], linewidth=2, markersize=8,
                        capsize=5, capthick=2, alpha=0.8)

        ax.axhline(y=avg_serial_time, color='r', linestyle='--',
                   label=f'Serial (Avg: {avg_serial_time:.4f}s)', linewidth=2)

        ax.set_title(f'Execution Time Comparison ({matrix_name})', fontsize=14, fontweight='bold')
        ax.set_xlabel('Number of Threads', fontsize=12)
        ax.set_ylabel('Average Execution Time (s)', fontsize=12)
        ax.set_xticks(all_threads)
        ax.legend(fontsize=9, loc='best')
        ax.grid(True, linestyle='--', linewidth=0.5, alpha=0.7)
        plt.tight_layout()
        plt.savefig(os.path.join(output_dir, f"plot_time_linear_{matrix_name}.png"), dpi=300)
        plt.close(fig)

        # --- 5. Plot 3: Speedup (Linear) with variance ---
        fig, ax = plt.subplots(figsize=(12, 7))

        for idx, (variant_name, data) in enumerate(processed_data.items()):
            ax.errorbar(data['Num_Threads'], data['Speedup'],
                        yerr=data['Speedup_std'],
                        label=variant_name.replace('_', ' ').title(),
                        marker=markers[idx % len(markers)], linewidth=2, markersize=8,
                        capsize=5, capthick=2, alpha=0.8)

        ax.plot(all_threads, all_threads, color='k', linestyle=':',
                label='Ideal Speedup', linewidth=2)

        ax.set_title(f'Speedup Analysis ({matrix_name})', fontsize=14, fontweight='bold')
        ax.set_xlabel('Number of Threads', fontsize=12)
        ax.set_ylabel('Speedup ($T_{serial} / T_{parallel}$)', fontsize=12)
        ax.set_xticks(all_threads)
        ax.legend(fontsize=9, loc='best')
        ax.grid(True, linestyle='--', linewidth=0.5, alpha=0.7)
        plt.tight_layout()
        plt.savefig(os.path.join(output_dir, f"plot_speedup_{matrix_name}.png"), dpi=300)
        plt.close(fig)

        # --- 6. Plot 3b: Speedup (Log-Log) ---
        fig, ax = plt.subplots(figsize=(12, 7))

        for idx, (variant_name, data) in enumerate(processed_data.items()):
            ax.plot(data['Num_Threads'], data['Speedup'],
                    label=variant_name.replace('_', ' ').title(),
                    marker=markers[idx % len(markers)], linewidth=2, markersize=8)

        ax.plot(all_threads, all_threads, color='k', linestyle=':',
                label='Ideal Speedup', linewidth=2)

        ax.set_title(f'Log-Log Speedup Analysis ({matrix_name})', fontsize=14, fontweight='bold')
        ax.set_xlabel('Number of Threads (Log Scale)', fontsize=12)
        ax.set_ylabel('Speedup (Log Scale)', fontsize=12)
        ax.set_xscale('log')
        ax.set_yscale('log')
        ax.set_xticks(all_threads)
        ax.xaxis.set_major_formatter(ticker.ScalarFormatter())
        ax.yaxis.set_major_formatter(ticker.ScalarFormatter())
        ax.legend(fontsize=9, loc='best')
        ax.grid(True, which='both', linestyle='--', linewidth=0.5, alpha=0.7)
        plt.tight_layout()
        plt.savefig(os.path.join(output_dir, f"plot_speedup_loglog_{matrix_name}.png"), dpi=300)
        plt.close(fig)

        # --- 7. Plot 4: Efficiency (Linear) with variance ---
        fig, ax = plt.subplots(figsize=(12, 7))

        for idx, (variant_name, data) in enumerate(processed_data.items()):
            ax.errorbar(data['Num_Threads'], data['Efficiency'],
                        yerr=data['Efficiency_std'],
                        label=variant_name.replace('_', ' ').title(),
                        marker=markers[idx % len(markers)], linewidth=2, markersize=8,
                        capsize=5, capthick=2, alpha=0.8)

        ax.axhline(y=100, color='k', linestyle=':', label='Ideal Efficiency (100%)', linewidth=2)

        ax.set_title(f'Parallel Efficiency ({matrix_name})', fontsize=14, fontweight='bold')
        ax.set_xlabel('Number of Threads', fontsize=12)
        ax.set_ylabel('Efficiency (%)', fontsize=12)
        ax.set_xticks(all_threads)
        ax.set_ylim([0, 110])
        ax.legend(fontsize=9, loc='best')
        ax.grid(True, linestyle='--', linewidth=0.5, alpha=0.7)
        plt.tight_layout()
        plt.savefig(os.path.join(output_dir, f"plot_efficiency_{matrix_name}.png"), dpi=300)
        plt.close(fig)

        # --- 8. Plot 4b: Efficiency (Semi-Log) ---
        fig, ax = plt.subplots(figsize=(12, 7))

        for idx, (variant_name, data) in enumerate(processed_data.items()):
            ax.plot(data['Num_Threads'], data['Efficiency'],
                    label=variant_name.replace('_', ' ').title(),
                    marker=markers[idx % len(markers)], linewidth=2, markersize=8)

        ax.axhline(y=100, color='k', linestyle=':', label='Ideal Efficiency (100%)', linewidth=2)

        ax.set_title(f'Efficiency (Semi-Log) ({matrix_name})', fontsize=14, fontweight='bold')
        ax.set_xlabel('Number of Threads (Log Scale)', fontsize=12)
        ax.set_ylabel('Efficiency (%)', fontsize=12)
        ax.set_xscale('log')
        ax.set_xticks(all_threads)
        ax.xaxis.set_major_formatter(ticker.ScalarFormatter())
        ax.set_ylim([0, 110])
        ax.legend(fontsize=9, loc='best')
        ax.grid(True, which='both', linestyle='--', linewidth=0.5, alpha=0.7)
        plt.tight_layout()
        plt.savefig(os.path.join(output_dir, f"plot_efficiency_logx_{matrix_name}.png"), dpi=300)
        plt.close(fig)

        # --- 9. Plot 5: Speedup Bar Chart ---
        fig, ax = plt.subplots(figsize=(14, 7))
        x = np.arange(len(all_threads))
        width = 0.8 / len(processed_data)

        for idx, (variant_name, data) in enumerate(processed_data.items()):
            speedups = [data[data['Num_Threads'] == t]['Speedup'].values[0]
                        if t in data['Num_Threads'].values else 0 for t in all_threads]
            offset = (idx - len(processed_data)/2 + 0.5) * width
            ax.bar(x + offset, speedups, width,
                   label=variant_name.replace('_', ' ').title())

        ax.plot(x, all_threads, 'k:', linewidth=2, label='Ideal Speedup')

        ax.set_title(f'Speedup Comparison by Thread Count ({matrix_name})', fontsize=14, fontweight='bold')
        ax.set_xlabel('Number of Threads', fontsize=12)
        ax.set_ylabel('Speedup', fontsize=12)
        ax.set_xticks(x)
        ax.set_xticklabels(all_threads)
        ax.legend(fontsize=9, loc='best')
        ax.grid(True, axis='y', linestyle='--', linewidth=0.5, alpha=0.7)
        plt.tight_layout()
        plt.savefig(os.path.join(output_dir, f"plot_speedup_bar_{matrix_name}.png"), dpi=300)
        plt.close(fig)

        # --- 10. Plot 6: Heatmap Comparison ---
        fig, ax = plt.subplots(figsize=(12, max(6, len(processed_data) * 0.8)))

        speedup_matrix = []
        labels = []

        for variant_name, data in processed_data.items():
            speedups = [data[data['Num_Threads'] == t]['Speedup'].values[0]
                        if t in data['Num_Threads'].values else np.nan for t in all_threads]
            speedup_matrix.append(speedups)
            labels.append(variant_name.replace('_', ' ').title())

        im = ax.imshow(speedup_matrix, cmap='RdYlGn', aspect='auto', vmin=0, vmax=max(all_threads))

        ax.set_xticks(np.arange(len(all_threads)))
        ax.set_yticks(np.arange(len(labels)))
        ax.set_xticklabels(all_threads)
        ax.set_yticklabels(labels)
        ax.set_xlabel('Number of Threads', fontsize=12)
        ax.set_title(f'Speedup Heatmap ({matrix_name})', fontsize=14, fontweight='bold')

        for i in range(len(labels)):
            for j in range(len(all_threads)):
                if not np.isnan(speedup_matrix[i][j]):
                    text = ax.text(j, i, f'{speedup_matrix[i][j]:.1f}',
                                   ha="center", va="center", color="black", fontsize=9)

        plt.colorbar(im, ax=ax, label='Speedup')
        plt.tight_layout()
        plt.savefig(os.path.join(output_dir, f"plot_heatmap_{matrix_name}.png"), dpi=300)
        plt.close(fig)

        # --- 11. Print Statistics ---
        print(f"\n=== Statistics for {matrix_name} ===")
        print(f"Serial time: {avg_serial_time:.6f}s ± {std_serial_time:.6f}s\n")

        for variant_name, data in processed_data.items():
            max_speedup = data['Speedup'].max()
            max_speedup_threads = data.loc[data['Speedup'].idxmax(), 'Num_Threads']
            best_efficiency = data['Efficiency'].max()
            best_efficiency_threads = data.loc[data['Efficiency'].idxmax(), 'Num_Threads']

            print(f"{variant_name.replace('_', ' ').title()}:")
            print(f"  Max Speedup: {max_speedup:.2f}x at {max_speedup_threads:.0f} threads")
            print(f"  Best Efficiency: {best_efficiency:.2f}% at {best_efficiency_threads:.0f} threads\n")

        return {
            'stats': {name: (data['Speedup'].max(), data['Efficiency'].max())
                      for name, data in processed_data.items()},
            'data': processed_data,
            'serial_time': avg_serial_time
        }

    except Exception as e:
        print(f"ERROR: Failed to process matrix {matrix_name}. Reason: {e}")
        import traceback
        traceback.print_exc()
        return {}

def create_comparison_plots(all_matrix_data, output_dir):
    """Creates comprehensive comparison plots for all matrices."""
    try:
        print("\n--- Creating comparison plots across all matrices ---")

        matrices = list(all_matrix_data.keys())
        if not matrices:
            return

        # Get all variant names and thread counts
        all_variants = set()
        all_threads = set()
        for matrix_data in all_matrix_data.values():
            if 'data' in matrix_data:
                all_variants.update(matrix_data['data'].keys())
                for variant_data in matrix_data['data'].values():
                    all_threads.update(variant_data['Num_Threads'])

        all_variants = sorted(list(all_variants))
        all_threads = sorted(list(all_threads))
        markers = ['o', 's', '^', 'd', 'v', 'p', '*', 'h']

        # --- 1. Speedup comparison for specific thread count (e.g., 32 threads) ---
        target_threads = [8, 16, 32]
        for target_thread in target_threads:
            if target_thread not in all_threads:
                continue

            fig, ax = plt.subplots(figsize=(14, 7))
            x = np.arange(len(matrices))
            width = 0.8 / len(all_variants)

            for idx, variant in enumerate(all_variants):
                speedups = []
                for matrix in matrices:
                    if variant in all_matrix_data[matrix].get('data', {}):
                        data = all_matrix_data[matrix]['data'][variant]
                        val = data[data['Num_Threads'] == target_thread]['Speedup']
                        speedups.append(val.values[0] if len(val) > 0 else 0)
                    else:
                        speedups.append(0)

                offset = (idx - len(all_variants)/2 + 0.5) * width
                ax.bar(x + offset, speedups, width,
                       label=variant.replace('_', ' ').title())

            ax.axhline(y=target_thread, color='k', linestyle=':', linewidth=2, label=f'Ideal Speedup ({target_thread}x)')
            ax.set_title(f'Speedup Comparison at {target_thread} Threads', fontsize=14, fontweight='bold')
            ax.set_xlabel('Matrix', fontsize=12)
            ax.set_ylabel('Speedup', fontsize=12)
            ax.set_xticks(x)
            ax.set_xticklabels(matrices, rotation=45, ha='right')
            ax.legend(fontsize=9, loc='best')
            ax.grid(True, axis='y', linestyle='--', linewidth=0.5, alpha=0.7)
            plt.tight_layout()
            plt.savefig(os.path.join(output_dir, f"comparison_speedup_{target_thread}threads.png"), dpi=300)
            plt.close(fig)

        # --- 2. Speedup curves for each variant across all matrices ---
        for variant in all_variants:
            fig, ax = plt.subplots(figsize=(14, 8))

            for idx, matrix in enumerate(matrices):
                if variant in all_matrix_data[matrix].get('data', {}):
                    data = all_matrix_data[matrix]['data'][variant]
                    ax.plot(data['Num_Threads'], data['Speedup'],
                            label=matrix,
                            marker=markers[idx % len(markers)], linewidth=2, markersize=8)

            ax.plot(all_threads, all_threads, 'k:', linewidth=2, label='Ideal Speedup')
            ax.set_title(f'Speedup Comparison - {variant.replace("_", " ").title()}', fontsize=14, fontweight='bold')
            ax.set_xlabel('Number of Threads', fontsize=12)
            ax.set_ylabel('Speedup', fontsize=12)
            ax.set_xticks(all_threads)
            ax.legend(fontsize=9, loc='best')
            ax.grid(True, linestyle='--', linewidth=0.5, alpha=0.7)
            plt.tight_layout()
            plt.savefig(os.path.join(output_dir, f"comparison_speedup_curves_{variant}.png"), dpi=300)
            plt.close(fig)

        # --- 3. Efficiency curves for each variant across all matrices ---
        for variant in all_variants:
            fig, ax = plt.subplots(figsize=(14, 8))

            for idx, matrix in enumerate(matrices):
                if variant in all_matrix_data[matrix].get('data', {}):
                    data = all_matrix_data[matrix]['data'][variant]
                    ax.plot(data['Num_Threads'], data['Efficiency'],
                            label=matrix,
                            marker=markers[idx % len(markers)], linewidth=2, markersize=8)

            ax.axhline(y=100, color='k', linestyle=':', linewidth=2, label='Ideal Efficiency (100%)')
            ax.set_title(f'Efficiency Comparison - {variant.replace("_", " ").title()}', fontsize=14, fontweight='bold')
            ax.set_xlabel('Number of Threads', fontsize=12)
            ax.set_ylabel('Efficiency (%)', fontsize=12)
            ax.set_xticks(all_threads)
            ax.set_ylim([0, 110])
            ax.legend(fontsize=9, loc='best')
            ax.grid(True, linestyle='--', linewidth=0.5, alpha=0.7)
            plt.tight_layout()
            plt.savefig(os.path.join(output_dir, f"comparison_efficiency_curves_{variant}.png"), dpi=300)
            plt.close(fig)

        # --- 4. Execution time comparison (normalized to serial) ---
        for target_thread in [8, 16, 32]:
            if target_thread not in all_threads:
                continue

            fig, ax = plt.subplots(figsize=(14, 7))
            x = np.arange(len(matrices))
            width = 0.8 / len(all_variants)

            for idx, variant in enumerate(all_variants):
                norm_times = []
                for matrix in matrices:
                    serial_time = all_matrix_data[matrix].get('serial_time', 1)
                    if variant in all_matrix_data[matrix].get('data', {}):
                        data = all_matrix_data[matrix]['data'][variant]
                        val = data[data['Num_Threads'] == target_thread]['Execution_time']
                        norm_times.append(val.values[0] / serial_time if len(val) > 0 else 1)
                    else:
                        norm_times.append(1)

                offset = (idx - len(all_variants)/2 + 0.5) * width
                ax.bar(x + offset, norm_times, width,
                       label=variant.replace('_', ' ').title())

            ax.axhline(y=1.0, color='r', linestyle='--', linewidth=2, label='Serial Time')
            ax.set_title(f'Normalized Execution Time at {target_thread} Threads', fontsize=14, fontweight='bold')
            ax.set_xlabel('Matrix', fontsize=12)
            ax.set_ylabel('Time / Serial Time', fontsize=12)
            ax.set_xticks(x)
            ax.set_xticklabels(matrices, rotation=45, ha='right')
            ax.legend(fontsize=9, loc='best')
            ax.grid(True, axis='y', linestyle='--', linewidth=0.5, alpha=0.7)
            plt.tight_layout()
            plt.savefig(os.path.join(output_dir, f"comparison_normalized_time_{target_thread}threads.png"), dpi=300)
            plt.close(fig)

        # --- 5. Maximum speedup achieved by each variant for each matrix ---
        fig, ax = plt.subplots(figsize=(max(12, len(matrices) * 1.2), 7))
        x = np.arange(len(matrices))
        width = 0.8 / len(all_variants)

        for idx, variant in enumerate(all_variants):
            max_speedups = []
            for matrix in matrices:
                if variant in all_matrix_data[matrix].get('stats', {}):
                    max_speedups.append(all_matrix_data[matrix]['stats'][variant][0])
                else:
                    max_speedups.append(0)

            offset = (idx - len(all_variants)/2 + 0.5) * width
            ax.bar(x + offset, max_speedups, width,
                   label=variant.replace('_', ' ').title())

        ax.set_title('Maximum Speedup Achieved Across All Matrices', fontsize=14, fontweight='bold')
        ax.set_xlabel('Matrix', fontsize=12)
        ax.set_ylabel('Maximum Speedup', fontsize=12)
        ax.set_xticks(x)
        ax.set_xticklabels(matrices, rotation=45, ha='right')
        ax.legend(fontsize=9, loc='best')
        ax.grid(True, axis='y', linestyle='--', linewidth=0.5, alpha=0.7)
        plt.tight_layout()
        plt.savefig(os.path.join(output_dir, "comparison_max_speedup.png"), dpi=300)
        plt.close(fig)

        # --- 6. Best efficiency achieved by each variant for each matrix ---
        fig, ax = plt.subplots(figsize=(max(12, len(matrices) * 1.2), 7))

        for idx, variant in enumerate(all_variants):
            best_efficiencies = []
            for matrix in matrices:
                if variant in all_matrix_data[matrix].get('stats', {}):
                    best_efficiencies.append(all_matrix_data[matrix]['stats'][variant][1])
                else:
                    best_efficiencies.append(0)

            offset = (idx - len(all_variants)/2 + 0.5) * width
            ax.bar(x + offset, best_efficiencies, width,
                   label=variant.replace('_', ' ').title())

        ax.axhline(y=100, color='k', linestyle=':', linewidth=1, label='Ideal (100%)')
        ax.set_title('Best Efficiency Across All Matrices', fontsize=14, fontweight='bold')
        ax.set_xlabel('Matrix', fontsize=12)
        ax.set_ylabel('Best Efficiency (%)', fontsize=12)
        ax.set_xticks(x)
        ax.set_xticklabels(matrices, rotation=45, ha='right')
        ax.legend(fontsize=9, loc='best')
        ax.grid(True, axis='y', linestyle='--', linewidth=0.5, alpha=0.7)
        plt.tight_layout()
        plt.savefig(os.path.join(output_dir, "comparison_best_efficiency.png"), dpi=300)
        plt.close(fig)

        # --- 7. Heatmap of speedups at maximum threads ---
        max_thread = max(all_threads)
        fig, ax = plt.subplots(figsize=(14, max(6, len(all_variants) * 0.8)))

        speedup_matrix = []
        for variant in all_variants:
            speedups = []
            for matrix in matrices:
                if variant in all_matrix_data[matrix].get('data', {}):
                    data = all_matrix_data[matrix]['data'][variant]
                    val = data[data['Num_Threads'] == max_thread]['Speedup']
                    speedups.append(val.values[0] if len(val) > 0 else 0)
                else:
                    speedups.append(0)
            speedup_matrix.append(speedups)

        im = ax.imshow(speedup_matrix, cmap='RdYlGn', aspect='auto', vmin=0, vmax=max_thread)

        ax.set_xticks(np.arange(len(matrices)))
        ax.set_yticks(np.arange(len(all_variants)))
        ax.set_xticklabels(matrices, rotation=45, ha='right')
        ax.set_yticklabels([v.replace('_', ' ').title() for v in all_variants])
        ax.set_xlabel('Matrix', fontsize=12)
        ax.set_title(f'Speedup Heatmap at {max_thread} Threads', fontsize=14, fontweight='bold')

        for i in range(len(all_variants)):
            for j in range(len(matrices)):
                text = ax.text(j, i, f'{speedup_matrix[i][j]:.1f}',
                               ha="center", va="center", color="black", fontsize=9)

        plt.colorbar(im, ax=ax, label='Speedup')
        plt.tight_layout()
        plt.savefig(os.path.join(output_dir, f"comparison_heatmap_{max_thread}threads.png"), dpi=300)
        plt.close(fig)

        print("Saved all comparison plots")

    except Exception as e:
        print(f"ERROR: Failed to create comparison plots. Reason: {e}")
        import traceback
        traceback.print_exc()

def main():
    if len(sys.argv) != 3:
        print("Usage: python analyze_results.py <results_run_folder> <plots_run_folder>")
        sys.exit(1)

    results_run_folder = Path(sys.argv[1])
    plots_run_folder = Path(sys.argv[2])

    if not results_run_folder.is_dir():
        print(f"Error: Results path '{results_run_folder}' is not a valid directory.")
        sys.exit(1)

    print(f"Scanning directory: {results_run_folder}")

    # Scan for matrix subdirectories
    matrix_dirs = [d for d in results_run_folder.iterdir() if d.is_dir()]

    if not matrix_dirs:
        print("No matrix subdirectories found.")
        sys.exit(1)

    print(f"Found {len(matrix_dirs)} matrix directories")

    plots_run_folder.mkdir(parents=True, exist_ok=True)
    print(f"\nSaving plots to: {plots_run_folder}")

    all_matrix_data = {}

    for matrix_dir in matrix_dirs:
        matrix_name = matrix_dir.name
        print(f"\nProcessing matrix directory: {matrix_name}")

        matrix_files = {}

        for csv_file in matrix_dir.glob("*.csv"):
            filename = csv_file.name

            # Classify file type
            if 'stats_serial_' in filename:
                matrix_files['serial'] = str(csv_file)
            elif 'Binning' in filename:
                matrix_files['openmp_binning'] = str(csv_file)
            elif 'Dynamic' in filename:
                matrix_files['openmp_dynamic'] = str(csv_file)
            elif 'Guided' in filename:
                matrix_files['openmp_guided'] = str(csv_file)
            elif 'Static' in filename:
                matrix_files['openmp_static'] = str(csv_file)

        if 'serial' in matrix_files and len(matrix_files) > 1:
            # Create matrix-specific plot directory
            matrix_plot_dir = plots_run_folder / matrix_name
            matrix_plot_dir.mkdir(parents=True, exist_ok=True)

            result = process_matrix_data(matrix_name, matrix_files, str(matrix_plot_dir))
            if result:
                all_matrix_data[matrix_name] = result
        else:
            print(f"Warning: Skipping matrix '{matrix_name}'. Missing serial or OpenMP files.")

    if all_matrix_data:
        # Create comparison plots in the main plots folder
        create_comparison_plots(all_matrix_data, str(plots_run_folder))

    print("\n=== Batch analysis complete ===")

if __name__ == "__main__":
    main()
