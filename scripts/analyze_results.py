import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np
import os
import sys
import re

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

        processed_data = {}
        all_threads = set()

        for variant_name, df in variants.items():
            avg_time = df.groupby('Num_Threads')['Execution_time'].mean().reset_index()
            avg_time['Speedup'] = avg_serial_time / avg_time['Execution_time']
            avg_time['Efficiency'] = avg_time['Speedup'] / avg_time['Num_Threads'] * 100
            processed_data[variant_name] = avg_time
            all_threads.update(avg_time['Num_Threads'])

        all_threads = sorted(list(all_threads))

        markers = ['o', 's', '^', 'd', 'v', 'p', '*', 'h']

        # --- 3. Plot 1: Execution Time (Log-Log) ---
        fig, ax = plt.subplots(figsize=(12, 7))

        for idx, (variant_name, data) in enumerate(processed_data.items()):
            ax.plot(data['Num_Threads'], data['Execution_time'],
                    label=variant_name.replace('_', ' ').title(),
                    marker=markers[idx % len(markers)], linewidth=2, markersize=8)

        ax.axhline(y=avg_serial_time, color='r', linestyle='--',
                   label=f'Serial (Avg: {avg_serial_time:.4f}s)', linewidth=2)

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

        # --- 4. Plot 2: Execution Time (Linear) ---
        fig, ax = plt.subplots(figsize=(12, 7))

        for idx, (variant_name, data) in enumerate(processed_data.items()):
            ax.plot(data['Num_Threads'], data['Execution_time'],
                    label=variant_name.replace('_', ' ').title(),
                    marker=markers[idx % len(markers)], linewidth=2, markersize=8)

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

        # --- 5. Plot 3: Speedup (Linear) ---
        fig, ax = plt.subplots(figsize=(12, 7))

        for idx, (variant_name, data) in enumerate(processed_data.items()):
            ax.plot(data['Num_Threads'], data['Speedup'],
                    label=variant_name.replace('_', ' ').title(),
                    marker=markers[idx % len(markers)], linewidth=2, markersize=8)

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

        # --- 7. Plot 4: Efficiency (Linear) ---
        fig, ax = plt.subplots(figsize=(12, 7))

        for idx, (variant_name, data) in enumerate(processed_data.items()):
            ax.plot(data['Num_Threads'], data['Efficiency'],
                    label=variant_name.replace('_', ' ').title(),
                    marker=markers[idx % len(markers)], linewidth=2, markersize=8)

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
        print(f"Serial time: {avg_serial_time:.6f}s\n")

        for variant_name, data in processed_data.items():
            max_speedup = data['Speedup'].max()
            max_speedup_threads = data.loc[data['Speedup'].idxmax(), 'Num_Threads']
            best_efficiency = data['Efficiency'].max()
            best_efficiency_threads = data.loc[data['Efficiency'].idxmax(), 'Num_Threads']

            print(f"{variant_name.replace('_', ' ').title()}:")
            print(f"  Max Speedup: {max_speedup:.2f}x at {max_speedup_threads:.0f} threads")
            print(f"  Best Efficiency: {best_efficiency:.2f}% at {best_efficiency_threads:.0f} threads\n")

        return {name: (data['Speedup'].max(), data['Efficiency'].max())
                for name, data in processed_data.items()}

    except Exception as e:
        print(f"ERROR: Failed to process matrix {matrix_name}. Reason: {e}")
        import traceback
        traceback.print_exc()
        return {}

