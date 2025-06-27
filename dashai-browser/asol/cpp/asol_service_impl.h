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

  ::grpc::Status ChatWithJules(
      ::grpc::ServerContext* context,
      const ipc::ConversationRequest* request,
      ipc::ConversationResponse* response) override;

 private:
  void SetError(ipc::ErrorDetails* error_details,
                int32_t code,
                const std::string& message,
                const std::string& user_message = "");

  std::unique_ptr<adapters::IGeminiTextAdapter> gemini_adapter_;

  bool InitializeAdapters();
  bool adapters_initialized_ = false;
};

}  // namespace asol
}  // namespace dashaibrowser

#endif  // DASHAI_BROWSER_ASOL_CPP_ASOL_SERVICE_IMPL_H_
