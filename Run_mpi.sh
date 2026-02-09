#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DATA_DIR="$ROOT_DIR/datasets"

# Possible overrides from outside (binary paths)
BIN_SERIAL="${BIN_SERIAL:-}"
BIN_MPI="${BIN_MPI:-}"
BIN_OPENMP="${BIN_OPENMP:-}"
MPI_RUN_CMD="${MPI_RUN_CMD:-mpirun}"

# Function to resolve a binary: search for executable or file and make it executable
resolve_bin() {
  varname="$1"; shift
  patterns=("$@")
  eval current="\${$varname:-}"
  if [ -n "$current" ] && [ -x "$current" ]; then
    return 0
  fi

  for p in "${patterns[@]}"; do
    FOUND="$(find "$ROOT_DIR" -type f -name "$p" -executable -print -quit || true)"
    if [ -n "$FOUND" ]; then
      eval "$varname=\"$FOUND\""
      return 0
    fi
    alt="${p//_/-}"
    if [ "$alt" != "$p" ]; then
      FOUND="$(find "$ROOT_DIR" -type f -name "$alt" -executable -print -quit || true)"
      if [ -n "$FOUND" ]; then
        eval "$varname=\"$FOUND\""
        return 0
      fi
    fi
  done

  for p in "${patterns[@]}"; do
    FOUND_FILE="$(find "$ROOT_DIR" -type f -name "$p" -print -quit || true)"
    if [ -n "$FOUND_FILE" ]; then
      chmod +x "$FOUND_FILE"
      eval "$varname=\"$FOUND_FILE\""
      return 0
    fi
    alt="${p//_/-}"
    if [ "$alt" != "$p" ]; then
      FOUND_FILE="$(find "$ROOT_DIR" -type f -name "$alt" -print -quit || true)"
      if [ -n "$FOUND_FILE" ]; then
        chmod +x "$FOUND_FILE"
        eval "$varname=\"$FOUND_FILE\""
        return 0
      fi
    fi
  done

  echo "Executable for \`$varname\` not found." >&2
  exit 1
}

# Resolve binaries
resolve_bin BIN_SERIAL serial_spmv
resolve_bin BIN_MPI mpi
resolve_bin BIN_OPENMP openmp_spmv

if [ $# -lt 3 ]; then
  echo "Usage: $0 <matrix_path> <results_dir> <plots_dir>" >&2
  exit 1
fi

MATRIX_PATH_RAW="$1"
OUTDIR="$2"
PLOTDIR="$3"

MATRIX_PATH="$(realpath "$MATRIX_PATH_RAW" 2>/dev/null || true)"

if [ -z "$MATRIX_PATH" ] || [ ! -f "$MATRIX_PATH" ]; then
  echo "Matrix file not found: \`$MATRIX_PATH_RAW\`" >&2
  exit 1
fi

mkdir -p "$OUTDIR"
mkdir -p "$PLOTDIR"

if [[ "$MATRIX_PATH" == "$ROOT_DIR/"* ]]; then
  RELPATH="${MATRIX_PATH#$ROOT_DIR/}"
else
  RELPATH="$MATRIX_PATH"
fi

echo "Results written to: \`$OUTDIR\`"
echo "Plots written to: \`$PLOTDIR\`"
echo "Running MPI tests for matrix: \`$RELPATH\`"

# Determine allocated MPI slots if running under PBS
NPROC_ALLOC=0
if [ -n "${PBS_NODEFILE:-}" ] && [ -f "$PBS_NODEFILE" ]; then
  NPROC_ALLOC=$(wc -l < "$PBS_NODEFILE" | tr -d ' ')
  echo "Detected PBS allocation: $NPROC_ALLOC MPI ranks (from $PBS_NODEFILE)"
fi

# Serial baseline runs (same as Run.sh serial)
SERIAL_RUNS=10
for run in $(seq 1 "$SERIAL_RUNS"); do
  echo "[$(date '+%Y-%m-%d %H:%M:%S')] Serial run $run/$SERIAL_RUNS: $RELPATH -> $OUTDIR"
  if ! "$BIN_SERIAL" "$RELPATH" "$run" "$OUTDIR"; then
    echo "Execution failed for \`$RELPATH\` run $run (serial)" >&2
  fi
done

# MPI runs: default sizes (powers of two up to 256)
MPi_SIZES=(1 2 4 8 16 32 64 128 256)
ITERATIONS=10

# If running under PBS and NPROC_ALLOC > 0, restrict sizes to <= NPROC_ALLOC
if [ "$NPROC_ALLOC" -gt 0 ]; then
  FILTERED=()
  for s in "${MPi_SIZES[@]}"; do
    if [ "$s" -le "$NPROC_ALLOC" ]; then
      FILTERED+=("$s")
    fi
  done
  if [ ${#FILTERED[@]} -eq 0 ]; then
    # No pre-defined size fits the allocation; use 1 and NPROC_ALLOC
    FILTERED=(1 "$NPROC_ALLOC")
  fi
  MPi_SIZES=("${FILTERED[@]}")
  echo "Restricted MPI sizes to: ${MPi_SIZES[*]} based on allocation"
fi

for nprocs in "${MPi_SIZES[@]}"; do
  for iter in $(seq 1 "$ITERATIONS"); do
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] MPI procs=$nprocs iter=$iter/$ITERATIONS: $RELPATH -> $OUTDIR"
    # Use configured MPI runner (mpirun or srun or other)
    if ! $MPI_RUN_CMD -np "$nprocs" "$BIN_MPI" --type file --file "$MATRIX_PATH" --iteration "$iter" --export "$OUTDIR"; then
      echo "Execution failed for \`$RELPATH\` procs $nprocs iter $iter (mpi)" >&2
    fi
  done
done

# OpenMP: run 10 iterations for various thread counts
THREADS=(1 2 4 8 16 32 64 128 256)
BINNING_ITERATIONS=10

for nthreads in "${THREADS[@]}"; do
  for iter in $(seq 1 "$BINNING_ITERATIONS"); do
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] Binning threads=$nthreads iter=$iter/$BINNING_ITERATIONS: $RELPATH -> $OUTDIR"
    if ! "$BIN_OPENMP" "$RELPATH" "$iter" "$OUTDIR" "$nthreads"; then
      echo "Execution failed for \`$RELPATH\` threads $nthreads iter $iter (binning)" >&2
    fi
  done
done

# Run analysis for this matrix only
if [ -f .venv/bin/activate ]; then
  source ./.venv/bin/activate
fi

if [ -f ./scripts/analyze_results.py ]; then
  python3 ./scripts/analyze_results.py "$OUTDIR" "$PLOTDIR" || true
fi
if [ -f ./scripts/make_paper_figures.py ]; then
  python3 ./scripts/make_paper_figures.py "$OUTDIR" "$PLOTDIR" || true
fi

if [ -f .venv/bin/activate ]; then
  deactivate
fi