def create_summary_plots(all_matrix_stats, output_dir):
    """Creates summary plots comparing all matrices and variants."""
    try:
        print("\n--- Creating summary plots ---")

        matrices = list(all_matrix_stats.keys())
        if not matrices:
            return

        # Get all variant names
        all_variants = set()
        for stats in all_matrix_stats.values():
            all_variants.update(stats.keys())
        all_variants = sorted(list(all_variants))

        # --- Summary 1: Max Speedup Comparison ---
        fig, ax = plt.subplots(figsize=(max(12, len(matrices) * 1.2), 7))
        x = np.arange(len(matrices))
        width = 0.8 / len(all_variants)

        for idx, variant in enumerate(all_variants):
            speedups = [all_matrix_stats[matrix].get(variant, (0, 0))[0]
                        for matrix in matrices]
            offset = (idx - len(all_variants)/2 + 0.5) * width
            ax.bar(x + offset, speedups, width,
                   label=variant.replace('_', ' ').title())

        ax.set_title('Maximum Speedup Across All Matrices', fontsize=14, fontweight='bold')
        ax.set_xlabel('Matrix', fontsize=12)
        ax.set_ylabel('Maximum Speedup', fontsize=12)
        ax.set_xticks(x)
        ax.set_xticklabels(matrices, rotation=45, ha='right')
        ax.legend(fontsize=9, loc='best')
        ax.grid(True, axis='y', linestyle='--', linewidth=0.5, alpha=0.7)
        plt.tight_layout()
        plt.savefig(os.path.join(output_dir, "summary_max_speedup.png"), dpi=300)
        plt.close(fig)

        # --- Summary 2: Best Efficiency Comparison ---
        fig, ax = plt.subplots(figsize=(max(12, len(matrices) * 1.2), 7))

        for idx, variant in enumerate(all_variants):
            efficiencies = [all_matrix_stats[matrix].get(variant, (0, 0))[1]
                            for matrix in matrices]
            offset = (idx - len(all_variants)/2 + 0.5) * width
            ax.bar(x + offset, efficiencies, width,
                   label=variant.replace('_', ' ').title())

        ax.set_title('Best Efficiency Across All Matrices', fontsize=14, fontweight='bold')
        ax.set_xlabel('Matrix', fontsize=12)
        ax.set_ylabel('Best Efficiency (%)', fontsize=12)
        ax.set_xticks(x)
        ax.set_xticklabels(matrices, rotation=45, ha='right')
        ax.axhline(y=100, color='k', linestyle=':', linewidth=1, label='Ideal (100%)')
        ax.legend(fontsize=9, loc='best')
        ax.grid(True, axis='y', linestyle='--', linewidth=0.5, alpha=0.7)
        plt.tight_layout()
        plt.savefig(os.path.join(output_dir, "summary_best_efficiency.png"), dpi=300)
        plt.close(fig)

        print("Saved summary plots")

    except Exception as e:
        print(f"ERROR: Failed to create summary plots. Reason: {e}")
        import traceback
        traceback.print_exc()

def main():
    if len(sys.argv) != 3:
        print("Usage: python analyze_results.py <input_folder_path> <output_folder_path>")
        sys.exit(1)

    input_folder_path = sys.argv[1]
    output_folder_path = sys.argv[2]

    if not os.path.isdir(input_folder_path):
        print(f"Error: Input path '{input_folder_path}' is not a valid directory.")
        sys.exit(1)

    print(f"Scanning directory: {input_folder_path}")

    matrix_files = {}

    for filename in os.listdir(input_folder_path):
        if not filename.endswith('.csv'):
            continue

        # Extract matrix name - it's always after "datasets_" and before ".mtx.csv"
        match = re.search(r'datasets_(.+?)\.mtx\.csv', filename)
        if not match:
            continue

        matrix_name = match.group(1)
        full_path = os.path.join(input_folder_path, filename)

        if matrix_name not in matrix_files:
            matrix_files[matrix_name] = {}

        # Classify file type
        if 'stats_serial_' in filename:
            matrix_files[matrix_name]['serial'] = full_path
        elif 'Binning' in filename:
            matrix_files[matrix_name]['openmp_binning'] = full_path
        elif 'Dynamic' in filename:
            matrix_files[matrix_name]['openmp_dynamic'] = full_path
        elif 'Guided' in filename:
            matrix_files[matrix_name]['openmp_guided'] = full_path
        elif 'Static' in filename:
            matrix_files[matrix_name]['openmp_static'] = full_path

    if not matrix_files:
        print("No valid result files found. Check filenames.")
        return

    print(f"Found {len(matrix_files)} matrices")
    for matrix_name, files in matrix_files.items():
        print(f"  {matrix_name}: {list(files.keys())}")

    os.makedirs(output_folder_path, exist_ok=True)
    print(f"\nSaving plots to: {output_folder_path}")

    all_matrix_stats = {}

    for matrix_name, files in matrix_files.items():
        if 'serial' in files and len(files) > 1:
            stats = process_matrix_data(matrix_name, files, output_folder_path)
            if stats:
                all_matrix_stats[matrix_name] = stats
        else:
            print(f"Warning: Skipping matrix '{matrix_name}'. Missing serial or OpenMP files.")

    if all_matrix_stats:
        create_summary_plots(all_matrix_stats, output_folder_path)

    print("\n=== Batch analysis complete ===")

if __name__ == "__main__":
    main()
