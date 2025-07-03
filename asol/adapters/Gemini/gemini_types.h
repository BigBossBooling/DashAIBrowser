// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASOL_ADAPTERS_GEMINI_GEMINI_TYPES_H_
#define ASOL_ADAPTERS_GEMINI_GEMINI_TYPES_H_

#include <string>
#include <vector>
#include <utility>

namespace asol {
namespace adapters {
namespace gemini {

// Response from the Gemini API
struct GeminiResponse {
  // The generated text from the model
  std::string text;
  
  // Whether the request was successful
  bool success = false;
  
  // Error message if unsuccessful
  std::string error_message;
  
  // Additional metadata as key-value pairs
  std::vector<std::pair<std::string, std::string>> metadata;
  
  // Whether this is a partial response (for streaming)
  bool is_partial = false;
};

// Callback for streaming responses
using StreamingResponseCallback = std::function<void(const GeminiResponse&, bool is_done)>;

// Configuration for Gemini API requests
struct GeminiConfig {
  // API key for authentication
  std::string api_key;
  
  // Model name (e.g., "gemini-pro")
  std::string model_name = "gemini-pro";
  
  // Temperature for text generation (0.0 to 1.0)
  double temperature = 0.7;
  
  // Maximum number of tokens to generate
  int max_output_tokens = 1024;
  
  // API endpoint URL
  std::string api_endpoint = "https://generativelanguage.googleapis.com/v1beta/models/";
};

}  // namespace gemini
}  // namespace adapters
}  // namespace asol

#endif  // ASOL_ADAPTERS_GEMINI_GEMINI_TYPES_H_