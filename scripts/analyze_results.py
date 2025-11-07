import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import os
import sys
import re

def process_matrix_data(matrix_name, files, output_dir):
    """
    Loads, processes, and plots data for a single matrix.

    Args:
        matrix_name (str): The name of the matrix (e.g., "crankseg_2").
        files (dict): A dictionary with paths to serial, openmp, and binning CSVs.
        output_dir (str): The directory where plots should be saved.
    """
    try:
        print(f"--- Processing matrix: {matrix_name} ---")

        # --- 1. Load Data ---
        df_serial = pd.read_csv(files['serial'])
        df_openmp = pd.read_csv(files['openmp'])
        df_binning = pd.read_csv(files['binning'])

        # --- 2. Process Data ---

        # Calculate average serial time
        avg_serial_time = df_serial['Execution_time'].mean()

        # Calculate average times for parallel versions
        avg_openmp_time = df_openmp.groupby('Num_Threads')['Execution_time'].mean().reset_index()
        avg_binning_time = df_binning.groupby('Num_Threads')['Execution_time'].mean().reset_index()

        # Calculate speedup
        avg_openmp_time['Speedup'] = avg_serial_time / avg_openmp_time['Execution_time']
        avg_binning_time['Speedup'] = avg_serial_time / avg_binning_time['Execution_time']

        # Get all unique thread counts for x-axis ticks
        all_threads = sorted(list(set(avg_openmp_time['Num_Threads']).union(set(avg_binning_time['Num_Threads']))))

        # --- 3. Plot 1: Execution Time (Log-Log) ---
        fig, ax = plt.subplots(figsize=(10, 6))

        # Plot data
        ax.plot(avg_openmp_time['Num_Threads'], avg_openmp_time['Execution_time'],
                label='OpenMP (Standard)', marker='s')
        ax.plot(avg_binning_time['Num_Threads'], avg_binning_time['Execution_time'],
                label='OpenMP (Binning)', marker='o')
        ax.axhline(y=avg_serial_time, color='r', linestyle='--',
                   label=f'Serial (Avg: {avg_serial_time:.2f}s)')

        # Customize plot
        ax.set_title(f'Log-Log Execution Time Comparison ({matrix_name})')
        ax.set_xlabel('Number of Threads (Log Scale)')
        ax.set_ylabel('Average Execution Time (s) - Log Scale')

        # Set scales to logarithmic
        ax.set_xscale('log')
        ax.set_yscale('log')

        # Set x-ticks to be the actual thread counts
        ax.set_xticks(all_threads)

        # Format x-ticks as plain numbers (e.g., "64" not "10^1.8")
        ax.xaxis.set_major_formatter(ticker.ScalarFormatter())

        ax.legend()
        ax.grid(True, which='both', linestyle='--', linewidth=0.5)

        # Save plot
        time_plot_filename = os.path.join(output_dir, f"plot_time_loglog_{matrix_name}.png")
        plt.savefig(time_plot_filename)
        plt.close(fig)
        print(f"Saved plot: {time_plot_filename}")

        # --- 4. Plot 2: Speedup (Linear) ---
        plt.figure(figsize=(10, 6))

        # Plot data
        plt.plot(avg_openmp_time['Num_Threads'], avg_openmp_time['Speedup'],
                 label='Speedup: OpenMP (Standard)', marker='s')
        plt.plot(avg_binning_time['Num_Threads'], avg_binning_time['Speedup'],
                 label='Speedup: OpenMP (Binning)', marker='o')
        plt.plot(all_threads, all_threads, color='k', linestyle=':',
                 label='Ideal Speedup')

        # Customize plot
        plt.title(f'Speedup Analysis ({matrix_name})')
        plt.xlabel('Number of Threads')
        plt.ylabel('Speedup ($T_{\text{serial}} / T_{\text{parallel}}$)')
        plt.xticks(all_threads)
        plt.legend()
        plt.grid(True, which='both', linestyle='--', linewidth=0.5)

        # Save plot
        speedup_plot_filename = os.path.join(output_dir, f"plot_speedup_{matrix_name}.png")
        plt.savefig(speedup_plot_filename)
        plt.close()
        print(f"Saved plot: {speedup_plot_filename}")

    except Exception as e:
        print(f"ERROR: Failed to process matrix {matrix_name}. Reason: {e}")

def main():
    # --- 1. Get Folder Paths from Command Line ---
    if len(sys.argv) != 3:
        print("Usage: python analyze_results.py <input_folder_path> <output_folder_path>")
        sys.exit(1)

    input_folder_path = sys.argv[1]
    output_folder_path = sys.argv[2]

    if not os.path.isdir(input_folder_path):
        print(f"Error: Input path '{input_folder_path}' is not a valid directory.")
        sys.exit(1)

    print(f"Scanning directory: {input_folder_path}")

    # --- 2. Scan and Group Files ---
    # This regex captures the type and the matrix name
    # It assumes the structure "stats_TYPE_SpMV_datasets_MATRIX.mtx.csv"
    pattern = re.compile(r"stats_(.*?)_SpMV_datasets_(.*?).mtx.csv")

    matrix_files = {}

    for filename in os.listdir(input_folder_path):
        match = pattern.match(filename)
        if match:
            file_type = match.group(1)
            matrix_name = match.group(2)
            full_path = os.path.join(input_folder_path, filename)

            # Initialize dictionary for this matrix if it's new
            if matrix_name not in matrix_files:
                matrix_files[matrix_name] = {}

            # Assign file path to the correct type
            if 'Binning' in file_type:
                matrix_files[matrix_name]['binning'] = full_path
            elif file_type == 'serial':
                matrix_files[matrix_name]['serial'] = full_path
            elif 'OpenMP' in file_type or 'openMP' in file_type:
                matrix_files[matrix_name]['openmp'] = full_path
            else:
                print(f"Warning: Unrecognized file type '{file_type}' for {filename}")

    # --- 3. Process Each Group ---
    if not matrix_files:
        print("No valid result files found. Check filenames.")
        return

    # Create the output directory if it doesn't exist
    os.makedirs(output_folder_path, exist_ok=True)
    print(f"Saving plots to: {output_folder_path}")

    for matrix_name, files in matrix_files.items():
        # Check if we have all three required files for a matrix
        if 'serial' in files and 'openmp' in files and 'binning' in files:
            process_matrix_data(matrix_name, files, output_folder_path)
        else:
            print(f"Warning: Skipping matrix '{matrix_name}'. Missing one or more files.")
            print(f"  Found: {list(files.keys())}")

    print("\nBatch analysis complete.")

if __name__ == "__main__":
    main()