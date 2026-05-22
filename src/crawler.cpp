#include "crawler.h"
#include "http_client.h"
#include "html_parser.h"
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <curl/curl.h>

// Initialize static data
const std::vector<std::string> ParallelCrawler::PREDEFINED_SITES = {
    "https://en.wikipedia.org/wiki/Parallel_computing",
    "https://www.bbc.com/news",
    "https://www.python.org",
    "https://cplusplus.com",
    "https://developer.mozilla.org",
    "https://www.geeksforgeeks.org",
    "https://stackoverflow.com",
    "https://www.ibm.com",
    "https://www.nasa.gov",
    "https://www.reuters.com"
};

const std::map<std::string, std::vector<std::pair<std::string, std::string>>> ParallelCrawler::DEMO_PAGES = {
    {"en.wikipedia.org", {
        {"https://en.wikipedia.org/wiki/Parallel_computing",
         "Parallel computing is a type of computation in which many calculations are carried out simultaneously. "
         "Large problems can often be divided into smaller ones, which can then be solved at the same time. "
         "There are several different forms of parallel computing: bit-level, instruction-level, data, and task parallelism. "
         "Parallelism has long been employed in high-performance computing, thread pools, distributed systems, and multi-core processors."},
        {"https://en.wikipedia.org/wiki/Multithreading_(computer_architecture)",
         "In computer architecture, multithreading is the ability of a central processing unit (CPU) or a single core "
         "in a multi-core processor to provide multiple threads of execution concurrently, supported by the operating system. "
         "Parallel threads can share the same process resources while running independently. Python uses the threading module."},
        {"https://en.wikipedia.org/wiki/Thread_(computing)",
         "A thread is the smallest unit of processing that can be performed in an OS. "
         "In most modern operating systems, a thread exists within a process. "
         "Multiple threads within a process share the same data space and can communicate with each other."},
        {"https://en.wikipedia.org/wiki/Distributed_computing",
         "Distributed computing is a field of computer science that studies distributed systems. "
         "A distributed system is a system whose components are located on different networked computers, "
         "which communicate and coordinate their actions by passing messages. The BFS algorithm is widely used."},
        {"https://en.wikipedia.org/wiki/Web_crawler",
         "A web crawler, sometimes called a spider or spiderbot, is an Internet bot that systematically browses the World Wide Web. "
         "Web crawlers are mainly used to create a copy of all visited pages for later processing by a search engine. "
         "Parallel crawling uses multiple threads to improve efficiency."},
    }},
    {"www.bbc.com", {
        {"https://www.bbc.com/news",
         "BBC News - Breaking news, features and analysis from Britain and around the world. "
         "Technology news: AI and machine learning advances continue to reshape industries. "
         "Scientists develop parallel processing algorithms for faster data analysis."},
        {"https://www.bbc.com/news/technology",
         "Technology news from BBC: New developments in parallel computing and distributed systems. "
         "Python remains the top programming language for data science and machine learning. "
         "Thread-based programming models evolve with modern multi-core hardware."},
        {"https://www.bbc.com/news/science-environment",
         "Science news: Researchers publish breakthrough on distributed quantum computing. "
         "High performance computing clusters now use thousands of parallel threads. "
         "Keyword search algorithms power modern information retrieval systems."},
        {"https://www.bbc.com/news/world",
         "World news updates. Economic reports highlight growth in computing sector. "
         "Global technology spending increases with focus on cloud and parallel systems."},
        {"https://www.bbc.com/news/education",
         "Education news: Universities increase enrollment in computer science programs. "
         "Parallel and distributed computing courses are among the fastest growing subjects."},
    }},
    {"www.python.org", {
        {"https://www.python.org",
         "Welcome to Python.org — the official home of the Python programming language. "
         "Python is a versatile, high-level programming language known for its clear syntax. "
         "Python supports threading, multiprocessing, and asyncio for parallel execution."},
        {"https://www.python.org/doc/",
         "Python Documentation — comprehensive guides for all Python modules. "
         "The threading module provides tools to run parallel threads. "
         "The queue module enables thread-safe FIFO data structures for task distribution."},
        {"https://www.python.org/downloads/",
         "Download the latest Python release. Python 3.12 includes improved parallel execution. "
         "The GIL (Global Interpreter Lock) affects CPU-bound thread parallelism in Python. "
         "Use multiprocessing for true parallelism or threading for I/O-bound tasks."},
        {"https://www.python.org/community/",
         "Python community resources. Join the mailing list, forums, and local user groups. "
         "Contribute to open source Python projects on GitHub including crawler libraries."},
        {"https://www.python.org/about/",
         "About Python: Created by Guido van Rossum, Python emphasizes readability and simplicity. "
         "Python is widely used in web development, data science, automation, and threading applications."},
    }},
    {"cplusplus.com", {
        {"https://cplusplus.com",
         "Cplusplus.com is a website dedicated to the C++ programming language. "
         "C++ is widely used for systems programming, game development, and high-performance computing applications. "
         "Modern C++ supports multithreading with std::thread and thread synchronization primitives."},
        {"https://cplusplus.com/reference/thread/",
         "C++ Reference Guide for threading library. Learn about std::thread for creating parallel threads. "
         "Mutex and lock mechanisms enable thread-safe access to shared data. The C++ standard library provides mutex and condition variables."},
        {"https://cplusplus.com/reference/queue/",
         "Queue containers in C++ enable thread-safe data structures for parallel algorithms. "
         "BFS algorithms commonly use queue structures to process nodes in parallel computing systems."},
        {"https://cplusplus.com/reference/",
         "Complete C++ Reference library documentation. Standard Template Library provides tools for parallel programming. "
         "Algorithm library includes parallel execution policies for multi-threaded operations."},
        {"https://cplusplus.com/articles/",
         "C++ Programming articles covering advanced topics including parallel programming techniques. "
         "Explore threading concepts, synchronization primitives, and concurrent data structures."},
    }},
    {"developer.mozilla.org", {
        {"https://developer.mozilla.org",
         "Mozilla Developer Network (MDN) is the official resource for web technologies. "
         "Web standards evolve to support parallel computing through Web Workers and asynchronous operations. "
         "Modern JavaScript enables concurrent processing with promises and async/await patterns."},
        {"https://developer.mozilla.org/en-US/docs/Web/JavaScript",
         "JavaScript documentation covering async programming and Web Workers for parallel processing. "
         "JavaScript can create worker threads for CPU-intensive operations. Promises enable parallel operations coordination."},
        {"https://developer.mozilla.org/en-US/docs/Web/API/Web_Workers_API",
         "Web Workers API enables true parallelism in web applications by offloading work to background threads. "
         "Multiple workers can run in parallel on modern multi-core systems. Message passing coordinates worker communication."},
        {"https://developer.mozilla.org/en-US/docs/Learn/",
         "MDN Learning area provides educational resources on web development and modern parallel programming patterns. "
         "Learn about client-side parallelism using Web Workers and asynchronous JavaScript."},
        {"https://developer.mozilla.org/en-US/docs/Web/HTTP",
         "HTTP protocol reference for understanding web communication. Web crawlers use HTTP to fetch content. "
         "Parallel crawling employs multiple HTTP connections simultaneously for efficient network utilization."},
    }},
    {"www.geeksforgeeks.org", {
        {"https://www.geeksforgeeks.org",
         "GeeksforGeeks provides computer science tutorials and interview preparation resources. "
         "Topics include data structures, algorithms, and parallel computing concepts. "
         "Learn about threading, concurrency control, and distributed computing fundamentals."},
        {"https://www.geeksforgeeks.org/threading-in-java/",
         "Threading tutorials explaining multi-threaded programming concepts. Java and C++ provide thread libraries. "
         "Synchronization mechanisms like mutex ensure thread safety in parallel applications. BFS algorithms benefit from parallel execution."},
        {"https://www.geeksforgeeks.org/queue-data-structure/",
         "Queue data structure is fundamental to parallel algorithms. BFS uses queues for level-order traversal. "
         "Thread-safe queues enable work distribution among parallel worker threads in crawler systems."},
        {"https://www.geeksforgeeks.org/graph-data-structure-and-algorithms/",
         "Graph algorithms including BFS and DFS can be parallelized for faster execution. "
         "Web crawlers model URLs as graphs and traverse them using parallel BFS algorithms."},
        {"https://www.geeksforgeeks.org/operating-system/",
         "Operating System tutorials covering process management and thread synchronization. "
         "Understand mutex, semaphores, and other synchronization primitives used in parallel crawlers."},
    }},
    {"stackoverflow.com", {
        {"https://stackoverflow.com",
         "Stack Overflow is the largest Q&A community for programmers worldwide. "
         "Thousands of questions address threading, concurrency, and parallel programming challenges. "
         "Solutions demonstrate best practices for multi-threaded applications and thread synchronization."},
        {"https://stackoverflow.com/questions/tagged/multithreading",
         "Multithreading questions provide practical solutions for concurrent programming. "
         "Community expertise covers Python threading, C++ std::thread, and platform-specific threading APIs. "
         "Learn from real-world examples of thread-safe coding patterns."},
        {"https://stackoverflow.com/questions/tagged/web-crawler",
         "Web crawler questions discuss parallel crawling strategies and implementation techniques. "
         "Developers share approaches for efficient URL fetching, HTML parsing, and keyword extraction."},
        {"https://stackoverflow.com/questions/tagged/parallel-processing",
         "Parallel processing discussions cover CPU parallelism, GPU computing, and distributed systems. "
         "Insights include thread pools, work queues, and load balancing strategies."},
        {"https://stackoverflow.com/questions/tagged/mutex",
         "Mutex and synchronization primitive questions help developers prevent race conditions. "
         "Learn proper locking strategies for shared data structures in multi-threaded applications."},
    }},
    {"www.ibm.com", {
        {"https://www.ibm.com",
         "IBM leads in enterprise computing, cloud services, and artificial intelligence. "
         "IBM develops parallel computing infrastructure for high-performance scientific computing. "
         "Enterprise systems benefit from distributed and parallel processing technologies."},
        {"https://www.ibm.com/cloud/",
         "IBM Cloud provides scalable computing resources for parallel workloads. "
         "Cloud infrastructure enables distributed crawling across geographic regions. "
         "Parallel data processing powers business intelligence and analytics systems."},
        {"https://www.ibm.com/research/",
         "IBM Research explores cutting-edge technologies including quantum computing and AI. "
         "Research papers discuss parallel algorithms and distributed system optimization. "
         "Quantum computing represents the future of parallel information processing."},
        {"https://www.ibm.com/products/power-systems",
         "IBM Power Systems deliver high-performance parallel computing capabilities. "
         "Multi-core processors enable massive parallelism for complex workloads. "
         "Server infrastructure supports millions of concurrent operations."},
        {"https://www.ibm.com/topics/distributed-computing",
         "IBM discusses distributed computing architecture and parallel processing patterns. "
         "Enterprise solutions rely on parallel databases, caching layers, and work distribution systems."},
    }},
    {"www.nasa.gov", {
        {"https://www.nasa.gov",
         "NASA uses supercomputing and parallel processing for space exploration simulations. "
         "Scientific computing requires massive parallel computing resources for climate and weather models. "
         "NASA's High-End Computing Program develops parallel algorithms for complex physics simulations."},
        {"https://www.nasa.gov/technology/",
         "NASA Technology section highlights innovations in parallel computing for space missions. "
         "Deep space exploration data processing uses distributed parallel systems. "
         "Real-time telemetry streams require high-performance concurrent data processing."},
        {"https://www.nasa.gov/earth/",
         "NASA Earth monitoring systems use parallel processing for satellite data analysis. "
         "Climate models run on supercomputers with thousands of parallel processors. "
         "Earth observation data crawling and indexing employs parallel search algorithms."},
        {"https://www.nasa.gov/astrophysics/",
         "Astrophysics research relies on parallel processing for analyzing astronomical data. "
         "Large survey datasets require parallel keyword search and categorization. "
         "Distributed computing enables analysis of billions of astronomical objects."},
        {"https://www.nasa.gov/space-missions/",
         "Space mission data processing uses parallel pipelines for image analysis. "
         "Rover and satellite data streams are processed using parallel crawling techniques. "
         "Mission planning algorithms benefit from multi-threaded optimization."},
    }},
    {"www.reuters.com", {
        {"https://www.reuters.com",
         "Reuters delivers breaking news through global news crawlers and content aggregation. "
         "News syndication uses parallel crawling to gather stories from thousands of sources. "
         "Real-time keyword search algorithms power content discovery and categorization."},
        {"https://www.reuters.com/technology",
         "Technology news section covers AI, computing trends, and digital transformation. "
         "Articles discuss parallel computing applications in business analytics. "
         "Explore advances in distributed systems and high-performance computing."},
        {"https://www.reuters.com/business",
         "Business news uses parallel search to extract relevant financial information. "
         "Market data aggregation employs distributed crawling for competitive intelligence. "
         "Multi-source content retrieval requires efficient parallel data fetching."},
        {"https://www.reuters.com/science",
         "Science news covers parallel computing breakthroughs and research advances. "
         "Articles discuss distributed systems, artificial intelligence, and quantum computing. "
         "Keyword search across scientific publications requires parallel text processing."},
        {"https://www.reuters.com/graphics",
         "Reuters graphics and multimedia content requires parallel processing pipelines. "
         "Content delivery networks crawl and distribute news globally using parallel systems. "
         "Metadata extraction and keyword indexing employ multi-threaded operations."},
    }},
};

