#include "http_client.h"
#include <stdexcept>

size_t HttpClient::write_callback(void* contents, size_t size, size_t nmemb, std::string* user_data) {
    size_t real_size = size * nmemb;
    user_data->append((char*)contents, real_size);
    return real_size;
}

HttpClient::HttpClient() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl_handle = curl_easy_init();
    if (!curl_handle) {
        throw std::runtime_error("Failed to initialize CURL");
    }
}

HttpClient::~HttpClient() {
    if (curl_handle) {
        curl_easy_cleanup(curl_handle);
    }
    curl_global_cleanup();
}

std::string HttpClient::fetch(const std::string& url) {
    std::string response_data;
    
    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &response_data);
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 5L);
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "Mozilla/5.0 (PDC-Crawler/1.0)");
    
    CURLcode res = curl_easy_perform(curl_handle);
    
    if (res != CURLE_OK) {
        throw std::runtime_error(std::string("CURL error: ") + curl_easy_strerror(res));
    }
    
    return response_data;
}

std::string HttpClient::get_final_url(const std::string& url) {
    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 5L);
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    
    // Perform a HEAD request to get final URL
    curl_easy_setopt(curl_handle, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "Mozilla/5.0 (PDC-Crawler/1.0)");
    
    CURLcode res = curl_easy_perform(curl_handle);
    curl_easy_setopt(curl_handle, CURLOPT_NOBODY, 0L);
    
    if (res != CURLE_OK) {
        return url;
    }
    
    char* final_url = nullptr;
    curl_easy_getinfo(curl_handle, CURLINFO_EFFECTIVE_URL, &final_url);
    
    if (final_url) {
        return std::string(final_url);
    }
    
    return url;
}
