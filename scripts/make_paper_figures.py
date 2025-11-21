import os
import sys
from pathlib import Path
import numpy as np
import matplotlib.pyplot as plt
import pandas as pd


def process_matrix_data(matrix_name, files, plot_dir):
    """
    Carica i dati della matrice, calcola lo speedup basato sul 90° percentile
    e restituisce i dati elaborati.
    """
    try:
        # Carica i dati seriali e calcola il tempo di esecuzione del 90° percentile.
        serial_df = pd.read_csv(files["serial"])
        # Assicurati che ci siano abbastanza dati per il calcolo del percentile.
        if len(serial_df['Execution_time']) < 10:
            print(f"Warning for matrix {matrix_name}: Serial runs are less than 10, percentile may not be accurate.")
        serial_time = serial_df['Execution_time'].quantile(0.9)

        if pd.isna(serial_time) or serial_time == 0:
            print(f"Error processing matrix {matrix_name}: Invalid serial time ({serial_time}).")
            return None

        results = {"data": {}}
        variants = sorted([k for k in files.keys() if k != "serial"])

        for variant in variants:
            df = pd.read_csv(files[variant])

            # Assicurati che ci siano abbastanza dati per il calcolo del percentile.
            if len(df) > 0 and len(df.groupby('Num_Threads').head(1)) * 10 > len(df):
                print(
                    f"Warning for matrix {matrix_name}, variant {variant}: Some thread counts have less than 10 runs.")

            # Calcola il 90° percentile del tempo di esecuzione per ogni numero di thread.
            percentile_times = df.groupby("Num_Threads")["Execution_time"].quantile(0.9)

            # Calcola lo speedup usando il tempo del 90° percentile.
            speedup = serial_time / percentile_times

            variant_data = pd.DataFrame({
                "Num_Threads": speedup.index,
                "Speedup": speedup.values
            })

            results["data"][variant] = variant_data

        return results

    except Exception as e:
        print(f"Error processing matrix {matrix_name}: {e}")
        return None


def load_all_matrices(results_run_folder: Path, plots_run_folder: Path):
    all_matrix_data = {}

    for matrix_dir in results_run_folder.iterdir():
        if not matrix_dir.is_dir():
            continue

        matrix_name = matrix_dir.name
        matrix_files = {}

        for csv_file in matrix_dir.glob("*.csv"):
            filename = csv_file.name
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
                variant_data = data_dict[v]
                speedup_at_target = variant_data[variant_data["Num_Threads"] == target_threads]["Speedup"]
                if not speedup_at_target.empty:
                    row.append(speedup_at_target.iloc[0])
                else:
                    row.append(np.nan)
            else:
                row.append(np.nan)
        speedup_matrix.append(row)

    speedup_matrix = np.array(speedup_matrix)

    fig, ax = plt.subplots(figsize=(3.5, 2.6), dpi=300)

    vmax = max(target_threads, float(np.nanmax(speedup_matrix)) if speedup_matrix.size > 0 and np.any(
        ~np.isnan(speedup_matrix)) else target_threads)
    im = ax.imshow(speedup_matrix, cmap="RdYlGn", aspect="auto", vmin=0, vmax=vmax)

    ax.set_xticks(np.arange(len(variants)))
    ax.set_yticks(np.arange(len(matrices)))
    ax.set_xticklabels(variant_labels, fontsize=8, rotation=45, ha="right")
    ax.set_yticklabels(matrices, fontsize=8)

    ax.set_xlabel("Scheduler", fontsize=9)
    ax.set_ylabel("Matrix", fontsize=9)
    ax.set_title(f"Speedup (90th Percentile) at {target_threads} threads", fontsize=9)

    for i in range(len(matrices)):
        for j in range(len(variants)):
            if not np.isnan(speedup_matrix[i, j]):
                ax.text(j, i, f"{speedup_matrix[i, j]:.2f}", ha="center", va="center", color="black", fontsize=7)

    cbar = plt.colorbar(im, ax=ax)
    cbar.ax.tick_params(labelsize=7)
    cbar.set_label("Speedup", fontsize=8)

    plt.tight_layout()
    output_dir.mkdir(parents=True, exist_ok=True)
    fig_path = output_dir / f"fig1_heatmap_speedup_{target_threads}threads.png"
    plt.savefig(fig_path, dpi=300)
    plt.close(fig)
    print(f"Saved Figure 1 (heatmap) to {fig_path}")


