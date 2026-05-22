cmake_minimum_required(VERSION 3.10)
project(parallel_web_crawler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -Wall -Wextra")

# Find required packages
find_package(CURL REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(Threads REQUIRED)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CURL_INCLUDE_DIRS})
include_directories(${LIBXML2_INCLUDE_DIR})

# Add nlohmann/json header-only library
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/main.cpp
    src/server.cpp
    src/crawler.cpp
    src/http_client.cpp
    src/html_parser.cpp
)

# Create executable
add_executable(parallel_web_crawler ${SOURCES})

# Link libraries
target_link_libraries(parallel_web_crawler 
    ${CURL_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    Threads::Threads
)

# Installation
install(TARGETS parallel_web_crawler DESTINATION bin)
