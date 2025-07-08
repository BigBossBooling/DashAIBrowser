// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASOL_ADAPTERS_OPENAI_OPENAI_TEXT_ADAPTER_H_
#define ASOL_ADAPTERS_OPENAI_OPENAI_TEXT_ADAPTER_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "third_party/nlohmann_json/json.hpp"

namespace asol {
namespace adapters {
namespace openai {

// Represents a message in a conversation with OpenAI
struct OpenAIMessage {
  enum class Role {
    USER,
    ASSISTANT,
    SYSTEM
  };

  Role role;
  std::string content;
};

// Represents configuration options for OpenAI API requests
struct OpenAIRequestConfig {
  std::string model_name = "gpt-4o";  // Default model
  float temperature = 0.7f;           // Controls randomness (0.0 to 1.0)
  int max_tokens = 1024;              // Maximum number of tokens in response
  float top_p = 0.95f;                // Nucleus sampling parameter
  float frequency_penalty = 0.0f;     // Penalize frequent tokens
  float presence_penalty = 0.0f;      // Penalize new tokens based on presence in text
  std::string organization_id = "";   // Optional organization ID
};

// Callback for handling OpenAI API responses
using OpenAIResponseCallback = 
    base::OnceCallback<void(bool success, const std::string& response)>;

class OpenAITextAdapter {
 public:
  OpenAITextAdapter();
  explicit OpenAITextAdapter(const std::string& api_key);
  ~OpenAITextAdapter();

  // Disallow copy and assign
  OpenAITextAdapter(const OpenAITextAdapter&) = delete;
  OpenAITextAdapter& operator=(const OpenAITextAdapter&) = delete;

  // Process a single text prompt and get a response
  void ProcessText(const std::string& text_input, OpenAIResponseCallback callback);
  
  // Process a conversation with multiple messages
  void ProcessConversation(const std::vector<OpenAIMessage>& messages,
                          OpenAIResponseCallback callback);
  
  // Configure the adapter with specific settings
  void SetRequestConfig(const OpenAIRequestConfig& config);
  
  // Get the current configuration
  const OpenAIRequestConfig& GetRequestConfig() const;
  
  // Set API key (for runtime configuration)
  void SetApiKey(const std::string& api_key);

  // Set organization ID (for runtime configuration)
  void SetOrganizationId(const std::string& org_id);

 private:
  // Helper methods for API interaction
  nlohmann::json BuildRequestPayload(const std::string& text_input) const;
  nlohmann::json BuildConversationPayload(
      const std::vector<OpenAIMessage>& messages) const;
  
  // Convert message role enum to string for API
  std::string RoleToString(OpenAIMessage::Role role) const;
  
  // Send request to OpenAI API
  void SendRequest(const nlohmann::json& payload, OpenAIResponseCallback callback);
  
  // Parse API response
  void HandleResponse(const std::string& response_data, 
                     OpenAIResponseCallback callback);

  // Private members
  std::string api_key_;
  OpenAIRequestConfig config_;
  
  // For async operations and callbacks
  base::WeakPtrFactory<OpenAITextAdapter> weak_ptr_factory_{this};
};

}  // namespace openai
}  // namespace adapters
}  // namespace asol

#endif  // ASOL_ADAPTERS_OPENAI_OPENAI_TEXT_ADAPTER_H_