// CrawlerResult implementation
CrawlerResult::CrawlerResult() = default;

std::string CrawlerResult::get_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time), "%H:%M:%S");
    return oss.str();
}

void CrawlerResult::add_log(const std::string& message) {
    std::lock_guard<std::mutex> lock(result_mutex);
    LogEntry entry;
    entry.timestamp = get_timestamp();
    entry.message = message;
    logs.push_back(entry);
    std::cout << "[" << entry.timestamp << "] " << message << std::endl;
}

void CrawlerResult::add_match(const std::string& url, const std::string& site) {
    std::lock_guard<std::mutex> lock(result_mutex);
    matched_urls.push_back({url, site});
}

void CrawlerResult::add_crawled(const std::string& url) {
    std::lock_guard<std::mutex> lock(result_mutex);
    crawled_urls.push_back({url});
}

json CrawlerResult::to_json() const {
    std::lock_guard<std::mutex> lock(result_mutex);
    json result;
    
    for (const auto& match : matched_urls) {
        result["matched_urls"].push_back({
            {"url", match.url},
            {"site", match.site}
        });
    }
    
    for (const auto& crawled : crawled_urls) {
        result["crawled_urls"].push_back(crawled.url);
    }
    
    for (const auto& log : logs) {
        result["logs"].push_back(json::object({
            {"timestamp", log.timestamp},
            {"message", log.message}
        }));
    }
    
    result["total_crawled"] = crawled_urls.size();
    result["total_matches"] = matched_urls.size();
    
    return result;
}

