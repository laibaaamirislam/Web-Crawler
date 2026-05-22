# Parallel Web Crawler — PDC Project
## FA23-BSE-173 · Parallel & Distributed Computing

---

## 📁 Project Structure

```
parallel_web_crawler/
├── CMakeLists.txt          ← Build configuration
├── README.md               ← This file
├── src/                    ← C++ source files
│   ├── main.cpp            ← Entry point
│   ├── server.cpp          ← HTTP server (TCP socket)
│   ├── crawler.cpp         ← Core crawler logic
│   ├── http_client.cpp     ← libcurl wrapper
│   └── html_parser.cpp     ← HTML parsing utilities
├── include/                ← Header files
│   ├── server.h
│   ├── crawler.h
│   ├── http_client.h
│   ├── html_parser.h
│   └── json/json.hpp       ← nlohmann/json library
├── build/                  ← Build output directory
│   └── parallel_web_crawler ← Compiled binary
├── templates/
│   └── index.html          ← Frontend HTML
└── static/
    ├── style.css           ← Dark tech UI styles
    └── script.js           ← Frontend JavaScript
```

---

## ⚙ Setup & Run

### Prerequisites
```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install libcurl4-openssl-dev libxml2-dev cmake g++
```

### Step 1 — Build the C++ backend
```bash
cmake -S . -B build
cmake --build build
```

### Step 2 — Start the server
```bash
./build/parallel_web_crawler
```

### Step 3 — Open the browser
```
http://127.0.0.1:5000
```

---

## 🧵 PDC Concepts Demonstrated

| Concept | Implementation |
|---------|---------------|
| **Inter-site Parallelism** | One `pthread` per website |
| **Intra-site Parallelism** | 2 worker threads per site share a `std::queue` |
| **BFS Crawling** | `std::queue` stores `(url, depth)` tuples |
| **Synchronization** | `std::mutex` guards the `visited` set |
| **Task Distribution** | Dynamic load balancing via shared queue |
| **HTTP Client** | libcurl for fetching pages |
| **Socket Programming** | TCP server for REST API |
| **JSON Serialization** | nlohmann/json for responses |

---

## 🌐 Predefined Sites

- `https://en.wikipedia.org/wiki/Parallel_computing`
- `https://www.bbc.com/news`
- `https://www.python.org`

To modify, edit `PREDEFINED_SITES` in `include/crawler.h`.

---

## 📊 Settings Guide

| Setting | Range | Description |
|---------|-------|-------------|
| Max Depth | 1–3 | How deep BFS goes from root page |
| Max Pages/Site | 2–20 | Page limit per website |

Keep depth=1 and pages=6 for fast demo results.

---

## 🔑 API Endpoints

### GET /sites
Returns the list of predefined websites.
```bash
curl http://localhost:5000/sites
```

### POST /crawl
Starts a parallel crawl session.
```bash
curl -X POST http://localhost:5000/crawl \
  -H "Content-Type: application/json" \
  -d '{
    "keyword": "parallel",
    "max_depth": 1,
    "max_pages": 8
  }'
```

**Request Parameters:**
- `keyword` (string, required): Keyword to search
- `max_depth` (int, optional): BFS depth (default: 1)
- `max_pages` (int, optional): Pages per site (default: 8)

**Response Format:**
```json
{
  "matched_urls": [
    {"url": "https://...", "site": "domain.com"},
    ...
  ],
  "crawled_urls": ["https://...", ...],
  "logs": [
    {"timestamp": "HH:MM:SS", "message": "..."},
    ...
  ],
  "total_crawled": 10,
  "total_matches": 3
}
```

---

## 💻 Technology Stack

| Component | Technology | Purpose |
|-----------|-----------|---------|
| **Build System** | CMake 3.10+ | Cross-platform building |
| **HTTP Client** | libcurl | Fetching web pages |
| **Threading** | pthreads | Parallel execution |
| **JSON** | nlohmann/json | API serialization |
| **HTTP Server** | POSIX sockets | TCP server implementation |
| **HTML Parsing** | regex | Link extraction |
| **Compiler** | g++ (C++17) | Compilation |

---

## 🧪 Testing

### Manual Test
```bash
# Terminal 1: Start server
./build/parallel_web_crawler

# Terminal 2: Test /sites endpoint
curl http://localhost:5000/sites

# Terminal 3: Test /crawl endpoint with demo data
curl -X POST http://localhost:5000/crawl \
  -H "Content-Type: application/json" \
  -d '{"keyword": "python", "max_depth": 1, "max_pages": 5}'
```

### Demo Mode
When network is unavailable, the crawler runs in **demo mode** using synthetic data. This allows full PDC logic testing in offline environments.

---

## 🎯 Example Keywords to Try

- `parallel` (matches Wikipedia page for sure)
- `python` (matches python.org)
- `news` (matches BBC)
- `thread`
- `computing`

---

## 📝 Notes

### Performance
- Network fetching is the bottleneck; tune `max_depth` and `max_pages` accordingly
- Intra-site parallelism helps when pages load slowly
- Demo mode runs instantly for testing logic without network I/O

### Error Handling
- Network errors are logged but don't crash the crawler
- Invalid JSON in POST requests returns 400
- Malformed URLs are skipped during link extraction
- Missing files return 404 HTTP responses

### Limitations
- No persistent storage (results are in-memory only)
- No rate limiting (may get blocked by servers)
- No cookie/session management
- Single-threaded request acceptance (but request handling is async per thread)

---

## 🔧 Build Troubleshooting

If dependencies are missing:
```bash
# Ubuntu/Debian
sudo apt-get install libcurl4-openssl-dev libxml2-dev cmake build-essential

# Fedora/RHEL
sudo dnf install libcurl-devel libxml2-devel cmake gcc-c++

# macOS
brew install curl libxml2 cmake
```

---

## 📄 License & Credits

- **Original Python Version**: Flask, requests, BeautifulSoup4
- **C++ Rewrite**: pthreads, libcurl, nlohmann/json
- **Course**: Parallel & Distributed Computing (PDC)
- **Student ID**: FA23-BSE-173

