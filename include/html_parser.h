#ifndef HTML_PARSER_H
#define HTML_PARSER_H

#include <string>
#include <vector>

class HtmlParser {
public:
    // Extract all links from HTML content
    static std::vector<std::string> extract_links(const std::string& html);

    // Extract text content from HTML (removes tags)
    static std::string extract_text(const std::string& html);

    // Check if keyword exists in HTML
    static bool search_text(const std::string& html, const std::string& keyword);
};

#endif // HTML_PARSER_H
