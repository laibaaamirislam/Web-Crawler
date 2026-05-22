#include "server.h"
#include "crawler.h"
#include "http_client.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>
#include <cctype>
#include <regex>

HttpServer::HttpServer(int port) : port(port) {}

HttpServer::~HttpServer() {
    stop();
}

// Helper function to escape JavaScript string literals
static std::string js_escape(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '\\': result += "\\\\"; break;
            case '\'': result += "\\'"; break;
            case '"': result += "\\\""; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c; break;
        }
    }
    return result;
}

// Helper function to escape regex special characters
static std::string regex_escape(const std::string& str) {
    std::string result;
    for (char c : str) {
        if (c == '\\' || c == '.' || c == '*' || c == '+' || c == '?' || 
            c == '[' || c == ']' || c == '{' || c == '}' || c == '(' || 
            c == ')' || c == '|' || c == '^' || c == '$') {
            result += '\\';
        }
        result += c;
    }
    return result;
}

void HttpServer::start() {
    running = true;
    
    // Create server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        std::cerr << "Error: Could not create socket" << std::endl;
        return;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Error: Could not set socket options" << std::endl;
        close(server_socket);
        return;
    }
    
    // Bind socket
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Error: Could not bind socket to port " << port << std::endl;
        close(server_socket);
        return;
    }
    
    // Listen for connections
    if (listen(server_socket, 128) < 0) {
        std::cerr << "Error: Could not listen on socket" << std::endl;
        close(server_socket);
        return;
    }
    
    std::cout << "=" << std::string(53, '=') << std::endl;
    std::cout << "  Parallel Web Crawler — C++ Server Starting" << std::endl;
    std::cout << "  Open: http://127.0.0.1:" << port << std::endl;
    std::cout << "=" << std::string(53, '=') << std::endl;
    
    // Accept connections
    while (running) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_socket < 0) {
            if (running) {
                std::cerr << "Error: Could not accept connection" << std::endl;
            }
            continue;
        }
        
        // Handle client in a separate thread
        std::thread(&HttpServer::handle_client, this, client_socket).detach();
    }
    
    close(server_socket);
}

void HttpServer::stop() {
    running = false;
    if (server_socket >= 0) {
        close(server_socket);
        server_socket = -1;
    }
}

void HttpServer::handle_client(int client_socket) {
    // Read request
    char buffer[8192] = {0};
    ssize_t bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_read < 0) {
        close(client_socket);
        return;
    }
    
    std::string request_data(buffer);
    std::string method, path;
    std::string body = parse_request(request_data, method, path);
    
    // Extract base path and query string
    size_t query_pos = path.find('?');
    std::string base_path = path;
    std::string query_string = "";
    if (query_pos != std::string::npos) {
        base_path = path.substr(0, query_pos);
        query_string = path.substr(query_pos + 1);
    }
    
    // Route the request
    std::string response;
    
    if (method == "GET") {
        if (base_path == "/" || base_path.empty()) {
            // Serve the index.html file
            response = serve_static_file("/templates/index.html");
        } else if (base_path == "/sites") {
            response = create_json_response(handle_get_sites(), 200);
        } else if (base_path == "/viewer") {
            // New viewer endpoint for keyword highlighting
            response = handle_get_viewer(query_string);
        } else if (base_path.find("/static/") == 0 || base_path.find("/templates/") == 0) {
            // Serve static files
            response = serve_static_file(base_path);
        } else {
            json error_json;
            error_json["error"] = "Not Found";
            response = create_json_response(error_json, 404);
        }
    } else if (method == "POST") {
        if (base_path == "/crawl") {
            try {
                json result = handle_post_crawl(body);
                response = create_json_response(result, 200);
            } catch (const std::exception& e) {
                response = create_error_response(std::string("Error: ") + e.what(), 400);
            }
        } else {
            json error_json;
            error_json["error"] = "Not Found";
            response = create_json_response(error_json, 404);
        }
    } else {
        json error_json;
        error_json["error"] = "Method Not Allowed";
        response = create_json_response(error_json, 405);
    }
    
    // Send response
    send(client_socket, response.c_str(), response.length(), 0);
    close(client_socket);
}

