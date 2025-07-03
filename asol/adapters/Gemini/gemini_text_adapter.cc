// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "asol/adapters/gemini/gemini_text_adapter.h"

#include <thread>

#include "base/logging.h"
#include "base/strings/string_util.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "third_party/nlohmann_json/json.hpp"

namespace asol {
namespace adapters {
namespace gemini {

GeminiTextAdapter::GeminiTextAdapter() 
    : GeminiTextAdapter(nullptr) {
}

GeminiTextAdapter::GeminiTextAdapter(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  DLOG(INFO) << "GeminiTextAdapter created.";
  
  // Create the HTTP client
  // In a real implementation, we would use the browser's URL loader factory
  // For now, we'll create a mock client that simulates responses
  http_client_ = std::make_unique<GeminiHttpClient>(url_loader_factory);
}

GeminiTextAdapter::~GeminiTextAdapter() {
  DLOG(INFO) << "GeminiTextAdapter destroyed.";
}

bool GeminiTextAdapter::Initialize(const GeminiConfig& config) {
  // Validate configuration
  if (config.api_key.empty()) {
    LOG(ERROR) << "API key is required for Gemini adapter initialization";
    return false;
  }
  
  config_ = config;
  
  // Configure the HTTP client
  http_client_->SetApiKey(config_.api_key);
  http_client_->SetApiEndpoint(config_.api_endpoint);
  
  is_initialized_ = true;
  
  LOG(INFO) << "GeminiTextAdapter initialized with model: " << config_.model_name;
  return true;
}

bool GeminiTextAdapter::Initialize(const std::string& config_json) {
  try {
    auto json_config = nlohmann::json::parse(config_json);
    
    GeminiConfig config;
    
    // Extract required configuration
    if (json_config.contains("api_key")) {
      config.api_key = json_config["api_key"];
    } else {
      LOG(ERROR) << "Missing required 'api_key' in configuration";
      return false;
    }
    
    // Extract optional configurations
    if (json_config.contains("model_name")) {
      config.model_name = json_config["model_name"];
    }
    
    if (json_config.contains("temperature")) {
      config.temperature = json_config["temperature"];
    }
    
    if (json_config.contains("max_output_tokens")) {
      config.max_output_tokens = json_config["max_output_tokens"];
    }
    
    if (json_config.contains("api_endpoint")) {
      config.api_endpoint = json_config["api_endpoint"];
    }
    
    return Initialize(config);
  } catch (const std::exception& e) {
    LOG(ERROR) << "Failed to parse configuration: " << e.what();
    return false;
  }
}

// AdapterInterface implementation

ModelResponse GeminiTextAdapter::ProcessText(const std::string& text_input) {
  DLOG(INFO) << "Processing text with Gemini Adapter: " << text_input.substr(0, 50) << "...";

  if (!IsReady()) {
    ModelResponse response;
    response.success = false;
    response.error_message = "Adapter not initialized. Call Initialize() first.";
    return response;
  }

  // Build request payload according to Gemini API format
  nlohmann::json request_payload = BuildRequestPayload(text_input);
  
  // Send request to Gemini API and convert the response
  GeminiResponse gemini_response = http_client_->SendRequest(request_payload, config_.model_name);
  return ConvertResponse(gemini_response);
}

void GeminiTextAdapter::ProcessTextAsync(const std::string& text_input, ResponseCallback callback) {
  if (!IsReady()) {
    ModelResponse response;
    response.success = false;
    response.error_message = "Adapter not initialized. Call Initialize() first.";
    callback(response);
    return;
  }

  // Build request payload
  nlohmann::json request_payload = BuildRequestPayload(text_input);
  
  // Send the request asynchronously
  http_client_->SendRequestAsync(
      request_payload, 
      config_.model_name,
      base::BindOnce([](ResponseCallback user_callback, const GeminiResponse& gemini_response,
                      GeminiTextAdapter* adapter) {
        ModelResponse response = adapter->ConvertResponse(gemini_response);
        user_callback(response);
      }, std::move(callback), base::Unretained(this)));
}

void GeminiTextAdapter::ProcessTextStream(
    const std::string& text_input, 
    StreamingResponseCallback callback) {
  if (!IsReady()) {
    ModelResponse response;
    response.success = false;
    response.error_message = "Adapter not initialized. Call Initialize() first.";
    callback(response, true);  // true indicates this is the final response
    return;
  }

  // Build request payload
  nlohmann::json request_payload = BuildRequestPayload(text_input);
  
  // Send the streaming request
  http_client_->SendStreamingRequest(
      request_payload, 
      config_.model_name,
      base::BindRepeating([](StreamingResponseCallback user_callback, 
                           GeminiTextAdapter* adapter,
                           const GeminiResponse& gemini_response,
                           bool is_done) {
        ModelResponse response = adapter->ConvertResponse(gemini_response);
        response.is_partial = gemini_response.is_partial;
        user_callback(response, is_done);
      }, std::move(callback), base::Unretained(this)));
}

std::string GeminiTextAdapter::GetName() const {
  return "Gemini";
}

std::vector<std::string> GeminiTextAdapter::GetCapabilities() const {
  return {"text-generation", "summarization", "question-answering"};
}

bool GeminiTextAdapter::IsReady() const {
  return is_initialized_ && !config_.api_key.empty();
}

bool GeminiTextAdapter::SupportsStreaming() const {
  return true;
}

std::string GeminiTextAdapter::GetModelName() const {
  return config_.model_name;
}

// Private methods

nlohmann::json GeminiTextAdapter::BuildRequestPayload(const std::string& text_input) const {
  nlohmann::json request;
  
  // Build contents array with user message
  nlohmann::json contents = nlohmann::json::array();
  nlohmann::json content;
  content["role"] = "user";
  
  nlohmann::json parts = nlohmann::json::array();
  nlohmann::json part;
  part["text"] = text_input;
  parts.push_back(part);
  
  content["parts"] = parts;
  contents.push_back(content);
  
  request["contents"] = contents;
  
  // Add generation config
  nlohmann::json generation_config;
  generation_config["temperature"] = config_.temperature;
  generation_config["maxOutputTokens"] = config_.max_output_tokens;
  
  request["generationConfig"] = generation_config;
  
  return request;
}

ModelResponse GeminiTextAdapter::ConvertResponse(const GeminiResponse& gemini_response) const {
  ModelResponse response;
  
  // Copy basic fields
  response.text = gemini_response.text;
  response.success = gemini_response.success;
  response.error_message = gemini_response.error_message;
  
  // Copy metadata
  response.metadata = gemini_response.metadata;
  
  // Add adapter-specific metadata if not already present
  bool has_adapter_info = false;
  for (const auto& meta : response.metadata) {
    if (meta.first == "adapter") {
      has_adapter_info = true;
      break;
    }
  }
  
  if (!has_adapter_info) {
    response.metadata.push_back({"adapter", "Gemini"});
  }
  
  return response;
}

}  // namespace gemini
}  // namespace adapters
}  // namespace asol
