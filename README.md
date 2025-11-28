# 🚀 AlgoVault — High-Performance Key-Value Store (C++17)

![build](https://img.shields.io/badge/build-passing-brightgreen)
![cpp](https://img.shields.io/badge/C++-17-blue)
![cmake](https://img.shields.io/badge/CMake-3.16+-blue)
![lru-cache](https://img.shields.io/badge/LRU%20Cache-enabled-blueviolet)
![ttl](https://img.shields.io/badge/TTL-supported-yellow)
![WAL](https://img.shields.io/badge/WAL-supported-success)
![platform](https://img.shields.io/badge/platform-macOS%20%7C%20Linux-lightgrey)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](https://github.com/SakshamPilane/AlgoVault/blob/main/LICENSE)
![contributions](https://img.shields.io/badge/contributions-welcome-orange)

AlgoVault is a lightweight, high-performance **in-memory key–value store** written in modern C++17.  
It supports:

- ⚡ **LRU Cache** (fast reads, automatic eviction)
- 🕒 **TTL / Expiring Keys**
- 🧱 **Write-Ahead Logging (WAL)** for crash recovery
- 🌐 **HTTP REST API Server**
- 🧹 **Background TTL cleanup thread**
- 🔄 **WAL compaction**
- 🧵 Thread-safe with shared mutexes
- 🛠️ Fully cross-platform (macOS, Linux, Windows)

---

## 📂 Project Structure
```
📦 AlgoVault
 ┣ 📂 src
 ┃ ┣ 📄 kvstore.cpp
 ┃ ┣ 📄 lru_cache.cpp
 ┃ ┣ 📄 persistence.cpp
 ┃ ┗ 📄 server.cpp
 ┣ 📂 include
 ┃ ┣ 📄 kvstore.h
 ┃ ┣ 📄 lru_cache.h
 ┃ ┣ 📄 persistence.h
 ┃ ┗ 📄 server.h
 ┣ 📂 external
 ┃ ┣ 📄 json.hpp
 ┃ ┗ 📄 httplib.h
 ┣ 📂 data
 ┣ 📄 main.cpp
 ┗ 📄 CMakeLists.txt
```


---

## 🔧 Build & Run

### **1. Build**
```bash
mkdir build
cd build
cmake ..
make
./algovault
```

You’ll see: 
```bash
Recovered X keys from WAL.
[TTL] Background cleaner running every 1 second.
[Server] Running at http://localhost:8080
```

---

## 🌐 REST API Endpoints

### 1️⃣ PUT a key
```bash
curl -X POST http://localhost:8080/put \
     -d '{"key":"name","value":"Saksham"}'
```

### 2️⃣ PUT with TTL
```bash
curl -X POST http://localhost:8080/put \
     -d '{"key":"temp","value":"123","ttl":5}'
```

### 3️⃣ GET
```bash
curl "http://localhost:8080/get?key=name"
```

### 4️⃣ DELETE
```bash
curl -X DELETE "http://localhost:8080/delete?key=name"
```

### 5️⃣ GET key TTL
```bash
curl "http://localhost:8080/ttl?key=temp"
```

### 6️⃣ WAL Compaction
```bash
curl -X POST http://localhost:8080/compact
```

---

## 🧠 LRU Cache Stats

### Get stats
```bash
curl http://localhost:8080/cache/stats
```

Response example:
```bash
{
  "hits": 10,
  "misses": 3,
  "evictions": 1,
  "items": 3
}
```

### Reset stats
```bash
curl -X POST http://localhost:8080/cache/stats/reset
```

---

## 🕒 TTL (Time-To-Live)

- AlgoVault supports per-key TTL using millisecond precision.
- Expired keys auto-delete
- Cleanup runs every 1 second
- WAL persists delete operations

### Example:
```bash
curl -X POST http://localhost:8080/put \
     -d '{"key":"session","value":"xyz","ttl":3}'
sleep 4
curl "http://localhost:8080/get?key=session"

```

Result:
```bash
{"found":false,"key":"session"}
```

---

## 🔁 Crash Recovery (WAL Replay)

When AlgoVault starts:
- Reads data/wal.log
- Replays SET and DEL operations
- Restores all keys exactly as before crash

Safe durability without slowing down writes.

## 📈 Performance Notes
- Most GET operations served directly from LRU cache
- Store uses std::shared_mutex for high concurrency
- WAL append is sequential — minimal overhead
- TTL cleanup runs independently

Perfect for:
- Backend caching
- Session storage
- Small embedded KV use cases
- Learning advanced C++ systems programming

## 🚧 Future Enhancements
- AOF/WAL rotation
- Snapshot dump to disk (RDB-style)
- Pub/Sub channels
- Cluster mode + sharding
- Benchmark suite
- WASM/Browser version
- Authentication & ACLs
- Docker container release

## 🧑‍💻 Author
Saksham Pilane
Backend Developer & Systems Engineer
C++ | Backend | Distributed Systems

## 📜 License
[![License](https://img.shields.io/badge/License-MIT-green.svg)](https://github.com/SakshamPilane/AlgoVault/blob/main/LICENSE)
This project is licensed under the **MIT License**.  
Click the badge above to read the full license.
