#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

step() {
    echo
    echo "============================================================"
    echo "$1"
    echo "============================================================"
}

step "1) Project root + structure"
pwd
ls -la

step "2) Architecture snapshot"
if command -v tree >/dev/null 2>&1; then
    tree -L 2
else
    find . -maxdepth 2 -type d | sort
fi
sed -n '1,160p' docs/architecture.md

step "3) Build"
make

step "4) Run server (background)"
./server >/tmp/mini_http_server_demo.log 2>&1 &
SERVER_PID=$!
sleep 1

echo "Server PID: $SERVER_PID"

step "5) Demo requests"
curl -i http://127.0.0.1:8080/
echo
curl -i http://127.0.0.1:8080/missing-file
echo
curl -i -X POST http://127.0.0.1:8080/
echo
curl --path-as-is -i http://127.0.0.1:8080/../etc/passwd

step "6) Unit + integration tests"
make unit-test
./scripts/test.sh

step "7) Benchmark"
./scripts/benchmark.sh 1000 50

step "8) Logs"
tail -n 20 logs/server.log

step "9) Cleanup"
kill "$SERVER_PID" >/dev/null 2>&1 || true
wait "$SERVER_PID" >/dev/null 2>&1 || true

echo "Demo walkthrough finished."
