// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASOL_ADAPTERS_ADAPTER_INTERFACE_H_
#define ASOL_ADAPTERS_ADAPTER_INTERFACE_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace asol {
namespace adapters {

// Common response structure for all AI model adapters
struct ModelResponse {
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

// Callback for asynchronous responses
using ResponseCallback = std::function<void(const ModelResponse&)>;

// Callback for streaming responses
using StreamingResponseCallback = std::function<void(const ModelResponse&, bool is_done)>;

// Base interface for all AI model adapters
class AdapterInterface {
 public:
  virtual ~AdapterInterface() = default;

  // Process text synchronously
  virtual ModelResponse ProcessText(const std::string& text_input) = 0;
  
  // Process text asynchronously
  virtual void ProcessTextAsync(const std::string& text_input,
                               ResponseCallback callback) = 0;
  
  // Process text with streaming response
  virtual void ProcessTextStream(const std::string& text_input,
                                StreamingResponseCallback callback) = 0;
  
  // Get the name of this adapter
  virtual std::string GetName() const = 0;
  
  // Get the capabilities of this adapter
  virtual std::vector<std::string> GetCapabilities() const = 0;
  
  // Check if the adapter is ready to process requests
  virtual bool IsReady() const = 0;
  
  // Initialize the adapter with configuration
  virtual bool Initialize(const std::string& config_json) = 0;
  
  // Check if the adapter supports streaming
  virtual bool SupportsStreaming() const = 0;
};

// Factory function to create adapters by type
std::unique_ptr<AdapterInterface> CreateAdapter(const std::string& adapter_type);

}  // namespace adapters
}  // namespace asol

#endif  // ASOL_ADAPTERS_ADAPTER_INTERFACE_H_