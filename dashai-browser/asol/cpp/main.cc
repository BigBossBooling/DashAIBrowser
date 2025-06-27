#include "asol/cpp/asol_gateway_server.h"
#include "asol/cpp/utils/curl_http_client.h" // For GlobalInit/Cleanup
#include <iostream>
#include <string>
#include <csignal> // For signal handling (Ctrl+C)
#include <memory>  // For std::unique_ptr

std::unique_ptr<dashaibrowser::asol::AsolGatewayServer> g_server_instance = nullptr;

void SignalHandler(int signum) {
    std::cout << "\nInterrupt signal (" << signum << ") received." << std::endl;
    if (g_server_instance) {
        std::cout << "Attempting to shut down ASOL Gateway Server..." << std::endl;
        g_server_instance->Shutdown();
    } else {
        std::cout << "Server instance not available for shutdown." << std::endl;
        // If server instance is null, GlobalCleanup might not have been reached either.
        // Call it here just in case, though ideally it's after Run() finishes.
        dashaibrowser::asol::utils::CurlHttpClient::GlobalCleanup();
        exit(signum);
    }
}

int main(int argc, char** argv) {
    // Initialize libcurl globally at the start of the application.
    if (!dashaibrowser::asol::utils::CurlHttpClient::GlobalInit()) {
        std::cerr << "Failed to initialize libcurl. Exiting." << std::endl;
        return 1;
    }

    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    std::string server_address("0.0.0.0:50051");
    if (argc > 1) {
        server_address = argv[1];
    }

    std::cout << "ASOL Gateway starting up..." << std::endl;
    g_server_instance = std::make_unique<dashaibrowser::asol::AsolGatewayServer>();

    std::cout << "Attempting to run server on address: " << server_address << std::endl;
    g_server_instance->Run(server_address);

    std::cout << "ASOL Gateway has shut down gracefully." << std::endl;

    // Cleanup libcurl globally at the end of the application.
    dashaibrowser::asol::utils::CurlHttpClient::GlobalCleanup();
    return 0;
}
