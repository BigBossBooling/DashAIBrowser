#include "asol/cpp/asol_gateway_server.h"
#include <iostream> // For logging

namespace dashaibrowser {
namespace asol {

AsolGatewayServer::AsolGatewayServer() {
    std::cout << "AsolGatewayServer: Instance created." << std::endl;
}

AsolGatewayServer::~AsolGatewayServer() {
    Shutdown(); // Ensure server is stopped on destruction
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
    std::cout << "AsolGatewayServer: Instance destroyed." << std::endl;
}

void AsolGatewayServer::Run(const std::string& address) {
    if (running_) {
        std::cout << "AsolGatewayServer::Run: Server is already running." << std::endl;
        return;
    }

    ::grpc::ServerBuilder builder;
    // Listen on the given address without any authentication mechanism for now.
    builder.AddListeningPort(address, ::grpc::InsecureServerCredentials());
    // Register "service_impl_" as the instance through which we'll communicate with
    // clients. In this case, it corresponds to an *synchronous* service.
    builder.RegisterService(&service_impl_);

    // Finally assemble the server.
    server_ = builder.BuildAndStart();
    if (!server_) {
        std::cerr << "AsolGatewayServer::Run: Failed to start server on " << address << std::endl;
        return; // TODO: Proper error handling/propagation
    }
    std::cout << "AsolGatewayServer::Run: Server listening on " << address << std::endl;
    running_ = true;

    // Detach the server thread to allow Run to return if needed, or join it.
    // For a simple main, server_->Wait() is blocking.
    // If we want Run to be non-blocking and manage lifetime elsewhere,
    // we'd run Wait in a separate thread.
    // For now, let's make Run blocking as it's simpler for a main() function.
    // server_thread_ = std::thread([this]() {
    //    server_->Wait(); // This blocks until the server is shutdown.
    // });
    // To make this Run method itself blocking:
    server_->Wait();


    // server_->Wait() will block until Shutdown() is called from another thread,
    // or if the server is shut down for other reasons.
    running_ = false;
    std::cout << "AsolGatewayServer::Run: Server has shut down." << std::endl;
}

void AsolGatewayServer::Shutdown() {
    if (server_ && running_) {
        std::cout << "AsolGatewayServer::Shutdown: Attempting to shut down server..." << std::endl;
        server_->Shutdown(); // This is a non-blocking call to request shutdown.
                             // The Wait() call in Run() will then unblock.
        running_ = false; // Mark as not running, though Wait() exiting is the true indicator.
        std::cout << "AsolGatewayServer::Shutdown: Server shutdown initiated." << std::endl;
    } else {
        std::cout << "AsolGatewayServer::Shutdown: Server not running or already shut down." << std::endl;
    }
}

}  // namespace asol
}  // namespace dashaibrowser
