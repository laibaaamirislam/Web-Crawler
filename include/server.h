#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <memory>
#include <json/json.hpp>

using json = nlohmann::json;

class HttpServer {
public:
    HttpServer(int port = 5000);
    ~HttpServer();

    void start();
    void stop();

private:
    int port;
    int server_socket = -1;
    bool running = false;

    // Request handling
    void handle_client(int client_socket);
    std::string parse_request(const std::string& request_data, std::string& method, std::string& path);
    
    // Endpoint handlers
    json handle_get_root();
    json handle_get_sites();
    json handle_post_crawl(const std::string& body);
    std::string handle_get_viewer(const std::string& query_string);

    // HTTP response helpers
    std::string create_http_response(const std::string& content_type, const std::string& body, int status_code = 200);
    std::string create_json_response(const json& data, int status_code = 200);
    std::string create_error_response(const std::string& error_message, int status_code = 400);

    // Utility helpers
    std::string serve_static_file(const std::string& file_path);
    std::string get_mime_type(const std::string& file_path);
};

#endif // SERVER_H
