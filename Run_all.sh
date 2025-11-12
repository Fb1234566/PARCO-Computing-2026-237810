#!/bin/bash
set -euo pipefail

DATA_DIR="datasets"

make clean
make serial-only
make openmp-static-only
make openmp-binning-only
make openmp-dynamic-only

module load gcc91
module load python-3.10.14

gcc() {
    gcc-9.1.0 "$@"
}

python3 -m venv .venv
source ./.venv/bin/activate
pip install --upgrade pip
pip install numpy matplotlib pandas


for f in "$DATA_DIR"/*; do
  if [[ -f "$f" ]]; then
    abs_path=$(realpath "$f")
    qsub -v MATRIX="$abs_path" Run.pbs
    echo "Submitted job for $abs_path"
  fi
done