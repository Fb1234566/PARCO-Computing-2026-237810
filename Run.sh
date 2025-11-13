#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DATA_DIR="$ROOT_DIR/datasets"

# Possible overrides from outside:
BIN_SERIAL="${BIN_SERIAL:-}"
BIN_OPENMP="${BIN_OPENMP:-}"
BIN_BINNING="${BIN_BINNING:-}"
BIN_DYNAMIC="${BIN_DYNAMIC:-}"
BIN_GUIDED="${BIN_GUIDED:-}"

# Function to resolve a binary: search for executable or file and make it executable
resolve_bin() {
  varname="$1"; shift
  patterns=("$@")
  eval current="\${$varname:-}"
  if [ -n "$current" ] && [ -x "$current" ]; then
    return 0
  fi

  # Search executable files with provided names (or '_' -> '-')
  for p in "${patterns[@]}"; do
    FOUND="$(find "$ROOT_DIR" -type f -name "$p" -executable -print -quit || true)"
    if [ -n "$FOUND" ]; then
      eval "$varname=\"\$FOUND\""
      return 0
    fi
    alt="${p//_/-}"
    if [ "$alt" != "$p" ]; then
      FOUND="$(find "$ROOT_DIR" -type f -name "$alt" -executable -print -quit || true)"
      if [ -n "$FOUND" ]; then
        eval "$varname=\"\$FOUND\""
        return 0
      fi
    fi
  done

  # If no executable found, search for file with that name and make it executable
  for p in "${patterns[@]}"; do
    FOUND_FILE="$(find "$ROOT_DIR" -type f -name "$p" -print -quit || true)"
    if [ -n "$FOUND_FILE" ]; then
      chmod +x "$FOUND_FILE"
      eval "$varname=\"\$FOUND_FILE\""
      return 0
    fi
    alt="${p//_/-}"
    if [ "$alt" != "$p" ]; then
      FOUND_FILE="$(find "$ROOT_DIR" -type f -name "$alt" -print -quit || true)"
      if [ -n "$FOUND_FILE" ]; then
        chmod +x "$FOUND_FILE"
        eval "$varname=\"\$FOUND_FILE\""
        return 0
      fi
    fi
  done

  echo "Executable for \`$varname\` not found." >&2
  exit 1
}

# Use name patterns (only name, no path)
resolve_bin BIN_SERIAL serial_spmv
resolve_bin BIN_OPENMP openmp_spmv
resolve_bin BIN_BINNING openmp_spmv_binning
resolve_bin BIN_DYNAMIC openmp_spmv_dynamic
resolve_bin BIN_GUIDED openmp_spmv_guided

if [ ! -d "$DATA_DIR" ]; then
  echo "Data directory not found: \`$DATA_DIR\`" >&2
  exit 1
fi

echo "Executables used: serial=\`$BIN_SERIAL\`, openmp=\`$BIN_OPENMP\`, binning=\`$BIN_BINNING\`, dynamic=\`$BIN_DYNAMIC\`, guided=\`$BIN_GUIDED\`"

THREADS=(1 2 4 6 8 12 16 24 32)
ITERATIONS=10
SERIAL_RUNS=10

# Expect matrix path, results directory and plots directory as arguments
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

# Verify/create output directories
mkdir -p "$OUTDIR"
mkdir -p "$PLOTDIR"

# Compute RELPATH relative to ROOT_DIR when possible, otherwise use absolute path
if [[ "$MATRIX_PATH" == "$ROOT_DIR/"* ]]; then
  RELPATH="${MATRIX_PATH#$ROOT_DIR/}"
else
  RELPATH="$MATRIX_PATH"
fi

echo "Results written to: \`$OUTDIR\`"
echo "Plots written to: \`$PLOTDIR\`"
echo "Running tests for matrix: \`$RELPATH\`"

# Serial: 10 runs for this single file
for run in $(seq 1 "$SERIAL_RUNS"); do
  echo "[$(date '+%Y-%m-%d %H:%M:%S')] Serial run $run/$SERIAL_RUNS: $RELPATH -> $OUTDIR"
  if ! "$BIN_SERIAL" "$RELPATH" "$run" "$OUTDIR"; then
    echo "Execution failed for \`$RELPATH\` run $run (serial)" >&2
  fi
done

# OpenMP: for each thread count run ITERATIONS times and pass the current index
for nthreads in "${THREADS[@]}"; do
  for iter in $(seq 1 "$ITERATIONS"); do
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] OpenMP threads=$nthreads iter=$iter/$ITERATIONS: $RELPATH -> $OUTDIR"
    if ! "$BIN_OPENMP" "$RELPATH" "$iter" "$OUTDIR" "$nthreads"; then
      echo "Execution failed for \`$RELPATH\` threads $nthreads iter $iter (openmp)" >&2
    fi
  done
done

# OpenMP Binning: for each thread count run ITERATIONS times and pass the current index
for nthreads in "${THREADS[@]}"; do
  for iter in $(seq 1 "$ITERATIONS"); do
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] Binning threads=$nthreads iter=$iter/$ITERATIONS: $RELPATH -> $OUTDIR"
    if ! "$BIN_BINNING" "$RELPATH" "$iter" "$OUTDIR" "$nthreads"; then
      echo "Execution failed for \`$RELPATH\` threads $nthreads iter $iter (binning)" >&2
    fi
  done
done

# OpenMP Dynamic: for each thread count run ITERATIONS times and pass the current index
for nthreads in "${THREADS[@]}"; do
  for iter in $(seq 1 "$ITERATIONS"); do
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] Dynamic threads=$nthreads iter=$iter/$ITERATIONS: $RELPATH -> $OUTDIR"
    if ! "$BIN_DYNAMIC" "$RELPATH" "$iter" "$OUTDIR" "$nthreads"; then
      echo "Execution failed for \`$RELPATH\` threads $nthreads iter $iter (dynamic)" >&2
    fi
  done
done

# OpenMP Guided: for each thread count run ITERATIONS times and pass the current index
for nthreads in "${THREADS[@]}"; do
  for iter in $(seq 1 "$ITERATIONS"); do
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] Guided threads=$nthreads iter=$iter/$ITERATIONS: $RELPATH -> $OUTDIR"
    if ! "$BIN_GUIDED" "$RELPATH" "$iter" "$OUTDIR" "$nthreads"; then
      echo "Execution failed for \`$RELPATH\` threads $nthreads iter $iter (guided)" >&2
    fi
  done
done

source ./.venv/bin/activate
# Analyze results for this matrix only
python3 ./scripts/analyze_results.py "$OUTDIR" "$PLOTDIR"
deactivate
