#!/bin/bash
set -euo pipefail

DATA_DIR="datasets"

make clean
make serial-only
make openmp-only
make openmp-binning-only

for f in "$DATA_DIR"/*; do
  if [[ -f "$f" ]]; then
    abs_path=$(realpath "$f")
    qsub -v MATRIX="$abs_path" Run.pbs
    echo "Submitted job for $abs_path"
  fi
done