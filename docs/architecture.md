# Architecture

```text
+------------------------+
|       main.cpp         |
|  (entrypoint/config)   |
+-----------+------------+
            |
            v
+------------------------+
|       server.cpp       |
| socket/bind/listen     |
| accept + threads       |
| max-client guard       |
+----+-----------+-------+
     |           |
     |           +-------------------------------+
     |                                           |
     v                                           v
+------------------------+            +------------------------+
|        http.cpp        |            |       logger.cpp       |
| parse request line     |            | logs/server.log        |
| build HTTP response    |            | synchronized console   |
| status reason mapping  |            | and file logging       |
+------------------------+            +------------------------+
     |
     v
+------------------------+
|       files.cpp        |
| path sanitize          |
| query stripping        |
| MIME type detection    |
| read static file       |
+------------------------+
```

Request flow: client socket -> `server.cpp::handle_client` -> parse/validate via `http.cpp` -> sanitize/read via `files.cpp` -> build response via `http.cpp` -> log via `logger.cpp`.
