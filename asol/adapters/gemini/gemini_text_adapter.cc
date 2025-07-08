// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "asol/adapters/gemini/gemini_text_adapter.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "third_party/nlohmann_json/json.hpp"

namespace asol {
namespace adapters {
namespace gemini {

namespace {
// Constants for Gemini API
constexpr char kGeminiApiEndpoint[] = "https://generativelanguage.googleapis.com/v1beta/models/";
constexpr char kGenerateContentMethod[] = ":generateContent";
constexpr char kApiKeyParam[] = "key=";

// Helper function to truncate text for logging
std::string TruncateForLogging(const std::string& text, size_t max_length = 50) {
  if (text.length() <= max_length)
    return text;
  return text.substr(0, max_length) + "...";
}

}  // namespace

GeminiTextAdapter::GeminiTextAdapter() : config_() {
  LOG(INFO) << "GeminiTextAdapter initialized with default configuration.";
}

GeminiTextAdapter::GeminiTextAdapter(const std::string& api_key) 
    : api_key_(api_key), config_() {
  LOG(INFO) << "GeminiTextAdapter initialized with provided API key.";
}

GeminiTextAdapter::~GeminiTextAdapter() {
  LOG(INFO) << "GeminiTextAdapter destroyed.";
}

void GeminiTextAdapter::ProcessText(
    const std::string& text_input,
    GeminiResponseCallback callback) {
  DLOG(INFO) << "Processing text with Gemini Adapter: " 
             << TruncateForLogging(text_input);
  
  // Build the request payload for a single text prompt
  nlohmann::json payload = BuildRequestPayload(text_input);
  
  // Send the request to the Gemini API
  SendRequest(payload, std::move(callback));
}

void GeminiTextAdapter::ProcessConversation(
    const std::vector<GeminiMessage>& messages,
    GeminiResponseCallback callback) {
  DLOG(INFO) << "Processing conversation with " << messages.size() << " messages";
  
  // Build the request payload for a conversation
  nlohmann::json payload = BuildConversationPayload(messages);
  
  // Send the request to the Gemini API
  SendRequest(payload, std::move(callback));
}

void GeminiTextAdapter::SetRequestConfig(const GeminiRequestConfig& config) {
  config_ = config;
  DLOG(INFO) << "Updated Gemini request configuration. Model: " 
             << config_.model_name;
}

const GeminiRequestConfig& GeminiTextAdapter::GetRequestConfig() const {
  return config_;
}

void GeminiTextAdapter::SetApiKey(const std::string& api_key) {
  api_key_ = api_key;
  DLOG(INFO) << "Updated API key.";
}

nlohmann::json GeminiTextAdapter::BuildRequestPayload(
    const std::string& text_input) const {
  nlohmann::json payload;
  
  // Add the content parts
  nlohmann::json content;
  nlohmann::json text_part;
  text_part["text"] = text_input;
  
  nlohmann::json parts = nlohmann::json::array();
  parts.push_back(text_part);
  
  content["parts"] = parts;
  content["role"] = "user";
  
  // Add the content to the main payload
  payload["contents"] = nlohmann::json::array({content});
  
  // Add generation configuration
  nlohmann::json generation_config;
  generation_config["temperature"] = config_.temperature;
  generation_config["maxOutputTokens"] = config_.max_output_tokens;
  generation_config["topP"] = config_.top_p;
  generation_config["topK"] = config_.top_k;
  
  payload["generationConfig"] = generation_config;
  
  return payload;
}

nlohmann::json GeminiTextAdapter::BuildConversationPayload(
    const std::vector<GeminiMessage>& messages) const {
  nlohmann::json payload;
  nlohmann::json contents = nlohmann::json::array();
  
  // Convert each message to the format expected by Gemini API
  for (const auto& message : messages) {
    nlohmann::json content;
    content["role"] = RoleToString(message.role);
    
    nlohmann::json parts = nlohmann::json::array();
    nlohmann::json text_part;
    text_part["text"] = message.content;
    parts.push_back(text_part);
    
    content["parts"] = parts;
    contents.push_back(content);
  }
  
  payload["contents"] = contents;
  
  // Add generation configuration
  nlohmann::json generation_config;
  generation_config["temperature"] = config_.temperature;
  generation_config["maxOutputTokens"] = config_.max_output_tokens;
  generation_config["topP"] = config_.top_p;
  generation_config["topK"] = config_.top_k;
  
  payload["generationConfig"] = generation_config;
  
  return payload;
}

std::string GeminiTextAdapter::RoleToString(GeminiMessage::Role role) const {
  switch (role) {
    case GeminiMessage::Role::USER:
      return "user";
    case GeminiMessage::Role::MODEL:
      return "model";
    case GeminiMessage::Role::SYSTEM:
      return "system";
    default:
      LOG(ERROR) << "Unknown role type, defaulting to 'user'";
      return "user";
  }
}

void GeminiTextAdapter::SendRequest(
    const nlohmann::json& payload,
    GeminiResponseCallback callback) {
  // In a real implementation, this would use network services to send an HTTP request
  // For now, we'll simulate a response
  
  std::string json_str = payload.dump(2);
  DLOG(INFO) << "Gemini API Request payload: " << TruncateForLogging(json_str, 100);
  
  // Construct the API URL (would be used in actual implementation)
  std::string api_url = std::string(kGeminiApiEndpoint) + 
                        config_.model_name + 
                        kGenerateContentMethod + 
                        "?" + kApiKeyParam + api_key_;
  
  DLOG(INFO) << "Would send request to: " << api_url;
  
  // Simulate async processing
  base::ThreadPool::PostDelayedTask(
      FROM_HERE,
      {base::TaskPriority::BEST_EFFORT},
      base::BindOnce(&GeminiTextAdapter::HandleResponse,
                    weak_ptr_factory_.GetWeakPtr(),
                    // Simulate a response - in real implementation this would be the API response
                    R"({
                      "candidates": [{
                        "content": {
                          "parts": [{
                            "text": "This is a simulated response from the Gemini API."
                          }],
                          "role": "model"
                        },
                        "finishReason": "STOP",
                        "index": 0
                      }]
                    })",
                    std::move(callback)),
      base::Milliseconds(100));  // Simulate network delay
}

void GeminiTextAdapter::HandleResponse(
    const std::string& response_data,
    GeminiResponseCallback callback) {
  DLOG(INFO) << "Handling Gemini API response: " 
             << TruncateForLogging(response_data, 100);
  
  bool success = true;
  std::string result_text;
  
  try {
    // Parse the JSON response
    nlohmann::json response = nlohmann::json::parse(response_data);
    
    // Extract the response text
    if (response.contains("candidates") && 
        !response["candidates"].empty() && 
        response["candidates"][0].contains("content") &&
        response["candidates"][0]["content"].contains("parts") &&
        !response["candidates"][0]["content"]["parts"].empty() &&
        response["candidates"][0]["content"]["parts"][0].contains("text")) {
      
      result_text = response["candidates"][0]["content"]["parts"][0]["text"];
    } else {
      success = false;
      result_text = "Failed to parse response: unexpected format";
      LOG(ERROR) << result_text;
    }
  } catch (const std::exception& e) {
    success = false;
    result_text = "Failed to parse response: " + std::string(e.what());
    LOG(ERROR) << result_text;
  }
  
  // Invoke the callback with the result
  std::move(callback).Run(success, result_text);
}

}  // namespace gemini
}  // namespace adapters
}  // namespace asol
