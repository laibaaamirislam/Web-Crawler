#include "html_parser.h"
#include <algorithm>
#include <cstring>
#include <regex>

std::vector<std::string> HtmlParser::extract_links(const std::string& html) {
    std::vector<std::string> links;
    
    if (html.empty()) {
        return links;
    }
    
    // Use regex to find href attributes in anchor tags
    // Pattern: href="..." or href='...' or href=...
    std::regex href_pattern(R"(href\s*=\s*[\"']?([^\s\"'<>]+)[\"']?)", std::regex::icase);
    std::sregex_iterator iter(html.begin(), html.end(), href_pattern);
    std::sregex_iterator end;
    
    while (iter != end) {
        std::string link = (*iter)[1];
        
        // Keep valid URLs
        if (!link.empty() && (link.substr(0, 4) == "http" || link[0] == '/' || 
                              link.substr(0, 1) == "." || link.find('#') == 0)) {
            links.push_back(link);
        }
        ++iter;
    }
    
    return links;
}

std::string HtmlParser::extract_text(const std::string& html) {
    std::string text = html;
    
    if (html.empty()) {
        return text;
    }
    
    // Remove script and style tags with their content
    std::regex script_pattern(R"(<script[^>]*>.*?</script>)", std::regex::icase);
    text = std::regex_replace(text, script_pattern, " ");
    
    std::regex style_pattern(R"(<style[^>]*>.*?</style>)", std::regex::icase);
    text = std::regex_replace(text, style_pattern, " ");
    
    // Remove HTML tags
    std::regex tag_pattern(R"(<[^>]*>)");
    text = std::regex_replace(text, tag_pattern, " ");
    
    // Decode HTML entities (basic set)
    std::regex entity_pattern(R"(&[a-zA-Z]+;|&#\d+;|&#x[0-9a-fA-F]+;)");
    text = std::regex_replace(text, entity_pattern, " ");
    
    // Clean up extra whitespace
    std::regex whitespace_pattern(R"(\s+)");
    text = std::regex_replace(text, whitespace_pattern, " ");
    
    return text;
}

bool HtmlParser::search_text(const std::string& html, const std::string& keyword) {
    if (html.empty() || keyword.empty()) {
        return false;
    }
    
    // Extract text content
    std::string text = extract_text(html);
    
    // Convert to lowercase for case-insensitive search
    std::string text_lower = text;
    std::transform(text_lower.begin(), text_lower.end(),
                   text_lower.begin(), ::tolower);
    
    std::string keyword_lower = keyword;
    std::transform(keyword_lower.begin(), keyword_lower.end(),
                   keyword_lower.begin(), ::tolower);
    
    // Check if keyword exists in text
    return text_lower.find(keyword_lower) != std::string::npos;
}
