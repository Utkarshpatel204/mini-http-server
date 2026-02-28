# Mini HTTP Server (C++)

A simple HTTP/1.1 static file server built from scratch in C++ using POSIX sockets on Linux/WSL.

## Features

- TCP server lifecycle: `socket`, `bind`, `listen`, `accept`, `recv`, `send`, `close`
- Stream-safe request reading (reads until `\r\n\r\n`)
- Strict request-line parsing and validation
- Static file serving from `www/`
- MIME type handling for common file extensions
- Status handling: `200`, `400`, `404`, `405`, `503`, `505`
- Path safety checks to block traversal attempts
- Request logging to `logs/server.log`
- Thread-per-connection model with max concurrent client guard

## Project Structure

```text
mini-http-server/
├── Makefile
├── README.md
├── docs/
│   ├── architecture.md
│   ├── sample-benchmark-output.txt
│   ├── sample-log-output.txt
│   └── sample-test-output.txt
├── scripts/
│   ├── benchmark.sh
│   └── test.sh
├── tests/
│   └── unit_tests.cpp
├── src/
│   ├── files.cpp / files.h
│   ├── http.cpp / http.h
│   ├── logger.cpp / logger.h
│   ├── main.cpp
│   └── server.cpp / server.h
├── www/
│   ├── index.html
│   └── style.css
└── logs/
```

## Architecture

Detailed module diagram: [docs/architecture.md](docs/architecture.md)

## Build and Run

```bash
make
./server
```

Or with Make targets:

```bash
make run
```

Server runs on: `http://127.0.0.1:8080`

## Test

```bash
make test
```

This validates:

- `GET /` -> `200`
- `GET /missing-file` -> `404`
- `POST /` -> `405`
- Traversal path attempt -> `400`
- `HTTP/2.0` request line -> `505`

Saved test proof: [docs/sample-test-output.txt](docs/sample-test-output.txt)

### Unit Tests (Focused)

The project also has focused unit tests for request-line parsing and path sanitization:

```bash
make unit-test
```

Test file: [tests/unit_tests.cpp](tests/unit_tests.cpp)

## Logging Proof

Saved sample logs: [docs/sample-log-output.txt](docs/sample-log-output.txt)

Log format:

```text
[YYYY-MM-DD HH:MM:SS] METHOD PATH VERSION -> STATUS
```

## Benchmark (Local)

Run:

```bash
./scripts/benchmark.sh 1000 50
```

- Arguments: `requests concurrency`
- Output file: [docs/sample-benchmark-output.txt](docs/sample-benchmark-output.txt)

This gives a simple throughput snapshot for resume/project proof.

Latest sample run (UTC `2026-02-28 13:51:57`):

- Requests: `1000`
- Concurrency: `50`
- Duration: `0.913932s`
- Throughput: `1094.17 req/s`
- Success rate: `100%` (`1000/1000` HTTP 200)

## Manual cURL Examples

```bash
curl -i http://127.0.0.1:8080/
curl -i http://127.0.0.1:8080/missing-file
curl -i -X POST http://127.0.0.1:8080/
curl --path-as-is -i http://127.0.0.1:8080/../etc/passwd
```

## Notes

- Current implementation closes the connection after each response (`Connection: close`).
- Max concurrent clients are configured in `src/main.cpp`.
- This project is intended for learning systems and networking fundamentals.

## Known Limitations

- Uses thread-per-connection (not a bounded thread pool).
- Supports only `GET` for static files.
- No HTTP keep-alive/persistent connection handling.
- No advanced caching headers (`ETag`, `Last-Modified`) yet.

## Future Work

- Replace detached threads with a fixed-size thread pool.
- Add support for keep-alive and multiple requests per connection.
- Add caching headers and conditional requests.
- Expand unit test coverage beyond parser/path logic.
