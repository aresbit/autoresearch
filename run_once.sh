#!/bin/sh
set -eu

RESULTS_FILE="${RESULTS_FILE:-results.tsv}"
LOG_FILE="${LOG_FILE:-run.log}"
DESC="${1:-experiment}"
TRAIN_CMD="${TRAIN_CMD:-./bin/train --time-budget 300}"

if [ ! -f "$RESULTS_FILE" ]; then
  ./init_results.sh "$RESULTS_FILE"
fi

$TRAIN_CMD > "$LOG_FILE" 2>&1 || true

commit="$(git rev-parse --short HEAD)"

extract_num() {
  key="$1"
  awk -F': *' -v k="$key" '$1==k {print $2; found=1} END{if(!found) print ""}' "$LOG_FILE" | tail -n 1
}

snri="$(extract_num snri)"
pesq="$(extract_num pesq)"
stoi="$(extract_num stoi)"
latency="$(extract_num latency_ms)"
cpu="$(extract_num cpu_percent)"

status="keep"

if [ -z "$snri" ] || [ -z "$pesq" ] || [ -z "$stoi" ] || [ -z "$latency" ] || [ -z "$cpu" ]; then
  echo "warning: expected keys not found (snri/pesq/stoi/latency_ms/cpu_percent)." >&2
  echo "warning: ensure TRAIN_CMD points to the vehicle-noise training entry." >&2
  snri="0.000000"
  pesq="0.000000"
  stoi="0.000000"
  latency="0.0"
  cpu="0.0"
  status="crash"
else
  over_latency="$(awk -v v="$latency" 'BEGIN{print (v>10.0)?1:0}')"
  over_cpu="$(awk -v v="$cpu" 'BEGIN{print (v>60.0)?1:0}')"
  if [ "$over_latency" -eq 1 ] || [ "$over_cpu" -eq 1 ]; then
    status="discard"
  fi
fi

printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
  "$commit" "$snri" "$pesq" "$stoi" "$latency" "$cpu" "$status" "$DESC" >> "$RESULTS_FILE"

printf 'status=%s\n' "$status"
printf 'metrics: snri=%s pesq=%s stoi=%s latency_ms=%s cpu_percent=%s\n' "$snri" "$pesq" "$stoi" "$latency" "$cpu"
printf 'logged: %s\n' "$RESULTS_FILE"
