#ifndef DASHAI_BROWSER_ASOL_CPP_ASOL_GATEWAY_SERVER_H_
#define DASHAI_BROWSER_ASOL_CPP_ASOL_GATEWAY_SERVER_H_

#include "asol/cpp/asol_service_impl.h" // The service implementation
#include <grpcpp/grpcpp.h>
#include <memory> // For std::unique_ptr
#include <string>
#include <thread> // For std::thread

namespace dashaibrowser {
namespace asol {

class AsolGatewayServer {
 public:
  AsolGatewayServer();
  ~AsolGatewayServer();

  // Starts the gRPC server and blocks until it's shut down.
  // address: The address to listen on (e.g., "0.0.0.0:50051").
  void Run(const std::string& address);

  // Stops the gRPC server.
  void Shutdown();

 private:
  AsolServiceImpl service_impl_; // Instance of our service implementation
  std::unique_ptr<::grpc::Server> server_; // The gRPC server instance
  std::thread server_thread_; // Thread for the server's blocking run loop
  bool running_ = false;
};

}  // namespace asol
}  // namespace dashaibrowser

#endif  // DASHAI_BROWSER_ASOL_CPP_ASOL_GATEWAY_SERVER_H_