def make_figure2_scaling_imbalance_overhead(
        all_matrix_data,
        output_dir: Path,
        hard_matrix_name: str = "Ga41As41H72",
        overhead_matrix_name: str = "largebasis",
):
    """
    Genera due subplot:
     (a) hard_matrix_name - Imbalance Test
     (b) overhead_matrix_name - Overhead Test

    Se mancano dati reali per una matrice o per una variante, viene sollevata una RuntimeError.
    """
    variants = ["openmp_static", "openmp_binning", "openmp_guided", "openmp_dynamic"]
    variant_labels = ["Static", "Binning", "Guided", "Dynamic"]
    markers = ["o", "s", "^", "d"]
    colors = ["#1f77b4", "#2ca02c", "#d62728", "#9467bd"]

    # Controllo presenza matrici
    for m in (hard_matrix_name, overhead_matrix_name):
        if m not in all_matrix_data:
            raise RuntimeError(f"Matrice `{m}` mancante in all_matrix_data; aborting (no synthetic data).")

    fig, axes = plt.subplots(1, 2, figsize=(7.0, 2.6), dpi=300, sharey=True)
    (ax_hard, ax_overhead) = axes

    def plot_side(ax, matrix_name, title_short):
        data_dict = all_matrix_data[matrix_name].get("data", {})
        # Verifica che tutte le varianti richieste esistano
        missing = [v for v in variants if v not in data_dict]
        if missing:
            raise RuntimeError(
                f"Mancano varianti per matrice `{matrix_name}`: {missing}; aborting (no synthetic data).")

        max_threads = 1
        for idx, (v, label) in enumerate(zip(variants, variant_labels)):
            df = data_dict[v]
            if df.empty:
                raise RuntimeError(f"DataFrame vuoto per variante `{v}` sulla matrice `{matrix_name}`; aborting.")
            # Ordina per Num_Threads per sicurezza
            ordered = df.sort_values("Num_Threads")
            threads = ordered["Num_Threads"].values
            speedup = ordered["Speedup"].values
            ax.plot(threads, speedup, marker=markers[idx], markersize=4, linestyle='-', label=label, color=colors[idx])
            max_threads = max(max_threads, int(np.nanmax(threads)))

        if max_threads > 1:
            ax.plot([1, max_threads], [1, max_threads], 'k:', label='Ideal')

        ax.set_title(title_short, fontsize=9)
        ax.set_xlabel("Threads", fontsize=9)
        ax.set_xscale('linear')
        ax.grid(True, linestyle="--", linewidth=0.4, alpha=0.7)
        ax.tick_params(axis="both", labelsize=8)
        ax.set_xlim(1, max_threads)
        ax.set_ylim(bottom=0.9)

    plot_side(ax_hard, hard_matrix_name, f"(a) {hard_matrix_name} (Imbalance Test)")
    ax_hard.set_ylabel("Speedup (90th Percentile)", fontsize=9)

    plot_side(ax_overhead, overhead_matrix_name, f"(b) {overhead_matrix_name} (Overhead Test)")

    handles, labels = ax_hard.get_legend_handles_labels()
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
    fig_path = output_dir / "fig2_scaling_imbalance_overhead.png"
    plt.savefig(fig_path, dpi=300)
    plt.close(fig)
    print(f"Saved Figure 2 (scaling easy vs hard) to {fig_path}")


# python
def _read_mtx_nnz(mtx_path: Path) -> int:
    """
    Legge un file Matrix Market `.mtx` e ritorna il campo NNZ
    (terzo valore della prima riga non-commento).
    """
    with mtx_path.open("r", encoding="utf-8", errors="ignore") as f:
        for line in f:
            s = line.strip()
            if not s or s.startswith('%'):
                continue
            parts = s.split()
            if len(parts) >= 3:
                try:
                    return int(parts[2])
                except ValueError:
                    return int(float(parts[2]))
    raise ValueError(f"Unable to parse NNZ from {mtx_path}")


def load_datasets_nnzs(datasets_folder: Path) -> dict:
    """
    Legge solo file Matrix Market `*.mtx` in `datasets_folder` e
    restituisce dict {matrix_name: nnz} (intero). Solleva RuntimeError
    se non trova file `.mtx` validi.
    """
    if not datasets_folder.exists() or not datasets_folder.is_dir():
        raise RuntimeError(f"Dataset folder `{datasets_folder}` non valida.")

    nnz_map = {}
    for f in sorted(datasets_folder.glob("*.mtx")):
        try:
            nnz = _read_mtx_nnz(f)
            nnz_map[f.stem] = int(nnz)
        except Exception:
            continue

    if not nnz_map:
        raise RuntimeError(f"Nessun file Matrix Market `.mtx` valido trovato in `{datasets_folder}`.")

    return nnz_map


