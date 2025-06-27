#ifndef DASHAI_BROWSER_ASOL_CPP_ASOL_SERVICE_IMPL_H_
#define DASHAI_BROWSER_ASOL_CPP_ASOL_SERVICE_IMPL_H_

#include "proto/asol_service.grpc.pb.h"
#include "asol/adapters/gemini/gemini_text_adapter.h" // Include Gemini adapter
#include <grpcpp/grpcpp.h>
#include <memory> // For std::unique_ptr

namespace dashaibrowser {
namespace asol {

class AsolServiceImpl final : public ipc::AsolInterface::Service {
 public:
  // Constructor can take dependencies like AI adapters.
  AsolServiceImpl();
  ~AsolServiceImpl() override;

  ::grpc::Status GetSummary(
      ::grpc::ServerContext* context,
      const ipc::SummaryRequest* request,
      ipc::SummaryResponse* response) override;

  ::grpc::Status TranslateText(
      ::grpc::ServerContext* context,
      const ipc::TranslationRequest* request,
      ipc::TranslationResponse* response) override;

 private:
  void SetError(ipc::ErrorDetails* error_details,
                int32_t code,
                const std::string& message,
                const std::string& user_message = "");

  // Member to hold the AI adapter instance(s).
  // Using the interface type for flexibility.
  std::unique_ptr<adapters::IGeminiTextAdapter> gemini_adapter_;
  // In a more complex scenario, this might be a map or a factory
  // to select adapters based on request parameters or configuration.
  // std::map<std::string, std::unique_ptr<adapters::IAiAdapter>> ai_adapters_;

  bool InitializeAdapters(); // Helper to initialize adapters
  bool adapters_initialized_ = false;
};

}  // namespace asol
}  // namespace dashaibrowser

#endif  // DASHAI_BROWSER_ASOL_CPP_ASOL_SERVICE_IMPL_H_
