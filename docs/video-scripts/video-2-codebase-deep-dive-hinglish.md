# Video 2 Script: Full Codebase Deep Dive (Hinglish)

## Goal
Student-friendly deep explanation of complete server internals from socket setup to response write.

## Target Duration
25-35 minutes

## Section 1: Project Intent (0:00-2:00)
### बोले
"Is project ka main aim hai HTTP server ko scratch se samajhna: TCP stream handling, HTTP parsing, filesystem serving, security checks, concurrency aur testing."

### Screen
- Open [README.md](../README.md)
- Highlight features list.

## Section 2: Entry Point (2:00-4:00)
### बोले
"`src/main.cpp` intentionally small rakha gaya hai. Isme sirf configuration hoti hai: port aur max concurrent clients, then `run_server` call hota hai."

### Screen
```bash
sed -n '1,120p' src/main.cpp
```

## Section 3: Server Lifecycle (4:00-11:00)
### बोले
"`src/server.cpp` me core networking flow hai: socket create, SO_REUSEADDR, bind, listen, accept loop. Yehi real server bootstrap hai."

"`accept` ke baad har client ko separate thread me handle kiya jata hai. Saath me `g_active_clients` counter rakhke max limit enforce kiya gaya hai. Agar limit cross ho jaaye, server 503 return karta hai."

### Screen
```bash
sed -n '1,260p' src/server.cpp
```

### Key teaching points
- TCP stream message boundaries guarantee nahi karta.
- `recv` ko loop me chalana padta hai until `\r\n\r\n`.
- Thread-per-client simple hai, but scalable limit important hai.

## Section 4: Request Reading + Validation (11:00-16:00)
### बोले
"`handle_client` function request read karta hai chunk-by-chunk. Agar header delimiter nahi mila to read continue hota hai. Agar request size 16KB se upar gayi to 400 return."

"Phir first line parse hoti hai: METHOD PATH VERSION. Malformed hua to 400. Unsupported HTTP version hua to 505. Unsupported method hua to 405."

### Screen
- show request read loop
- show parse/validation branches

## Section 5: HTTP Utilities (16:00-20:00)
### बोले
"`src/http.cpp` me protocol-related pure functions hain. `build_response` status line, headers aur body assemble karta hai. `parse_request_line` strict parsing karta hai."

### Screen
```bash
sed -n '1,220p' src/http.cpp
```

### Concept explain
- `Content-Length` kyun mandatory hai?
- `Connection: close` behavior.
- `Allow: GET` in 405 response.

## Section 6: File + Security Layer (20:00-24:00)
### बोले
"`src/files.cpp` URL path ko sanitize karta hai. Query/fragment strip karta hai. `..` traversal aur backslash path reject karta hai. MIME type extension map use karke content-type set hota hai."

### Screen
```bash
sed -n '1,220p' src/files.cpp
```

## Section 7: Logging Layer (24:00-27:00)
### बोले
"`src/logger.cpp` thread-safe logging karta hai. Mutexes use hue hain taki concurrent requests me log corruption na ho. Console aur file logs dono sync mode me maintain hote hain."

### Screen
```bash
sed -n '1,220p' src/logger.cpp
```

## Section 8: Testing Strategy (27:00-31:00)
### बोले
"Testing do level pe hai:"

"1) Unit tests (`tests/unit_tests.cpp`) parser + sanitizer jaise deterministic logic ke liye."

"2) Integration tests (`scripts/test.sh`) real server boot karke HTTP statuses verify karte hain."

### Screen
```bash
make unit-test
make test
sed -n '1,220p' tests/unit_tests.cpp
sed -n '1,220p' scripts/test.sh
```

## Section 9: Benchmark + Engineering Tradeoffs (31:00-34:00)
### बोले
"`scripts/benchmark.sh` se throughput snapshot milta hai. Isse resume me measurable outcome add hota hai."

"Tradeoff: thread-per-connection simple hai but high scale ke liye thread pool better rahega. Keep-alive aur caching headers future work me hai."

### Screen
```bash
./scripts/benchmark.sh 1000 50
cat docs/sample-benchmark-output.txt
```

## Section 10: Closing Guidance for Students (34:00-35:00)
### बोले
"Agar aap beginner ho, pehle TCP lifecycle samjho, fir HTTP parsing, fir security checks, fir tests. Har module ka single responsibility follow karo. Is project ko clone karke step-by-step modify karo."

## Presenter Notes
- Pace slow rakho, har code block pe 20-40 sec pause.
- Har response code ka real curl demo dikhana.
- Whiteboard style flow repeat: request -> parse -> validate -> serve -> respond -> log.
