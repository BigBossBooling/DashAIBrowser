// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASOL_ADAPTERS_COPILOT_COPILOT_SERVICE_PROVIDER_H_
#define ASOL_ADAPTERS_COPILOT_COPILOT_SERVICE_PROVIDER_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "asol/adapters/copilot/copilot_text_adapter.h"
#include "asol/core/ai_service_provider.h"
#include "base/memory/weak_ptr.h"

namespace asol {
namespace adapters {
namespace copilot {

// CopilotServiceProvider implements the AIServiceProvider interface for Microsoft Copilot.
class CopilotServiceProvider : public core::AIServiceProvider {
 public:
  CopilotServiceProvider();
  explicit CopilotServiceProvider(const std::string& api_key);
  ~CopilotServiceProvider() override;

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

  // Convert between ASOL and Copilot message formats
  std::vector<CopilotMessage> ConvertToCopilotMessages(
      const std::vector<core::ContextMessage>& context_messages);
  
  // Handle Copilot adapter response
  void OnCopilotResponse(AIResponseCallback callback,
                      bool success,
                      const std::string& response);

  // The Copilot adapter
  std::unique_ptr<CopilotTextAdapter> copilot_adapter_;
  
  // Configuration
  std::unordered_map<std::string, std::string> config_;
  
  // For weak pointers
  base::WeakPtrFactory<CopilotServiceProvider> weak_ptr_factory_{this};
};

}  // namespace copilot
}  // namespace adapters
}  // namespace asol

#endif  // ASOL_ADAPTERS_COPILOT_COPILOT_SERVICE_PROVIDER_H_