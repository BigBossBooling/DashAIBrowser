// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASOL_ADAPTERS_COPILOT_COPILOT_TEXT_ADAPTER_H_
#define ASOL_ADAPTERS_COPILOT_COPILOT_TEXT_ADAPTER_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "third_party/nlohmann_json/json.hpp"

namespace asol {
namespace adapters {
namespace copilot {

// Represents a message in a conversation with Microsoft Copilot
struct CopilotMessage {
  enum class Role {
    USER,
    ASSISTANT,
    SYSTEM
  };

  Role role;
  std::string content;
};

// Represents configuration options for Microsoft Copilot API requests
struct CopilotRequestConfig {
  std::string model_name = "copilot-4";  // Default model
  float temperature = 0.7f;              // Controls randomness (0.0 to 1.0)
  int max_tokens = 1024;                 // Maximum number of tokens in response
  float top_p = 0.95f;                   // Nucleus sampling parameter
  std::string api_version = "2023-12-01-preview"; // API version
};

// Callback for handling Microsoft Copilot API responses
using CopilotResponseCallback = 
    base::OnceCallback<void(bool success, const std::string& response)>;

class CopilotTextAdapter {
 public:
  CopilotTextAdapter();
  explicit CopilotTextAdapter(const std::string& api_key);
  ~CopilotTextAdapter();

  // Disallow copy and assign
  CopilotTextAdapter(const CopilotTextAdapter&) = delete;
  CopilotTextAdapter& operator=(const CopilotTextAdapter&) = delete;

  // Process a single text prompt and get a response
  void ProcessText(const std::string& text_input, CopilotResponseCallback callback);
  
  // Process a conversation with multiple messages
  void ProcessConversation(const std::vector<CopilotMessage>& messages,
                          CopilotResponseCallback callback);
  
  // Configure the adapter with specific settings
  void SetRequestConfig(const CopilotRequestConfig& config);
  
  // Get the current configuration
  const CopilotRequestConfig& GetRequestConfig() const;
  
  // Set API key (for runtime configuration)
  void SetApiKey(const std::string& api_key);

  // Set endpoint (for runtime configuration)
  void SetEndpoint(const std::string& endpoint);

 private:
  // Helper methods for API interaction
  nlohmann::json BuildRequestPayload(const std::string& text_input) const;
  nlohmann::json BuildConversationPayload(
      const std::vector<CopilotMessage>& messages) const;
  
  // Convert message role enum to string for API
  std::string RoleToString(CopilotMessage::Role role) const;
  
  // Send request to Microsoft Copilot API
  void SendRequest(const nlohmann::json& payload, CopilotResponseCallback callback);
  
  // Parse API response
  void HandleResponse(const std::string& response_data, 
                     CopilotResponseCallback callback);

  // Private members
  std::string api_key_;
  std::string endpoint_ = "https://api.cognitive.microsoft.com/copilot/v1/chat/completions";
  CopilotRequestConfig config_;
  
  // For async operations and callbacks
  base::WeakPtrFactory<CopilotTextAdapter> weak_ptr_factory_{this};
};

}  // namespace copilot
}  // namespace adapters
}  // namespace asol

#endif  // ASOL_ADAPTERS_COPILOT_COPILOT_TEXT_ADAPTER_H_