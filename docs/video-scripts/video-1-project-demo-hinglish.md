# Video 1 Script: Project Demo (Hinglish)

## Goal
Show end-to-end demo: build, run, test, benchmark, logs, and project structure.

## Target Duration
8-10 minutes

## Opening (0:00-0:30)
### बोले (Hinglish)
"Hi everyone, aaj main apna Mini HTTP Server project demo karunga jo C++ aur POSIX sockets pe build hai. Isme HTTP parsing, static file serving, logging, testing, aur concurrency support implement kiya hai."

### Screen pe
- Open terminal in project root.
- Run:
```bash
pwd
ls -la
```

## Quick Architecture View (0:30-1:20)
### बोले
"Codebase modular hai. `src/server.cpp` networking + request flow handle karta hai, `http.cpp` parsing/response banata hai, `files.cpp` path safety + MIME types deta hai, aur `logger.cpp` logs maintain karta hai."

### Screen pe
```bash
tree -L 2
sed -n '1,120p' docs/architecture.md
```

## Build + Run (1:20-2:20)
### बोले
"Pehle main build karta hoon. Makefile all modules compile karta hai aur `server` binary generate hoti hai."

### Screen pe
```bash
make
./server
```

### बोले
"Server port 8080 pe run ho raha hai. Ab main dusre terminal se requests fire karta hoon."

## Manual API Checks (2:20-4:10)
### Screen pe (new terminal)
```bash
curl -i http://127.0.0.1:8080/
curl -i http://127.0.0.1:8080/missing-file
curl -i -X POST http://127.0.0.1:8080/
curl --path-as-is -i http://127.0.0.1:8080/../etc/passwd
```

### बोले
"Yahan aap dekh sakte ho valid page pe 200, missing file pe 404, POST pe 405, aur traversal attack pe 400 return ho raha hai."

## Automated Tests (4:10-5:10)
### बोले
"Project me focused unit tests aur integration tests dono hain."

### Screen pe
```bash
make unit-test
make test
```

### बोले
"Unit tests parser aur sanitizer validate karte hain. Integration tests full HTTP behavior check karte hain."

## Benchmark Demo (5:10-6:20)
### बोले
"Ab basic throughput benchmark run karte hain."

### Screen pe
```bash
./scripts/benchmark.sh 1000 50
cat docs/sample-benchmark-output.txt
```

### बोले
"Is run me 1000 requests, concurrency 50, and throughput ~1094 req/s mila."

## Logs Demo (6:20-7:10)
### बोले
"Har request structured format me logs/server.log me store hoti hai."

### Screen pe
```bash
tail -n 20 logs/server.log
```

## GitHub/Docs Proof (7:10-8:20)
### बोले
"README me architecture, test proof, log proof, benchmark proof aur future roadmap documented hai."

### Screen pe
```bash
sed -n '1,260p' README.md
```

## Closing (8:20-9:00)
### बोले
"Ye project low-level networking, protocol handling, security checks, and practical software engineering practices show karta hai. Thank you."

## Recording Tips
- Font size: 16-18 terminal
- Zoom: 125%-150%
- Keep command history clean
- Pause 2-3 sec after each key output
