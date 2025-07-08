// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASOL_ADAPTERS_GEMINI_GEMINI_TEXT_ADAPTER_H_
#define ASOL_ADAPTERS_GEMINI_GEMINI_TEXT_ADAPTER_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "third_party/nlohmann_json/json.hpp"

namespace asol {
namespace adapters {
namespace gemini {

// Represents a message in a conversation with Gemini
struct GeminiMessage {
  enum class Role {
    USER,
    MODEL,
    SYSTEM
  };

  Role role;
  std::string content;
};

// Represents configuration options for Gemini API requests
struct GeminiRequestConfig {
  std::string model_name = "gemini-pro";  // Default model
  float temperature = 0.7f;               // Controls randomness (0.0 to 1.0)
  int max_output_tokens = 1024;           // Maximum number of tokens in response
  float top_p = 0.95f;                    // Nucleus sampling parameter
  float top_k = 40.0f;                    // Top-k sampling parameter
};

// Callback for handling Gemini API responses
using GeminiResponseCallback = 
    base::OnceCallback<void(bool success, const std::string& response)>;

class GeminiTextAdapter {
 public:
  GeminiTextAdapter();
  explicit GeminiTextAdapter(const std::string& api_key);
  ~GeminiTextAdapter();

  // Disallow copy and assign
  GeminiTextAdapter(const GeminiTextAdapter&) = delete;
  GeminiTextAdapter& operator=(const GeminiTextAdapter&) = delete;

  // Process a single text prompt and get a response
  void ProcessText(const std::string& text_input, GeminiResponseCallback callback);
  
  // Process a conversation with multiple messages
  void ProcessConversation(const std::vector<GeminiMessage>& messages,
                          GeminiResponseCallback callback);
  
  // Configure the adapter with specific settings
  void SetRequestConfig(const GeminiRequestConfig& config);
  
  // Get the current configuration
  const GeminiRequestConfig& GetRequestConfig() const;
  
  // Set API key (for runtime configuration)
  void SetApiKey(const std::string& api_key);

 private:
  // Helper methods for API interaction
  nlohmann::json BuildRequestPayload(const std::string& text_input) const;
  nlohmann::json BuildConversationPayload(
      const std::vector<GeminiMessage>& messages) const;
  
  // Convert message role enum to string for API
  std::string RoleToString(GeminiMessage::Role role) const;
  
  // Send request to Gemini API
  void SendRequest(const nlohmann::json& payload, GeminiResponseCallback callback);
  
  // Parse API response
  void HandleResponse(const std::string& response_data, 
                     GeminiResponseCallback callback);

  // Private members
  std::string api_key_;
  GeminiRequestConfig config_;
  
  // For async operations and callbacks
  base::WeakPtrFactory<GeminiTextAdapter> weak_ptr_factory_{this};
};

}  // namespace gemini
}  // namespace adapters
}  // namespace asol

#endif  // ASOL_ADAPTERS_GEMINI_GEMINI_TEXT_ADAPTER_H_
