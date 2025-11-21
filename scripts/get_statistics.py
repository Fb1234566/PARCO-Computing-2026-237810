import sys
from pathlib import Path
import pandas as pd

TARGET_THREAD = 32
VAR_ORDER = ['Static', 'Dynamic', 'Guided', 'Binning']

def classify_csvs(folder: Path):
    files = {}
    for csv in folder.glob("*.csv"):
        name = csv.name
        if 'stats_serial_' in name:
            files['serial'] = csv
        elif 'Static' in name:
            files['Static'] = csv
        elif 'Dynamic' in name:
            files['Dynamic'] = csv
        elif 'Guided' in name:
            files['Guided'] = csv
        elif 'Binning' in name:
            files['Binning'] = csv
    return files

def process_matrix(matrix_name: str, files: dict, target_thread: int = TARGET_THREAD):
    if 'serial' not in files:
        return None
    try:
        serial_df = pd.read_csv(files['serial'])
        # I tempi sono già in ms, quindi non moltiplichiamo
        serial_90 = float(serial_df['Execution_time'].quantile(0.9))
    except Exception:
        return None

    row = {'Matrix': matrix_name, 'Serial_ms': serial_90}
    for var in VAR_ORDER:
        speed_key = f"{var}_speedup"
        ms_key = f"{var}_ms"
        row[speed_key] = None
        row[ms_key] = None

        if var in files:
            try:
                df = pd.read_csv(files[var])
                grouped = df.groupby('Num_Threads')['Execution_time'].quantile(0.9).reset_index()
                val = grouped.loc[grouped['Num_Threads'] == target_thread, 'Execution_time']
                if len(val) == 0:
                    continue
                var_90 = float(val.values[0])
                var_ms = var_90  # già in ms
                speedup = (serial_90 / var_90) if var_90 > 0 else None
                row[speed_key] = speedup
                row[ms_key] = var_ms
            except Exception:
                row[speed_key] = None
                row[ms_key] = None
    return row

def main():
    if len(sys.argv) != 3:
        print("Usage: python `scripts/get_statistics.py` <results_matrix_folder> <output_folder>")
        sys.exit(1)

    results_matrix_folder = Path(sys.argv[1])
    output_folder = Path(sys.argv[2])

    if not results_matrix_folder.is_dir():
        print(f"Error: Results path `{results_matrix_folder}` is not a valid directory.")
        sys.exit(1)

    output_folder.mkdir(parents=True, exist_ok=True)
    results_run_folder = results_matrix_folder.parent

    summaries = []
    candidate_dirs = [results_matrix_folder] + [d for d in results_run_folder.iterdir() if d.is_dir() and d != results_matrix_folder]

    for mat_dir in candidate_dirs:
        files = classify_csvs(mat_dir)
        if 'serial' in files and any(k in files for k in VAR_ORDER):
            res = process_matrix(mat_dir.name, files)
            if res:
                summaries.append(res)

    if not summaries:
        print("No valid matrices found.")
        sys.exit(0)

    # Build markdown table header:
    header = ["Matrix", "Serial (ms)"]
    for var in VAR_ORDER:
        header.append(f"{var} Speedup")
        header.append(f"{var} (ms)")

    md_lines = []
    md_lines.append("| " + " | ".join(header) + " |")
    md_lines.append("|" + "|".join(["---"] * len(header)) + "|")

    for r in sorted(summaries, key=lambda x: x['Matrix']):
        row_vals = []
        # Matrix name
        row_vals.append(r['Matrix'])
        # Serial ms
        serial_ms = r.get('Serial_ms')
        row_vals.append(f"{float(serial_ms):.2f}" if serial_ms is not None else "-")

        # For each variant add speedup and ms
        for var in VAR_ORDER:
            speed = r.get(f"{var}_speedup")
            ms = r.get(f"{var}_ms")
            row_vals.append(f"{float(speed):.2f}" if (speed is not None) else "-")
            row_vals.append(f"{float(ms):.2f}" if (ms is not None) else "-")

        md_lines.append("| " + " | ".join(row_vals) + " |")

    md_content = "# Speedup a 32 Threads (90th percentile)\n\n" + "\n".join(md_lines) + "\n"

    out_file = output_folder / "results_summary.md"
    out_file.write_text(md_content)
    print(f"Wrote summary to `{out_file}`")
    print(md_content)

if __name__ == "__main__":
    main()
