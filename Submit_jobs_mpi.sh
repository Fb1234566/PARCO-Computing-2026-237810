#!/bin/bash
set -euo pipefail

DATA_DIR="datasets"

# Load modules required for MPI runs and Python analysis
module load gcc91
module load mpich-3.2.1--gcc-9.1.0
module load python-3.10.14

# Verify mpicc is available (module should provide it)
if ! command -v mpicc >/dev/null 2>&1; then
    echo "Error: mpicc not found in PATH. Make sure the MPI module is available." >&2
    exit 1
fi

MPI_SIZES=(1 2 4 8 16 32 64 128 256)


# Build MPI target and synthetic matrix generator
make clean
make mpi
make serial-only
make synth

# Create a Python virtual environment (for analysis) and install minimal deps
if [ ! -d "./.venv" ]; then
  python3 -m venv .venv
fi
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
mkdir -p "$DATA_DIR"

# Submit one job per matrix (loop over MPI_SIZES happens inside the PBS job)
for f in "$DATA_DIR"/*.mtx; do
  if [[ -f "$f" ]]; then
    abs_path=$(realpath "$f")
    matrix_name=$(basename "$f" | sed 's/\.[^.]*$//')

    output_dir="$WORKDIR/results/run_${RUN_TIMESTAMP}/${matrix_name}/"
    graph_dir="$WORKDIR/plots/run_${RUN_TIMESTAMP}/${matrix_name}/"

    mkdir -p "$output_dir"
    mkdir -p "$graph_dir"

    qsub -v MATRIX="$abs_path",OUTPUT_DIR="$output_dir",GRAPH_DIR="$graph_dir",MPI_SIZES="${MPI_SIZES[*]}" Run_MPI.pbs
    echo "Submitted job for $matrix_name with all MPI_SIZES"
  fi
done

echo "All jobs submitted successfully."
