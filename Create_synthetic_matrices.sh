#!/bin/bash
set -euo pipefail

# Script to generate synthetic matrices for weak scaling analysis
# Usage: ./Create_synthetic_matrices.sh [MPI_SIZES...]
# If no arguments provided, defaults to: 1 2 4 8 16 32 64 128 256

DATA_DIR="datasets"

# Use provided MPI sizes or defaults
if [ $# -eq 0 ]; then
    MPI_SIZES=(1 2 4 8 16 32 64 128 256)
else
    MPI_SIZES=("$@")
fi

# Ensure datasets directory exists
mkdir -p "$DATA_DIR"

# Build synthetic matrix generator if needed
if [ ! -f "bin/create_synthetic_matrices" ]; then
    echo "Building synthetic matrix generator..."
    make synth
fi

# Generate synthetic matrices for weak scaling
echo "Generating synthetic matrices for weak scaling analysis..."
cd "$DATA_DIR"
for mpi_size in "${MPI_SIZES[@]}"; do
    rows=$((10000 * mpi_size))
    cols=$((10000 * mpi_size))
    nnz=$((200000 * mpi_size))
    echo "Generating matrix for MPI_SIZE=$mpi_size (${rows}x${cols}, nnz=${nnz})"
    ../bin/create_synthetic_matrices "$rows" "$cols" "$nnz"
done
cd ..
echo "Synthetic matrices generated successfully."