std::string HttpServer::parse_request(const std::string& request_data, std::string& method, std::string& path) {
    std::istringstream iss(request_data);
    std::string line;
    
    // Parse first line: METHOD PATH HTTP/VERSION
    if (std::getline(iss, line)) {
        std::istringstream first_line(line);
        first_line >> method >> path;
        // Keep the full path including query string
    }
    
    // Find body (after empty line)
    std::string body;
    std::string header_line;
    bool headers_done = false;
    
    while (std::getline(iss, header_line)) {
        if (header_line.empty() || header_line == "\r") {
            headers_done = true;
            break;
        }
    }
    
    if (headers_done) {
        std::getline(iss, body);
    }
    
    return body;
}

json HttpServer::handle_get_root() {
    json result;
    result["status"] = "OK";
    result["message"] = "Parallel Web Crawler API Server";
    return result;
}

json HttpServer::handle_get_sites() {
    json result;
    for (const auto& site : ParallelCrawler::PREDEFINED_SITES) {
        result["sites"].push_back(site);
    }
    return result;
}

json HttpServer::handle_post_crawl(const std::string& body) {
    // Parse JSON body
    json data = json::parse(body);
    
    // Extract parameters
    std::string keyword = data.value("keyword", "");
    int max_depth = data.value("max_depth", 1);
    int max_pages = data.value("max_pages", 8);
    
    if (keyword.empty()) {
        throw std::runtime_error("Keyword is required");
    }
    
    // Create crawler and run
    ParallelCrawler crawler(keyword, max_depth, max_pages);
    json result = crawler.run();
    
    return result;
}

// Helper function to URL decode a string
static std::string url_decode(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            char hex_str[3] = {str[i + 1], str[i + 2], '\0'};
            // Validate that both characters are valid hex digits
            bool valid_hex = (std::isxdigit(static_cast<unsigned char>(hex_str[0])) && 
                             std::isxdigit(static_cast<unsigned char>(hex_str[1])));
            if (valid_hex) {
                int hex_value = 0;
                std::sscanf(hex_str, "%2x", &hex_value);
                result += static_cast<char>(hex_value);
                i += 2;
            } else {
                // Invalid hex sequence, keep the % character
                result += str[i];
            }
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    return result;
}

// Helper function to parse query parameters
static std::map<std::string, std::string> parse_query_params(const std::string& query_string) {
    std::map<std::string, std::string> params;
    std::istringstream iss(query_string);
    std::string pair;
    
    while (std::getline(iss, pair, '&')) {
        size_t eq_pos = pair.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = url_decode(pair.substr(0, eq_pos));
            std::string value = url_decode(pair.substr(eq_pos + 1));
            params[key] = value;
        }
    }
    return params;
}

