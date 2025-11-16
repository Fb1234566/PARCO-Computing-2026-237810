import os
import sys
from pathlib import Path
import numpy as np
import matplotlib.pyplot as plt

from analyze_results import process_matrix_data


def load_all_matrices(results_run_folder: Path, plots_run_folder: Path):
    all_matrix_data = {}

    for matrix_dir in results_run_folder.iterdir():
        if not matrix_dir.is_dir():
            continue

        matrix_name = matrix_dir.name
        matrix_files = {}

        for csv_file in matrix_dir.glob("*.csv"):
            filename = csv_file.name

            if "stats_serial_" in filename:
                matrix_files["serial"] = str(csv_file)
            elif "Binning" in filename:
                matrix_files["openmp_binning"] = str(csv_file)
            elif "Dynamic" in filename:
                matrix_files["openmp_dynamic"] = str(csv_file)
            elif "Guided" in filename:
                matrix_files["openmp_guided"] = str(csv_file)
            elif "Static" in filename:
                matrix_files["openmp_static"] = str(csv_file)

        if "serial" in matrix_files and len(matrix_files) > 1:
            plot_dir = plots_run_folder / matrix_name
            plot_dir.mkdir(parents=True, exist_ok=True)

            result = process_matrix_data(matrix_name, matrix_files, str(plot_dir))
            if result:
                all_matrix_data[matrix_name] = result

    print(f"Loaded matrices for paper figures: {sorted(list(all_matrix_data.keys()))}")
    return all_matrix_data


def make_figure1_heatmap_32(all_matrix_data, output_dir: Path, target_threads: int = 32):
    matrices = sorted(all_matrix_data.keys())
    variants = ["openmp_static", "openmp_binning", "openmp_guided", "openmp_dynamic"]
    variant_labels = ["Static", "Binning", "Guided", "Dynamic"]

    speedup_matrix = []
    for matrix in matrices:
        row = []
        data_dict = all_matrix_data[matrix].get("data", {})
        for v in variants:
            if v in data_dict:
                df = data_dict[v]
                val = df[df["Num_Threads"] == target_threads]["Speedup"]
                row.append(float(val.values[0]) if len(val) > 0 else 0.0)
            else:
                row.append(0.0)
        speedup_matrix.append(row)

    speedup_matrix = np.array(speedup_matrix)

    fig, ax = plt.subplots(figsize=(3.5, 2.6), dpi=300)

    vmax = max(target_threads, float(np.nanmax(speedup_matrix)) if speedup_matrix.size else target_threads)
    im = ax.imshow(speedup_matrix, cmap="RdYlGn", aspect="auto", vmin=0, vmax=vmax)

    ax.set_xticks(np.arange(len(variants)))
    ax.set_yticks(np.arange(len(matrices)))
    ax.set_xticklabels(variant_labels, fontsize=8, rotation=45, ha="right")
    ax.set_yticklabels(matrices, fontsize=8)

    ax.set_xlabel("Scheduler", fontsize=9)
    ax.set_ylabel("Matrix", fontsize=9)
    ax.set_title(f"Speedup at {target_threads} Threads", fontsize=9)

    for i in range(len(matrices)):
        for j in range(len(variants)):
            text_val = f"{speedup_matrix[i, j]:.1f}"
            ax.text(
                j,
                i,
                text_val,
                ha="center",
                va="center",
                color="black",
                fontsize=7,
            )

    cbar = plt.colorbar(im, ax=ax)
    cbar.ax.tick_params(labelsize=7)
    cbar.set_label("Speedup", fontsize=8)

    plt.tight_layout()
    output_dir.mkdir(parents=True, exist_ok=True)
    fig_path = output_dir / f"fig1_heatmap_speedup_{target_threads}threads.png"
    plt.savefig(fig_path, dpi=300)
    plt.close(fig)
    print(f"Saved Figure 1 (heatmap) to {fig_path}")


