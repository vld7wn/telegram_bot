#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <functional>

// Forward declare Bot for callbacks
namespace TgBot {
    class Bot;
}

class HttpServer {
public:
    HttpServer(TgBot::Bot& bot);
    ~HttpServer();

    // Start the HTTP server on specified port
    void start(int port = 8080);
    
    // Stop the HTTP server
    void stop();
    
    // Check if server is running
    bool isRunning() const;

private:
    TgBot::Bot& bot_;
    std::thread server_thread_;
    std::atomic<bool> running_{false};
    int port_;

    // Setup all API routes
    void setupRoutes();
    
    // Thread function
    void serverLoop();
};

// Helper function to get server instance (singleton pattern for global access)
HttpServer* getHttpServer();
void initHttpServer(TgBot::Bot& bot);