std::string HttpServer::handle_get_viewer(const std::string& query_string) {
    // Parse query parameters
    auto params = parse_query_params(query_string);
    
    std::string url = params.count("url") ? params["url"] : "";
    std::string keyword = params.count("keyword") ? params["keyword"] : "";
    
    if (url.empty() || keyword.empty()) {
        std::string html = "<html><head><title>Error</title></head><body>";
        html += "<h1>Error</h1><p>Missing url or keyword parameter.</p>";
        html += "</body></html>";
        return create_http_response("text/html", html, 400);
    }
    
    // Try to fetch the content
    std::string content;
    
    // First, try to get from demo pages
    bool found_demo = false;
    for (const auto& domain_entry : ParallelCrawler::DEMO_PAGES) {
        for (const auto& page_pair : domain_entry.second) {
            if (page_pair.first == url) {
                content = page_pair.second;
                found_demo = true;
                break;
            }
        }
        if (found_demo) break;
    }
    
    // If not found in demo, try to fetch from network
    if (!found_demo) {
        try {
            HttpClient client;
            content = client.fetch(url);
            // Remove HTML tags for simpler display
            std::regex tag_regex("<[^>]*>");
            content = std::regex_replace(content, tag_regex, "");
        } catch (...) {
            std::string html = "<html><head><title>Error</title></head><body>";
            html += "<h1>Error</h1><p>Could not fetch the URL: " + url + "</p>";
            html += "</body></html>";
            return create_http_response("text/html", html, 400);
        }
    }
    
    // HTML escape content
    auto escape_html = [](const std::string& text) -> std::string {
        std::string result;
        for (char c : text) {
            switch (c) {
                case '&': result += "&amp;"; break;
                case '<': result += "&lt;"; break;
                case '>': result += "&gt;"; break;
                case '"': result += "&quot;"; break;
                case '\'': result += "&#39;"; break;
                default: result += c; break;
            }
        }
        return result;
    };
    
    std::string escaped_content = escape_html(content);
    std::string escaped_keyword = escape_html(keyword);
    std::string escaped_url = escape_html(url);
    std::string js_escaped_keyword = js_escape(keyword);
    
    // Build HTML piece by piece
    std::string html;
    html += "<!DOCTYPE html>\n";
    html += "<html>\n";
    html += "<head>\n";
    html += "    <meta charset=\"UTF-8\">\n";
    html += "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    html += "    <title>Viewer - Parallel Web Crawler</title>\n";
    html += "    <style>\n";
    html += "        * { margin: 0; padding: 0; box-sizing: border-box; }\n";
    html += "        body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; ";
    html += "background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); ";
    html += "min-height: 100vh; padding: 20px; }\n";
    html += "        .viewer-container { max-width: 900px; margin: 0 auto; background: white; ";
    html += "border-radius: 12px; box-shadow: 0 20px 60px rgba(0, 0, 0, 0.3); overflow: hidden; }\n";
    html += "        .viewer-header { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); ";
    html += "color: white; padding: 24px; display: flex; justify-content: space-between; align-items: center; }\n";
    html += "        .viewer-title { font-size: 20px; font-weight: bold; }\n";
    html += "        .viewer-url { font-size: 12px; opacity: 0.8; margin-top: 8px; word-break: break-all; font-family: monospace; }\n";
    html += "        .viewer-buttons { display: flex; gap: 8px; }\n";
    html += "        .viewer-btn { padding: 8px 16px; background: rgba(255, 255, 255, 0.2); color: white; ";
    html += "border: 1px solid rgba(255, 255, 255, 0.5); border-radius: 6px; cursor: pointer; ";
    html += "font-size: 12px; text-decoration: none; display: inline-block; transition: all 0.2s; }\n";
    html += "        .viewer-btn:hover { background: rgba(255, 255, 255, 0.3); border-color: rgba(255, 255, 255, 0.8); }\n";
    html += "        .viewer-content { padding: 30px; line-height: 1.6; color: #333; max-height: 600px; overflow-y: auto; }\n";
    html += "        .highlight.found { background-color: #ff6b6b; color: white; padding: 2px 6px; border-radius: 3px; font-weight: bold; }\n";
    html += "        .viewer-info { background: #f5f5f5; padding: 16px 24px; border-top: 1px solid #ddd; font-size: 12px; color: #666; }\n";
    html += "        .keyword-badge { display: inline-block; background: #ff6b6b; color: white; padding: 4px 10px; border-radius: 4px; font-weight: bold; margin: 0 4px; }\n";
    html += "    </style>\n";
    html += "</head>\n";
    html += "<body>\n";
    html += "    <div class=\"viewer-container\">\n";
    html += "        <div class=\"viewer-header\">\n";
    html += "            <div>\n";
    html += "                <div class=\"viewer-title\">Page Viewer - Keyword Highlight</div>\n";
    html += "                <div class=\"viewer-url\">Source: " + escaped_url + "</div>\n";
    html += "            </div>\n";
    html += "            <div class=\"viewer-buttons\">\n";
    html += "                <button class=\"viewer-btn\" onclick=\"history.back()\">&larr; Back</button>\n";
    html += "                <a href=\"" + escaped_url + "\" class=\"viewer-btn\" target=\"_blank\">Open Original</a>\n";
    html += "            </div>\n";
    html += "        </div>\n";
    html += "        \n";
    html += "        <div class=\"viewer-content\" id=\"content\">\n";
    html += escaped_content + "\n";
    html += "        </div>\n";
    html += "        \n";
    html += "        <div class=\"viewer-info\">\n";
    html += "            Highlighting keyword: <span class=\"keyword-badge\">" + escaped_keyword + "</span>\n";
    html += "        </div>\n";
    html += "    </div>\n";
    html += "    \n";
    html += "    <script>\n";
    html += "        document.addEventListener('DOMContentLoaded', function() {\n";
    html += "            highlightKeyword('" + js_escaped_keyword + "');\n";
    html += "        });\n";
    html += "        \n";
    html += "        function highlightKeyword(keyword) {\n";
    html += "            if (!keyword) return;\n";
    html += "            \n";
    html += "            const contentDiv = document.getElementById('content');\n";
    html += "            const text = contentDiv.textContent;\n";
    html += "            \n";
    html += "            // Escape regex special characters in keyword\n";
    html += "            const escapedKeyword = keyword.replace(/[.*+?^${}()|[\\]\\\\]/g, '\\\\$&');\n";
    html += "            const regex = new RegExp('\\\\b' + escapedKeyword + '\\\\b', 'gi');\n";
    html += "            const matches = text.match(regex);\n";
    html += "            \n";
    html += "            if (matches && matches.length > 0) {\n";
    html += "                let html = contentDiv.innerHTML;\n";
    html += "                const placeholder = '___PLACEHOLDER___';\n";
    html += "                const htmlTags = [];\n";
    html += "                html = html.replace(/<[^>]*>/g, (tag) => {\n";
    html += "                    htmlTags.push(tag);\n";
    html += "                    return placeholder + (htmlTags.length - 1) + placeholder;\n";
    html += "                });\n";
    html += "                \n";
    html += "                html = html.replace(new RegExp('\\\\b' + escapedKeyword + '\\\\b', 'gi'), \n";
    html += "                    '<span class=\"highlight found\">$&</span>');\n";
    html += "                \n";
    html += "                for (let i = 0; i < htmlTags.length; i++) {\n";
    html += "                    html = html.split(placeholder + i + placeholder).join(htmlTags[i]);\n";
    html += "                }\n";
    html += "                \n";
    html += "                contentDiv.innerHTML = html;\n";
    html += "                \n";
    html += "                const count = matches.length;\n";
    html += "                document.querySelector('.viewer-info').innerHTML = \n";
    html += "                    'Highlighting keyword: <span class=\"keyword-badge\">" + escaped_keyword + "</span> (' + count + ' occurrence' + (count !== 1 ? 's' : '') + ' found)';\n";
    html += "            }\n";
    html += "        }\n";
    html += "    </script>\n";
    html += "</body>\n";
    html += "</html>\n";
    
    return create_http_response("text/html", html, 200);
}