// SiteCrawler implementation
SiteCrawler::SiteCrawler(const std::string& base_url, const std::string& keyword,
                         int max_depth, int max_pages, std::shared_ptr<CrawlerResult> result,
                         int num_workers)
    : base_url(base_url), keyword_lower(keyword), max_depth(max_depth),
      max_pages(max_pages), result(result), num_workers(num_workers) {
    
    // Extract domain from URL
    size_t start = base_url.find("://") + 3;
    size_t end = base_url.find("/", start);
    if (end == std::string::npos) {
        domain = base_url.substr(start);
    } else {
        domain = base_url.substr(start, end - start);
    }
    
    // Convert keyword to lowercase
    std::transform(keyword_lower.begin(), keyword_lower.end(),
                   keyword_lower.begin(), ::tolower);
}

bool SiteCrawler::is_same_domain(const std::string& url) {
    size_t start = url.find("://");
    if (start == std::string::npos) return false;
    start += 3;
    
    size_t end = url.find("/", start);
    std::string url_domain;
    if (end == std::string::npos) {
        url_domain = url.substr(start);
    } else {
        url_domain = url.substr(start, end - start);
    }
    
    return url_domain == domain;
}

std::pair<std::string, std::string> SiteCrawler::fetch_page(const std::string& url) {
    try {
        HttpClient client;
        std::string html = client.fetch(url);
        std::string final_url = client.get_final_url(url);
        if (!html.empty()) {
            return {html, final_url};
        }
    } catch (const std::exception& e) {
        result->add_log(std::string("⚠ Error fetching ") + url + ": " + e.what());
    }
    return {"", url};
}