def make_figure2_weak_scaling_memory_bound(
        all_matrix_data,
        output_dir: Path,
        datasets_folder: Path,
        target_threads: int = 32,
):
    """
    Weak-scaling: X = NNZ (da `datasets_folder` .mtx), Y = Speedup a `target_threads`.
    Usa solo dati reali presenti in `all_matrix_data` (solleva RuntimeError se mancanti).
    """
    # Carica mappa name -> nnz dai .mtx
    nnz_map = load_datasets_nnzs(datasets_folder)

    # mappa case-insensitive per trovare corrispondenze
    lower_to_real = {k.lower(): k for k in nnz_map.keys()}

    # Intersezione: solo matrici per cui abbiamo sia dati sperimentali che file .mtx
    available = []
    for m in all_matrix_data.keys():
        if m in nnz_map or m.lower() in lower_to_real:
            available.append(m)
    if not available:
        raise RuntimeError("Nessuna matrice presente sia in `all_matrix_data` che in `datasets` (.mtx).")

    # Ordina matrici per NNZ crescente
    def _nnz_for_name(m):
        if m in nnz_map:
            return nnz_map[m]
        return nnz_map[lower_to_real[m.lower()]]

    matrices = sorted(available, key=_nnz_for_name)

    variants = ["openmp_static", "openmp_binning", "openmp_guided", "openmp_dynamic"]
    variant_labels = ["Static", "Binning", "Guided", "Dynamic"]
    markers = ["o", "s", "^", "d"]
    colors = ["#1f77b4", "#2ca02c", "#d62728", "#9467bd"]

    x_nnz = []
    data_by_variant = {v: [] for v in variants}

    for m in matrices:
        # ricava nnz
        if m in nnz_map:
            nnz = nnz_map[m]
        else:
            nnz = nnz_map[lower_to_real[m.lower()]]
        x_nnz.append(nnz / 1e6)  # milioni di NNZ

        data_dict = all_matrix_data[m].get("data", {})
        # Verifica presenza di tutte le varianti richieste
        missing = [v for v in variants if v not in data_dict]
        if missing:
            raise RuntimeError(f"Variante/i {missing} mancante per matrice `{m}`; aborting (no synthetic data).")

        for v in variants:
            df_v = data_dict[v]
            val = df_v[df_v["Num_Threads"] == target_threads]["Speedup"]
            if val.empty:
                raise RuntimeError(f"Speedup a {target_threads} threads non trovato per variante `{v}` su matrice `{m}`.")
            data_by_variant[v].append(float(val.iloc[0]))

    # Plot
    fig, ax = plt.subplots(figsize=(5.5, 3.2), dpi=300)
    for idx, (v, label) in enumerate(zip(variants, variant_labels)):
        y = np.array(data_by_variant[v])
        ax.plot(x_nnz, y, marker=markers[idx], markersize=5, linestyle='-', label=label, color=colors[idx])

    ax.set_xlabel("Problem size (NNZ, milioni)", fontsize=9)
    ax.set_ylabel(f"Speedup (90th percentile) at {target_threads} threads", fontsize=9)
    ax.set_title("Weak-Scaling Analysis (Memory-Bound Bottleneck)", fontsize=9)
    ax.grid(True, linestyle="--", linewidth=0.4, alpha=0.7)
    ax.tick_params(labelsize=8)
    if x_nnz:
        ax.set_xlim(min(x_nnz) * 0.95, max(x_nnz) * 1.05)
    ax.set_ylim(bottom=0)
    ax.legend(fontsize=8, frameon=False)
    plt.tight_layout()

    output_dir.mkdir(parents=True, exist_ok=True)
    fig_path = output_dir / "fig2_weak_scaling_memory_bound.png"
    plt.savefig(fig_path, dpi=300)
    plt.close(fig)
    print(f"Saved Figure 2 (weak-scaling memory-bound) to {fig_path}")

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

    make_figure2_scaling_imbalance_overhead(
        all_matrix_data,
        paper_figures_dir,
        hard_matrix_name="Ga41As41H72",
        overhead_matrix_name="largebasis",
    )

    datasets_folder = Path("./datasets")
    make_figure2_weak_scaling_memory_bound(
        all_matrix_data,
        paper_figures_dir,
        datasets_folder=datasets_folder,
        target_threads=32,
    )


print("=== Paper figures generation complete ===")

if __name__ == "__main__":
    main()
