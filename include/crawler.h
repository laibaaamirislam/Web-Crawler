#ifndef CRAWLER_H
#define CRAWLER_H

#include <string>
#include <vector>
#include <set>
#include <queue>
#include <mutex>
#include <thread>
#include <memory>
#include <chrono>
#include <map>
#include <json/json.hpp>

using json = nlohmann::json;

// Result structure for a crawl match
struct URLMatch {
    std::string url;
    std::string site;
};

// Crawled URL record
struct CrawledURL {
    std::string url;
};

// Log entry
struct LogEntry {
    std::string timestamp;
    std::string message;
};

// Result holder class
class CrawlerResult {
public:
    CrawlerResult();
    ~CrawlerResult() = default;

    void add_log(const std::string& message);
    void add_match(const std::string& url, const std::string& site);
    void add_crawled(const std::string& url);

    json to_json() const;

private:
    std::vector<URLMatch> matched_urls;
    std::vector<CrawledURL> crawled_urls;
    std::vector<LogEntry> logs;
    mutable std::mutex result_mutex;

    std::string get_timestamp();
};

// Single site crawler class
class SiteCrawler {
public:
    SiteCrawler(const std::string& base_url, const std::string& keyword,
                int max_depth, int max_pages, std::shared_ptr<CrawlerResult> result,
                int num_workers = 2);
    ~SiteCrawler() = default;

    void crawl();

private:
    std::string base_url;
    std::string domain;
    std::string keyword_lower;
    int max_depth;
    int max_pages;
    std::shared_ptr<CrawlerResult> result;
    int num_workers;

    // Thread-safe structures
    std::queue<std::pair<std::string, int>> url_queue;  // (url, depth)
    std::set<std::string> visited;
    mutable std::mutex visited_mutex;
    mutable std::mutex pages_mutex;
    mutable std::mutex queue_mutex;
    int pages_crawled = 0;

    // Helper methods
    bool is_same_domain(const std::string& url);
    std::pair<std::string, std::string> fetch_page(const std::string& url);
    std::vector<std::string> extract_links(const std::string& html, const std::string& current_url);
    bool search_keyword(const std::string& html);
    void worker_thread(int worker_id);
};

// Main parallel crawler class
class ParallelCrawler {
public:
    static const std::vector<std::string> PREDEFINED_SITES;
    static const std::map<std::string, std::vector<std::pair<std::string, std::string>>> DEMO_PAGES;

    ParallelCrawler(const std::string& keyword, int max_depth = 1,
                    int max_pages = 10, 
                    const std::vector<std::string>& sites = {});
    ~ParallelCrawler() = default;

    json run();

private:
    std::string keyword;
    int max_depth;
    int max_pages;
    std::vector<std::string> sites;
    std::shared_ptr<CrawlerResult> result;

    bool check_network();
    void run_live();
    void run_demo();
};

#endif // CRAWLER_H
