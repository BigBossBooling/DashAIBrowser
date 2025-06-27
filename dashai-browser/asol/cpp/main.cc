#include "asol/cpp/asol_gateway_server.h"
#include <iostream>
#include <string>
#include <csignal> // For signal handling (Ctrl+C)
#include <memory>  // For std::unique_ptr

// Global pointer to the server instance to allow signal handlers to access it.
// This is a common, though simplistic, way to handle shutdown on Ctrl+C.
// More robust applications might use a dedicated shutdown mechanism.
std::unique_ptr<dashaibrowser::asol::AsolGatewayServer> g_server_instance = nullptr;

void SignalHandler(int signum) {
    std::cout << "\nInterrupt signal (" << signum << ") received." << std::endl;
    if (g_server_instance) {
        std::cout << "Attempting to shut down ASOL Gateway Server..." << std::endl;
        g_server_instance->Shutdown();
    } else {
        std::cout << "Server instance not available for shutdown." << std::endl;
        exit(signum);
    }
}

int main(int argc, char** argv) {
    // Register signal SIGINT and SIGTERM handlers
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    std::string server_address("0.0.0.0:50051"); // Default address

    // Allow overriding server address via command line argument (optional)
    if (argc > 1) {
        server_address = argv[1];
    }

    std::cout << "ASOL Gateway starting up..." << std::endl;

    g_server_instance = std::make_unique<dashaibrowser::asol::AsolGatewayServer>();

    std::cout << "Attempting to run server on address: " << server_address << std::endl;
    g_server_instance->Run(server_address); // This call will block until the server is shut down.

    std::cout << "ASOL Gateway has shut down gracefully." << std::endl;

    return 0;
}