std::string HttpServer::create_http_response(const std::string& content_type, 
                                             const std::string& body, int status_code) {
    std::ostringstream response;
    
    std::string status_text;
    if (status_code == 200) status_text = "OK";
    else if (status_code == 400) status_text = "Bad Request";
    else if (status_code == 404) status_text = "Not Found";
    else if (status_code == 405) status_text = "Method Not Allowed";
    else status_text = "Unknown";
    
    response << "HTTP/1.1 " << status_code << " " << status_text << "\r\n";
    response << "Content-Type: " << content_type << "\r\n";
    response << "Content-Length: " << body.length() << "\r\n";
    response << "Access-Control-Allow-Origin: *\r\n";
    response << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
    response << "Access-Control-Allow-Headers: Content-Type\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << body;
    
    return response.str();
}

std::string HttpServer::create_json_response(const json& data, int status_code) {
    return create_http_response("application/json", data.dump(), status_code);
}

std::string HttpServer::create_error_response(const std::string& error_message, int status_code) {
    json error_json;
    error_json["error"] = error_message;
    return create_json_response(error_json, status_code);
}

std::string HttpServer::serve_static_file(const std::string& file_path) {
    std::string full_path = "/home/runner/work/Parallel-Web-Crawler/Parallel-Web-Crawler" + file_path;
    
    std::ifstream file(full_path, std::ios::binary);
    if (!file.is_open()) {
        json error_json;
        error_json["error"] = "File not found";
        return create_json_response(error_json, 404);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    std::string mime_type = get_mime_type(file_path);
    return create_http_response(mime_type, content, 200);
}

std::string HttpServer::get_mime_type(const std::string& file_path) {
    if (file_path.find(".html") != std::string::npos) return "text/html";
    if (file_path.find(".css") != std::string::npos) return "text/css";
    if (file_path.find(".js") != std::string::npos) return "application/javascript";
    if (file_path.find(".json") != std::string::npos) return "application/json";
    if (file_path.find(".png") != std::string::npos) return "image/png";
    if (file_path.find(".jpg") != std::string::npos || file_path.find(".jpeg") != std::string::npos) 
        return "image/jpeg";
    if (file_path.find(".gif") != std::string::npos) return "image/gif";
    if (file_path.find(".svg") != std::string::npos) return "image/svg+xml";
    
    return "application/octet-stream";
}
