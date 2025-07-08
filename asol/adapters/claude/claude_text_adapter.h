// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASOL_ADAPTERS_CLAUDE_CLAUDE_TEXT_ADAPTER_H_
#define ASOL_ADAPTERS_CLAUDE_CLAUDE_TEXT_ADAPTER_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "third_party/nlohmann_json/json.hpp"

namespace asol {
namespace adapters {
namespace claude {

// Represents a message in a conversation with Claude
struct ClaudeMessage {
  enum class Role {
    USER,
    ASSISTANT,
    SYSTEM
  };

  Role role;
  std::string content;
};

// Represents configuration options for Claude API requests
struct ClaudeRequestConfig {
  std::string model_name = "claude-3-opus-20240229";  // Default model
  float temperature = 0.7f;                           // Controls randomness (0.0 to 1.0)
  int max_tokens = 1024;                              // Maximum number of tokens in response
  float top_p = 0.95f;                                // Nucleus sampling parameter
  float top_k = 40.0f;                                // Top-k sampling parameter
  std::string anthropic_version = "2023-06-01";       // API version
};

// Callback for handling Claude API responses
using ClaudeResponseCallback = 
    base::OnceCallback<void(bool success, const std::string& response)>;

class ClaudeTextAdapter {
 public:
  ClaudeTextAdapter();
  explicit ClaudeTextAdapter(const std::string& api_key);
  ~ClaudeTextAdapter();

  // Disallow copy and assign
  ClaudeTextAdapter(const ClaudeTextAdapter&) = delete;
  ClaudeTextAdapter& operator=(const ClaudeTextAdapter&) = delete;

  // Process a single text prompt and get a response
  void ProcessText(const std::string& text_input, ClaudeResponseCallback callback);
  
  // Process a conversation with multiple messages
  void ProcessConversation(const std::vector<ClaudeMessage>& messages,
                          ClaudeResponseCallback callback);
  
  // Configure the adapter with specific settings
  void SetRequestConfig(const ClaudeRequestConfig& config);
  
  // Get the current configuration
  const ClaudeRequestConfig& GetRequestConfig() const;
  
  // Set API key (for runtime configuration)
  void SetApiKey(const std::string& api_key);

 private:
  // Helper methods for API interaction
  nlohmann::json BuildRequestPayload(const std::string& text_input) const;
  nlohmann::json BuildConversationPayload(
      const std::vector<ClaudeMessage>& messages) const;
  
  // Convert message role enum to string for API
  std::string RoleToString(ClaudeMessage::Role role) const;
  
  // Send request to Claude API
  void SendRequest(const nlohmann::json& payload, ClaudeResponseCallback callback);
  
  // Parse API response
  void HandleResponse(const std::string& response_data, 
                     ClaudeResponseCallback callback);

  // Private members
  std::string api_key_;
  ClaudeRequestConfig config_;
  
  // For async operations and callbacks
  base::WeakPtrFactory<ClaudeTextAdapter> weak_ptr_factory_{this};
};

}  // namespace claude
}  // namespace adapters
}  // namespace asol

#endif  // ASOL_ADAPTERS_CLAUDE_CLAUDE_TEXT_ADAPTER_H_