#!/bin/bash
set -euo pipefail

DATA_DIR="datasets"
RUN_SCRIPT="./Run.sh"

# 1. Controlla se lo script Run.sh esiste ed è eseguibile
if [ ! -x "$RUN_SCRIPT" ]; then
    echo "Errore: Lo script '$RUN_SCRIPT' non è stato trovato o non è eseguibile." >&2
    echo "Assicurati che sia nella stessa directory e abbia i permessi di esecuzione (chmod +x $RUN_SCRIPT)." >&2
    exit 1
fi

# 2. Compila i progetti
echo "Pulizia e compilazione dei progetti..."
make clean
make serial-only
make openmp-static-only
make openmp-binning-only
make openmp-dynamic-only
make openmp-guided-only
echo "Compilazione completata."

# 3. Imposta l'ambiente Python
echo "Impostazione dell'ambiente virtuale Python..."
if [ ! -d ".venv" ]; then
    python3 -m venv .venv
fi
source ./.venv/bin/activate
pip install --upgrade pip
pip install numpy matplotlib pandas scipy
echo "Ambiente Python pronto."

# 4. Crea directory di output con timestamp
RUN_TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
WORKDIR="$(pwd)"
BASE_RESULTS_DIR="$WORKDIR/results/run_${RUN_TIMESTAMP}"
BASE_PLOTS_DIR="$WORKDIR/plots/run_${RUN_TIMESTAMP}"

mkdir -p "$BASE_RESULTS_DIR"
mkdir -p "$BASE_PLOTS_DIR"

echo "I risultati saranno salvati in: $BASE_RESULTS_DIR"
echo "I grafici saranno salvati in: $BASE_PLOTS_DIR"

# 5. Esegui i test per ogni matrice nella directory dei dati
for f in "$DATA_DIR"/*; do
  if [[ -f "$f" ]]; then
    matrix_name=$(basename "$f" | sed 's/\.[^.]*$//')
    echo "-----------------------------------------------------"
    echo "Inizio test per la matrice: $matrix_name"
    echo "-----------------------------------------------------"

    # Crea directory specifiche per la matrice
    output_dir="$BASE_RESULTS_DIR/${matrix_name}/"
    graph_dir="$BASE_PLOTS_DIR/${matrix_name}/"
    mkdir -p "$output_dir"
    mkdir -p "$graph_dir"

    # Esegui lo script Run.sh per la matrice corrente
    # Passa il percorso della matrice, la directory dei risultati e la directory dei grafici
    "$RUN_SCRIPT" "$f" "$output_dir" "$graph_dir"

    echo "Test completati per la matrice: $matrix_name"
  fi
done

deactivate
echo "-----------------------------------------------------"
echo "Tutti i test sono stati completati."
echo "-----------------------------------------------------"