std::vector<std::string> SiteCrawler::extract_links(const std::string& html, const std::string& /* current_url */) {
    return HtmlParser::extract_links(html);
}

bool SiteCrawler::search_keyword(const std::string& html) {
    return HtmlParser::search_text(html, keyword_lower);
}

void SiteCrawler::worker_thread(int worker_id) {
    std::string thread_name = std::string("Worker-") + domain.substr(0, 15) + "-T" + std::to_string(worker_id);
    
    while (true) {
        std::pair<std::string, int> task;
        
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            if (url_queue.empty()) {
                break;
            }
            task = url_queue.front();
            url_queue.pop();
        }
        
        // Check page limit
        {
            std::lock_guard<std::mutex> lock(pages_mutex);
            if (pages_crawled >= max_pages) {
                break;
            }
            pages_crawled++;
        }
        
        std::string url = task.first;
        int depth = task.second;
        
        result->add_log(std::string("🔍 ") + thread_name + " crawling: " + url.substr(0, 70));
        
        // Fetch page
        auto [html, final_url] = fetch_page(url);
        if (html.empty()) {
            continue;
        }
        
        result->add_crawled(final_url);
        
        // Search for keyword
        if (search_keyword(html)) {
            result->add_log(std::string("✅ KEYWORD FOUND by ") + thread_name + " at: " + final_url.substr(0, 70));
            result->add_match(final_url, domain);
        } else {
            result->add_log(std::string("❌ Keyword not found at: ") + final_url.substr(0, 60));
        }
        
        // Extract links if depth allows
        if (depth < max_depth) {
            auto links = extract_links(html, final_url);
            int new_links_added = 0;
            
            for (const auto& link : links) {
                if (is_same_domain(link)) {
                    {
                        std::lock_guard<std::mutex> lock(visited_mutex);
                        if (visited.find(link) == visited.end()) {
                            visited.insert(link);
                            {
                                std::lock_guard<std::mutex> q_lock(queue_mutex);
                                url_queue.push({link, depth + 1});
                            }
                            new_links_added++;
                        }
                    }
                }
            }
            
            if (new_links_added > 0) {
                result->add_log(std::string("🔗 ") + thread_name + " added " + std::to_string(new_links_added) + 
                              " new URLs (depth " + std::to_string(depth + 1) + ")");
            }
        }
    }
}

