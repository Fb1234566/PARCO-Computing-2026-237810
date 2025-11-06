#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DATA_DIR="$ROOT_DIR/datasets"
DEFAULT_BIN="/home/universita/Documenti/ParallelComputing/deliverable1_2025_2026/bin/serial_spmv"
BIN="${BIN:-$DEFAULT_BIN}"
RESULTS_DIR="$ROOT_DIR/results"

make clean
make serial-only

# risolvi BIN se non eseguibile
if [ ! -x "$BIN" ]; then
  FOUND="$(find "$ROOT_DIR" -type f \( -name 'serial_spmv' -o -name 'serial_spvm' \) -executable -print -quit || true)"
  if [ -n "$FOUND" ]; then
    BIN="$FOUND"
  else
    FOUND_FILE="$(find "$ROOT_DIR" -type f \( -name 'serial_spmv' -o -name 'serial_spvm' \) -print -quit || true)"
    if [ -n "$FOUND_FILE" ]; then
      BIN="$FOUND_FILE"
      chmod +x "$BIN"
    else
      echo "Eseguibile \`serial_spmv\` o \`serial_spvm\` non trovato." >&2
      exit 1
    fi
  fi
fi

if [ ! -d "$DATA_DIR" ]; then
  echo "Cartella dati non trovata: \`$DATA_DIR\`" >&2
  exit 1
fi

mkdir -p "$RESULTS_DIR"

# cartella unica per l'intero test (creata all'avvio) e con slash finale
ts="$(date '+%Y-%m-%d_%H-%M-%S-%3N')"
TOP_OUTDIR="$RESULTS_DIR/run_${ts}"
mkdir -p "$TOP_OUTDIR"
TOP_OUTDIR="${TOP_OUTDIR%/}/"

echo "Risultati scritti in: \`$TOP_OUTDIR\`"
echo "Eseguibile usato: \`$BIN\`"

# Per ogni file in datasets (ricorsivamente), esegui il binario 15 volte
while IFS= read -r -d '' file; do
  for run in $(seq 1 15); do
    RELPATH="${file#"$ROOT_DIR/"}"
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] Esecuzione $run/15: $file -> $TOP_OUTDIR (passo: $RELPATH)"
    if ! "$BIN" "$RELPATH" "$run" "$TOP_OUTDIR"; then
      echo "Esecuzione fallita per \`$file\` run $run" >&2
    fi
  done
done < <(find "$DATA_DIR" -type f -print0)
