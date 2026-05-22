#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <string>
#include <curl/curl.h>

class HttpClient {
public:
    HttpClient();
    ~HttpClient();

    // Fetch a URL and return the response body
    std::string fetch(const std::string& url);

    // Get the final URL after redirects
    std::string get_final_url(const std::string& url);

private:
    CURL* curl_handle;

    // Callback for write data
    static size_t write_callback(void* contents, size_t size, size_t nmemb, std::string* user_data);
};

#endif // HTTP_CLIENT_H
