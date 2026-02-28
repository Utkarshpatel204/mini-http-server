#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

cleanup() {
    if [[ -n "${SERVER_PID:-}" ]]; then
        kill "$SERVER_PID" >/dev/null 2>&1 || true
        wait "$SERVER_PID" >/dev/null 2>&1 || true
    fi
}
trap cleanup EXIT

echo "[1/4] Building server..."
make >/dev/null

echo "[2/4] Starting server..."
./server >/tmp/mini_http_server_test.log 2>&1 &
SERVER_PID=$!
sleep 1

check_status() {
    local name="$1"
    local expected="$2"
    local actual="$3"

    if [[ "$actual" == "$expected" ]]; then
        echo "PASS: $name ($actual)"
    else
        echo "FAIL: $name (expected $expected, got $actual)"
        exit 1
    fi
}

echo "[3/4] Running HTTP checks..."
status_200="$(curl -sS -o /dev/null -w "%{http_code}" http://127.0.0.1:8080/)"
check_status "GET /" "200" "$status_200"

status_404="$(curl -sS -o /dev/null -w "%{http_code}" http://127.0.0.1:8080/missing-file)"
check_status "GET /missing-file" "404" "$status_404"

status_405="$(curl -sS -o /dev/null -w "%{http_code}" -X POST http://127.0.0.1:8080/)"
check_status "POST /" "405" "$status_405"

status_400="$(curl -sS --path-as-is -o /dev/null -w "%{http_code}" http://127.0.0.1:8080/../etc/passwd)"
check_status "Traversal path" "400" "$status_400"

status_505="$(
    bash -lc '
        exec 3<>/dev/tcp/127.0.0.1/8080
        printf "GET / HTTP/2.0\r\nHost: 127.0.0.1\r\n\r\n" >&3
        head -n 1 <&3
        exec 3>&-
        exec 3<&-
    ' | awk "{print \$2}"
)"
check_status "GET / HTTP/2.0" "505" "$status_505"

echo "[4/4] All tests passed."