void SiteCrawler::crawl() {
    result->add_log(std::string("🚀 Starting crawl for site: ") + base_url);
    
    // Add base URL to visited and queue
    {
        std::lock_guard<std::mutex> lock(visited_mutex);
        visited.insert(base_url);
    }
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        url_queue.push({base_url, 0});
    }
    
    // Create and start worker threads
    std::vector<std::thread> workers;
    for (int i = 1; i <= num_workers; ++i) {
        workers.emplace_back(&SiteCrawler::worker_thread, this, i);
        result->add_log(std::string("🧵 Spawned worker thread ") + std::to_string(i) + " for " + domain);
    }
    
    // Wait for all workers to finish
    for (auto& t : workers) {
        t.join();
    }
    
    result->add_log(std::string("✔ Finished crawling ") + domain + " — " + 
                   std::to_string(pages_crawled) + " pages processed");
}

// ParallelCrawler implementation
ParallelCrawler::ParallelCrawler(const std::string& keyword, int max_depth,
                                 int max_pages, const std::vector<std::string>& sites)
    : keyword(keyword), result(std::make_shared<CrawlerResult>()) {
    
    // Clamp values
    this->max_depth = std::max(1, std::min(max_depth, 3));
    this->max_pages = std::max(2, std::min(max_pages, 20));
    
    if (!sites.empty()) {
        this->sites = sites;
    } else {
        this->sites = PREDEFINED_SITES;
    }
}

