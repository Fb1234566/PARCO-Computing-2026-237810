#!/bin/bash
set -euo pipefail

DATA_DIR="datasets"

make clean
make serial-only
make openmp-static-only
make openmp-binning-only
make openmp-dynamic-only
make openmp-guided-only

module load gcc91
module load python-3.10.14

gcc() {
    gcc-9.1.0 "$@"
}

python3 -m venv .venv
source ./.venv/bin/activate
pip install --upgrade pip
pip install numpy matplotlib pandas

# Create timestamp for this run
RUN_TIMESTAMP=$(date +"%Y%m%d_%H%M%S")

# Create base directories for this run
mkdir -p "results/run_${RUN_TIMESTAMP}"
mkdir -p "plots/run_${RUN_TIMESTAMP}"

for f in "$DATA_DIR"/*; do
  if [[ -f "$f" ]]; then
    abs_path=$(realpath "$f")
    matrix_name=$(basename "$f" | sed 's/\.[^.]*$//')

    # Create directory for this matrix under results and plots
    output_dir="results/run_${RUN_TIMESTAMP}/${matrix_name}"
    graph_dir="plots/run_${RUN_TIMESTAMP}/${matrix_name}"

    mkdir -p "$output_dir"
    mkdir -p "$graph_dir"

    qsub -v MATRIX="$abs_path",OUTPUT_DIR="$output_dir",GRAPH_DIR="$graph_dir",PBS_O_WORKDIR="$PWD" Run.pbs
    echo "Submitted job for $abs_path -> $output_dir"
  fi
done
