#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DATA_DIR="$ROOT_DIR/datasets"
RESULTS_DIR="$ROOT_DIR/results"
PLOTS_DIR="$ROOT_DIR/plots"

# Possible overrides from outside:
BIN_SERIAL="${BIN_SERIAL:-}"
BIN_OPENMP="${BIN_OPENMP:-}"
BIN_BINNING="${BIN_BINNING:-}"

make clean
make serial-only
make openmp-only
make openmp-binning-only

python3 -m venv .venv

source ./.venv/bin/activate

pip install numpy matplotlib pandas

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

if [ ! -d "$DATA_DIR" ]; then
  echo "Data directory not found: \`$DATA_DIR\`" >&2
  exit 1
fi

mkdir -p "$RESULTS_DIR"

ts="$(date '+%Y-%m-%d_%H-%M-%S-%3N')"
TOP_OUTDIR="$RESULTS_DIR/run_${ts}"
TOP_PLOTDIR="$PLOTS_DIR/run_${ts}"
mkdir -p "$TOP_OUTDIR"
mkdir -p "$TOP_PLOTDIR"
TOP_OUTDIR="${TOP_OUTDIR%/}/"
TOP_PLOTDIR="${TOP_PLOTDIR}/"

echo "Results written to: \`$TOP_OUTDIR\`"
echo "Executables used: serial=\`$BIN_SERIAL\`, openmp=\`$BIN_OPENMP\`, binning=\`$BIN_BINNING\`"

THREADS=(1 2 4 6 8 12 16 24 32 48 64 96)
ITERATIONS=15
SERIAL_RUNS=15

while IFS= read -r -d '' file; do
  RELPATH="${file#"$ROOT_DIR/"}"

  # Serial: 15 runs per file (current step)
  for run in $(seq 1 "$SERIAL_RUNS"); do
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] Serial run $run/$SERIAL_RUNS: $file -> $TOP_OUTDIR (step: $RELPATH)"
    if ! "$BIN_SERIAL" "$RELPATH" "$run" "$TOP_OUTDIR"; then
      echo "Execution failed for \`$file\` run $run (serial)" >&2
    fi
  done

  # OpenMP: for each thread count run ITERATIONS times and pass the current index
  for nthreads in "${THREADS[@]}"; do
    for iter in $(seq 1 "$ITERATIONS"); do
      echo "[$(date '+%Y-%m-%d %H:%M:%S')] OpenMP threads=$nthreads iter=$iter/$ITERATIONS: $file -> $TOP_OUTDIR (step: $RELPATH)"
      if ! "$BIN_OPENMP" "$RELPATH" "$iter" "$TOP_OUTDIR" "$nthreads"; then
        echo "Execution failed for \`$file\` threads $nthreads iter $iter (openmp)" >&2
      fi
    done
  done

  # OpenMP Binning: for each thread count run ITERATIONS times and pass the current index
  for nthreads in "${THREADS[@]}"; do
    for iter in $(seq 1 "$ITERATIONS"); do
      echo "[$(date '+%Y-%m-%d %H:%M:%S')] Binning threads=$nthreads iter=$iter/$ITERATIONS: $file -> $TOP_OUTDIR (step: $RELPATH)"
      if ! "$BIN_BINNING" "$RELPATH" "$iter" "$TOP_OUTDIR" "$nthreads"; then
        echo "Execution failed for \`$file\` threads $nthreads iter $iter (binning)" >&2
      fi
    done
  done

done < <(find "$DATA_DIR" -type f -print0)

python3 ./scripts/analyze_results.py $TOP_OUTDIR $TOP_PLOTDIR

deactivate
rm -rf ./.venv