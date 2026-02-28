#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

REQUESTS="${1:-1000}"
CONCURRENCY="${2:-50}"
OUTPUT_FILE="${3:-docs/sample-benchmark-output.txt}"

cleanup() {
    if [[ -n "${SERVER_PID:-}" ]]; then
        kill "$SERVER_PID" >/dev/null 2>&1 || true
        wait "$SERVER_PID" >/dev/null 2>&1 || true
    fi
}
trap cleanup EXIT

mkdir -p "$(dirname "$OUTPUT_FILE")"

make >/dev/null
./server >/tmp/mini_http_server_benchmark.log 2>&1 &
SERVER_PID=$!
sleep 1

start_ns="$(date +%s%N)"
seq 1 "$REQUESTS" | xargs -I{} -P "$CONCURRENCY" curl -sS -o /dev/null -w "%{http_code}\n" http://127.0.0.1:8080/ > /tmp/mini_http_server_benchmark_statuses.txt
end_ns="$(date +%s%N)"

duration_sec="$(awk -v s="$start_ns" -v e="$end_ns" 'BEGIN { printf "%.6f", (e - s) / 1000000000 }')"
success_count="$(grep -c '^200$' /tmp/mini_http_server_benchmark_statuses.txt || true)"
failure_count="$((REQUESTS - success_count))"
throughput="$(awk -v r="$REQUESTS" -v d="$duration_sec" 'BEGIN { if (d > 0) printf "%.2f", r / d; else print "0.00" }')"

{
    echo "Benchmark Date (UTC): $(date -u '+%Y-%m-%d %H:%M:%S')"
    echo "Requests: $REQUESTS"
    echo "Concurrency: $CONCURRENCY"
    echo "Duration (seconds): $duration_sec"
    echo "Throughput (requests/sec): $throughput"
    echo "HTTP 200 responses: $success_count"
    echo "Non-200 responses: $failure_count"
} > "$OUTPUT_FILE"

cat "$OUTPUT_FILE"
