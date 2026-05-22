#include "server.h"
#include <iostream>
#include <signal.h>

HttpServer* g_server = nullptr;

void signal_handler(int signal) {
    if (signal == SIGINT) {
        std::cout << "\n\nShutting down server..." << std::endl;
        if (g_server) {
            g_server->stop();
        }
        exit(0);
    }
}

int main() {
    // Set signal handler for graceful shutdown
    signal(SIGINT, signal_handler);
    
    // Create and start server
    HttpServer server(5000);
    g_server = &server;
    
    server.start();
    
    return 0;
}
