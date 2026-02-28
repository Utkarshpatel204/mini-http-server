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
│   ├── sample-log-output.txt
│   └── sample-test-output.txt
├── scripts/
│   └── test.sh
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

## Logging Proof

Saved sample logs: [docs/sample-log-output.txt](docs/sample-log-output.txt)

Log format:

```text
[YYYY-MM-DD HH:MM:SS] METHOD PATH VERSION -> STATUS
```

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