bool ParallelCrawler::check_network() {
    try {
        HttpClient client;
        std::string response = client.fetch("https://www.python.org");
        return !response.empty();
    } catch (...) {
        return false;
    }
}

void ParallelCrawler::run_live() {
    std::vector<std::thread> site_threads;
    std::vector<std::shared_ptr<SiteCrawler>> crawlers;
    
    for (size_t idx = 0; idx < sites.size(); ++idx) {
        auto site_url = sites[idx];
        
        auto crawler = std::make_shared<SiteCrawler>(
            site_url, keyword, max_depth, max_pages, result, 2
        );
        
        crawlers.push_back(crawler);
        
        // Use lambda to capture crawler by shared_ptr
        site_threads.emplace_back([this, crawler, idx, site_url]() {
            result->add_log(std::string("🌐 Launching SiteThread-") + std::to_string(idx + 1) + 
                           " for: " + site_url);
            crawler->crawl();
        });
    }
    
    for (auto& t : site_threads) {
        t.join();
    }
}

void ParallelCrawler::run_demo() {
    result->add_log("⚠  No network detected — running in DEMO MODE with simulated pages");
    result->add_log("   (All PDC logic — threads, queues, locks — runs for real!)");
    result->add_log(std::string(50, '='));
    
    std::vector<std::thread> site_threads;
    
    for (size_t idx = 0; idx < sites.size(); ++idx) {
        auto site_url = sites[idx];
        
        // Extract domain
        size_t start = site_url.find("://") + 3;
        size_t end = site_url.find("/", start);
        std::string domain = (end == std::string::npos) ? 
                            site_url.substr(start) : 
                            site_url.substr(start, end - start);
        
        // Find demo pages for this domain
        auto it = DEMO_PAGES.find(domain);
        if (it == DEMO_PAGES.end()) {
            result->add_log(std::string("⚠  No demo data for ") + domain + ", skipping");
            continue;
        }
        
        auto pages = it->second;
        
        // Create thread for this site
        site_threads.emplace_back([this, idx, site_url, domain, pages]() {
            std::string thread_name = std::string("SiteThread-") + std::to_string(idx + 1);
            result->add_log(std::string("🚀 ") + thread_name + " starting: " + site_url);
            
            std::queue<std::tuple<std::string, int, std::string>> bfs_queue;
            std::set<std::string> visited;
            std::mutex visited_lock;
            std::mutex page_idx_lock;
            
            bfs_queue.push({pages[0].first, 0, pages[0].second});
            visited.insert(pages[0].first);
            
            size_t page_idx = 1;
            int pages_done = 0;
            
            std::string keyword_lower = keyword;
            std::transform(keyword_lower.begin(), keyword_lower.end(),
                         keyword_lower.begin(), ::tolower);
            
            // Worker function
            auto worker_func = [&](int worker_id) {
                std::string wname = std::string("Worker-") + domain.substr(0, 10) + "-T" + std::to_string(worker_id);
                result->add_log(std::string("🧵 Spawned ") + wname);
                
                while (true) {
                    std::tuple<std::string, int, std::string> task;
                    
                    {
                        std::lock_guard<std::mutex> lock(page_idx_lock);
                        if (bfs_queue.empty()) {
                            break;
                        }
                        pages_done++;
                        if (pages_done > max_pages) {
                            break;
                        }
                        task = bfs_queue.front();
                        bfs_queue.pop();
                    }
                    
                    auto [url, depth, content] = task;
                    
                    result->add_log(std::string("🔍 ") + wname + " crawling [" + std::to_string(pages_done) + 
                                   "/" + std::to_string(max_pages) + "]: " + url.substr(0, 65));
                    result->add_crawled(url);
                    
                    // Keyword search
                    std::string content_lower = content;
                    std::transform(content_lower.begin(), content_lower.end(),
                                 content_lower.begin(), ::tolower);
                    
                    if (content_lower.find(keyword_lower) != std::string::npos) {
                        result->add_log(std::string("✅ KEYWORD FOUND by ") + wname + ": " + url.substr(0, 65));
                        result->add_match(url, domain);
                    } else {
                        result->add_log(std::string("❌ Keyword not found: ") + url.substr(0, 55));
                    }
                    
                    // BFS: queue next pages
                    if (depth < max_depth) {
                        {
                            std::lock_guard<std::mutex> lock(page_idx_lock);
                            if (page_idx < pages.size()) {
                                auto next_page = pages[page_idx];
                                page_idx++;
                                
                                {
                                    std::lock_guard<std::mutex> vlock(visited_lock);
                                    if (visited.find(next_page.first) == visited.end()) {
                                        visited.insert(next_page.first);
                                        bfs_queue.push({next_page.first, depth + 1, next_page.second});
                                        result->add_log(std::string("🔗 ") + wname + " added URL (depth " + 
                                                       std::to_string(depth + 1) + "): " + next_page.first.substr(0, 55));
                                    }
                                }
                            }
                        }
                    }
                }
                
                result->add_log(std::string("✔ ") + wname + " finished");
            };
            
            // Create worker threads
            std::vector<std::thread> workers;
            for (int i = 1; i <= 2; ++i) {
                workers.emplace_back(worker_func, i);
            }
            
            for (auto& t : workers) {
                t.join();
            }
            
            result->add_log(std::string("✔ ") + thread_name + " done — " + std::to_string(pages_done) + 
                           " pages processed on " + domain);
        });
    }
    
    for (auto& t : site_threads) {
        t.join();
    }
}

json ParallelCrawler::run() {
    result->add_log(std::string(50, '='));
    result->add_log("🕷  Parallel Web Crawler Started");
    result->add_log(std::string("🔑 Keyword: '") + keyword + "'");
    result->add_log(std::string("📐 Max Depth: ") + std::to_string(max_depth) + 
                   " | Max Pages/Site: " + std::to_string(max_pages));
    result->add_log(std::string("🌐 Sites: ") + std::to_string(sites.size()));
    result->add_log(std::string(50, '='));
    
    // Check network and decide between live/demo mode
    if (check_network()) {
        result->add_log("🌐 Network available — running LIVE crawl");
        run_live();
    } else {
        run_demo();
    }
    
    result->add_log(std::string(50, '='));
    result->add_log("🏁 Crawl Complete!");
    result->add_log(std::string("📊 Total pages crawled: ") + 
                   std::to_string(result->to_json()["total_crawled"].get<int>()));
    result->add_log(std::string("✅ Total keyword matches: ") + 
                   std::to_string(result->to_json()["total_matches"].get<int>()));
    result->add_log(std::string(50, '='));
    
    return result->to_json();
}