def make_figure2_scaling_easy_hard(
        all_matrix_data,
        output_dir: Path,
        easy_matrix_name: str,
        hard_matrix_name: str,
):
    variants = ["openmp_static", "openmp_binning", "openmp_guided", "openmp_dynamic"]
    variant_labels = ["Static", "Binning", "Guided", "Dynamic"]
    markers = ["o", "s", "^", "d"]

    if easy_matrix_name not in all_matrix_data:
        print(f"Warning: easy matrix `{easy_matrix_name}` not found in {list(all_matrix_data.keys())}; skipping Figure 2.")
        return
    if hard_matrix_name not in all_matrix_data:
        print(f"Warning: hard matrix `{hard_matrix_name}` not found in {list(all_matrix_data.keys())}; skipping Figure 2.")
        return

    fig, axes = plt.subplots(1, 2, figsize=(7.0, 2.6), dpi=300, sharey=True)
    (ax_easy, ax_hard) = axes

    def plot_matrix(ax, matrix_name, title_short):
        data_dict = all_matrix_data[matrix_name]["data"]

        max_threads = 0
        for idx, (v, label) in enumerate(zip(variants, variant_labels)):
            if v not in data_dict:
                continue
            df = data_dict[v].sort_values("Num_Threads")
            if not df.empty:
                max_threads = max(max_threads, df["Num_Threads"].max())
            ax.plot(
                df["Num_Threads"],
                df["Speedup"],
                marker=markers[idx % len(markers)],
                linewidth=1.0,
                markersize=3.5,
                label=label,
            )

        if max_threads > 0:
            ideal_threads = np.arange(1, max_threads + 1)
            ax.plot(
                ideal_threads,
                ideal_threads,
                "k--",
                linewidth=0.8,
                label="Ideal",
            )

        ax.set_title(title_short, fontsize=9)
        ax.set_xlabel("Threads", fontsize=9)
        ax.grid(True, linestyle="--", linewidth=0.4, alpha=0.7)
        ax.tick_params(axis="both", labelsize=8)

    plot_matrix(ax_easy, easy_matrix_name, f"(a) {easy_matrix_name}")
    ax_easy.set_ylabel("Speedup", fontsize=9)

    plot_matrix(ax_hard, hard_matrix_name, f"(b) {hard_matrix_name}")

    handles, labels = ax_easy.get_legend_handles_labels()
    fig.legend(
        handles,
        labels,
        loc="upper center",
        bbox_to_anchor=(0.5, 1.02),
        ncol=len(labels),
        fontsize=8,
        frameon=False,
    )

    plt.tight_layout(rect=[0, 0, 1, 0.95])
    output_dir.mkdir(parents=True, exist_ok=True)
    fig_path = output_dir / "fig2_scaling_easy_vs_hard.png"
    plt.savefig(fig_path, dpi=300)
    plt.close(fig)
    print(f"Saved Figure 2 (scaling easy vs hard) to {fig_path}")


def main():
    if len(sys.argv) != 3:
        print("Usage: python scripts/make_paper_figures.py <results_run_folder> <plots_run_folder>")
        sys.exit(1)

    results_run_folder = Path(sys.argv[1])
    plots_run_folder = Path(sys.argv[2])

    if not results_run_folder.is_dir():
        print(f"Error: results path `{results_run_folder}` is not a valid directory.")
        sys.exit(1)


    plots_run_folder = plots_run_folder.parent

    paper_figures_dir = plots_run_folder / "paper_figures"
    paper_figures_dir.mkdir(parents=True, exist_ok=True)
    results_run_folder = results_run_folder.parent
    all_matrix_data = load_all_matrices(results_run_folder, plots_run_folder)

    if not all_matrix_data:
        print("Error: no matrix data found.")
        sys.exit(1)

    make_figure1_heatmap_32(all_matrix_data, paper_figures_dir, target_threads=32)

    # Sostituisci i nomi qui con quelli esatti che vedi in output da load_all_matrices
    make_figure2_scaling_easy_hard(
        all_matrix_data,
        paper_figures_dir,
        easy_matrix_name="nd24k",        # <-- metti il nome reale
        hard_matrix_name="Ga41As41H72",  # <-- metti il nome reale se diverso
    )

    print("=== Paper figures generation complete ===")


if __name__ == "__main__":
    main()
