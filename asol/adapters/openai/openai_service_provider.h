// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASOL_ADAPTERS_OPENAI_OPENAI_SERVICE_PROVIDER_H_
#define ASOL_ADAPTERS_OPENAI_OPENAI_SERVICE_PROVIDER_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "asol/adapters/openai/openai_text_adapter.h"
#include "asol/core/ai_service_provider.h"
#include "base/memory/weak_ptr.h"

namespace asol {
namespace adapters {
namespace openai {

// OpenAIServiceProvider implements the AIServiceProvider interface for OpenAI.
class OpenAIServiceProvider : public core::AIServiceProvider {
 public:
  OpenAIServiceProvider();
  explicit OpenAIServiceProvider(const std::string& api_key);
  ~OpenAIServiceProvider() override;

  // AIServiceProvider implementation
  std::string GetProviderId() const override;
  std::string GetProviderName() const override;
  std::string GetProviderVersion() const override;
  Capabilities GetCapabilities() const override;
  bool SupportsTaskType(TaskType task_type) const override;
  void ProcessRequest(const AIRequestParams& params, 
                    AIResponseCallback callback) override;
  void Configure(const std::unordered_map<std::string, std::string>& config) override;
  std::unordered_map<std::string, std::string> GetConfiguration() const override;

 private:
  // Helper methods for processing different task types
  void ProcessTextGeneration(const AIRequestParams& params, 
                           AIResponseCallback callback);
  void ProcessTextSummarization(const AIRequestParams& params, 
                              AIResponseCallback callback);
  void ProcessContentAnalysis(const AIRequestParams& params, 
                            AIResponseCallback callback);
  void ProcessQuestionAnswering(const AIRequestParams& params, 
                              AIResponseCallback callback);
  void ProcessCodeGeneration(const AIRequestParams& params, 
                           AIResponseCallback callback);
  void ProcessTranslation(const AIRequestParams& params, 
                        AIResponseCallback callback);

  // Convert between ASOL and OpenAI message formats
  std::vector<OpenAIMessage> ConvertToOpenAIMessages(
      const std::vector<core::ContextMessage>& context_messages);
  
  // Handle OpenAI adapter response
  void OnOpenAIResponse(AIResponseCallback callback,
                      bool success,
                      const std::string& response);

  // The OpenAI adapter
  std::unique_ptr<OpenAITextAdapter> openai_adapter_;
  
  // Configuration
  std::unordered_map<std::string, std::string> config_;
  
  // For weak pointers
  base::WeakPtrFactory<OpenAIServiceProvider> weak_ptr_factory_{this};
};

}  // namespace openai
}  // namespace adapters
}  // namespace asol

#endif  // ASOL_ADAPTERS_OPENAI_OPENAI_SERVICE_PROVIDER_H_