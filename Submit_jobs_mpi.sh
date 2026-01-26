#!/bin/bash
set -euo pipefail

DATA_DIR="datasets"

# Build only MPI target
make clean
make mpi

# Load modules required for MPI runs and Python analysis
module load gcc91
module load mpich-3.2.1--gcc-9.1.0
module load python-3.10.14

mpicc() {
    mpicc-3.2.1 "${@}"
}

python3 -m venv .venv
source ./.venv/bin/activate
pip install --upgrade pip
pip install numpy matplotlib pandas scipy

# Create timestamp for this run
RUN_TIMESTAMP=$(date +"%Y%m%d_%H%M%S")

# Get absolute path of current directory
WORKDIR="$(pwd)"

# Create base directories for this run
mkdir -p "$WORKDIR/results/run_${RUN_TIMESTAMP}"
mkdir -p "$WORKDIR/plots/run_${RUN_TIMESTAMP}"

for f in "$DATA_DIR"/*; do
  if [[ -f "$f" ]]; then
    abs_path=$(realpath "$f")
    matrix_name=$(basename "$f" | sed 's/\.[^.]*$//')

    # Use absolute paths
    output_dir="$WORKDIR/results/run_${RUN_TIMESTAMP}/${matrix_name}/"
    graph_dir="$WORKDIR/plots/run_${RUN_TIMESTAMP}/${matrix_name}/"

    mkdir -p "$output_dir"
    mkdir -p "$graph_dir"

    qsub -v MATRIX="$abs_path",OUTPUT_DIR="$output_dir",GRAPH_DIR="$graph_dir" Run_MPI.pbs
    echo "Submitted MPI job for $abs_path -> $output_dir"
  fi
done

