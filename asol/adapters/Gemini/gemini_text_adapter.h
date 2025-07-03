// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASOL_ADAPTERS_GEMINI_GEMINI_TEXT_ADAPTER_H_
#define ASOL_ADAPTERS_GEMINI_GEMINI_TEXT_ADAPTER_H_

#include <functional>
#include <memory>
#include <mutex>
#include <string>

#include "asol/adapters/adapter_interface.h"
#include "asol/adapters/gemini/gemini_http_client.h"
#include "asol/adapters/gemini/gemini_types.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "third_party/nlohmann_json/json.hpp"

namespace asol {
namespace adapters {
namespace gemini {

// GeminiTextAdapter provides an interface to the Google Gemini API
// for text processing and generation.
class GeminiTextAdapter : public AdapterInterface {
 public:
  // Constructor with default URL loader factory
  GeminiTextAdapter();
  
  // Constructor with custom URL loader factory for testing
  explicit GeminiTextAdapter(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
      
  ~GeminiTextAdapter() override;

  // AdapterInterface implementation
  ModelResponse ProcessText(const std::string& text_input) override;
  void ProcessTextAsync(const std::string& text_input, ResponseCallback callback) override;
  void ProcessTextStream(const std::string& text_input, StreamingResponseCallback callback) override;
  std::string GetName() const override;
  std::vector<std::string> GetCapabilities() const override;
  bool IsReady() const override;
  bool Initialize(const std::string& config_json) override;
  bool SupportsStreaming() const override;

  // Gemini-specific methods
  bool Initialize(const GeminiConfig& config);
  std::string GetModelName() const;

 private:
  // Helper methods for API interaction
  nlohmann::json BuildRequestPayload(const std::string& text_input) const;
  
  // Convert between response types
  ModelResponse ConvertResponse(const GeminiResponse& gemini_response) const;
  
  // Configuration
  GeminiConfig config_;
  
  // HTTP client for API communication
  std::unique_ptr<GeminiHttpClient> http_client_;
  
  // State tracking
  bool is_initialized_ = false;
  std::mutex request_mutex_;
};

}  // namespace gemini
}  // namespace adapters
}  // namespace asol

#endif  // ASOL_ADAPTERS_GEMINI_GEMINI_TEXT_ADAPTER_H_
