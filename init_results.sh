#!/bin/sh
set -eu

OUT_FILE="${1:-results.tsv}"

if [ ! -f "$OUT_FILE" ]; then
  printf 'commit\tsnri\tpesq\tstoi\tlatency_ms\tcpu_percent\tstatus\tdescription\n' > "$OUT_FILE"
  echo "initialized: $OUT_FILE"
else
  echo "exists: $OUT_FILE"
fi
