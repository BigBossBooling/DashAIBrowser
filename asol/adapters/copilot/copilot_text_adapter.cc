// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "asol/adapters/copilot/copilot_text_adapter.h"

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
namespace copilot {

namespace {
// Constants for Microsoft Copilot API
constexpr char kApiKeyHeader[] = "api-key: ";
constexpr char kContentTypeHeader[] = "Content-Type: application/json";

// Helper function to truncate text for logging
std::string TruncateForLogging(const std::string& text, size_t max_length = 50) {
  if (text.length() <= max_length)
    return text;
  return text.substr(0, max_length) + "...";
}

}  // namespace

CopilotTextAdapter::CopilotTextAdapter() : config_() {
  LOG(INFO) << "CopilotTextAdapter initialized with default configuration.";
}

CopilotTextAdapter::CopilotTextAdapter(const std::string& api_key) 
    : api_key_(api_key), config_() {
  LOG(INFO) << "CopilotTextAdapter initialized with provided API key.";
}

CopilotTextAdapter::~CopilotTextAdapter() {
  LOG(INFO) << "CopilotTextAdapter destroyed.";
}

void CopilotTextAdapter::ProcessText(
    const std::string& text_input,
    CopilotResponseCallback callback) {
  DLOG(INFO) << "Processing text with Copilot Adapter: " 
             << TruncateForLogging(text_input);
  
  // Build the request payload for a single text prompt
  nlohmann::json payload = BuildRequestPayload(text_input);
  
  // Send the request to the Microsoft Copilot API
  SendRequest(payload, std::move(callback));
}

void CopilotTextAdapter::ProcessConversation(
    const std::vector<CopilotMessage>& messages,
    CopilotResponseCallback callback) {
  DLOG(INFO) << "Processing conversation with " << messages.size() << " messages";
  
  // Build the request payload for a conversation
  nlohmann::json payload = BuildConversationPayload(messages);
  
  // Send the request to the Microsoft Copilot API
  SendRequest(payload, std::move(callback));
}

void CopilotTextAdapter::SetRequestConfig(const CopilotRequestConfig& config) {
  config_ = config;
  DLOG(INFO) << "Updated Copilot request configuration. Model: " 
             << config_.model_name;
}

const CopilotRequestConfig& CopilotTextAdapter::GetRequestConfig() const {
  return config_;
}

void CopilotTextAdapter::SetApiKey(const std::string& api_key) {
  api_key_ = api_key;
  DLOG(INFO) << "Updated API key.";
}

void CopilotTextAdapter::SetEndpoint(const std::string& endpoint) {
  endpoint_ = endpoint;
  DLOG(INFO) << "Updated endpoint to: " << endpoint_;
}

nlohmann::json CopilotTextAdapter::BuildRequestPayload(
    const std::string& text_input) const {
  nlohmann::json payload;
  
  // Set the model
  payload["model"] = config_.model_name;
  
  // Add the messages array with system and user messages
  nlohmann::json messages = nlohmann::json::array();
  
  // Add a default system message if processing a single text input
  nlohmann::json system_message;
  system_message["role"] = "system";
  system_message["content"] = "You are Microsoft Copilot, a helpful AI assistant.";
  messages.push_back(system_message);
  
  // Add the user message
  nlohmann::json user_message;
  user_message["role"] = "user";
  user_message["content"] = text_input;
  messages.push_back(user_message);
  
  payload["messages"] = messages;
  
  // Add generation parameters
  payload["temperature"] = config_.temperature;
  payload["max_tokens"] = config_.max_tokens;
  payload["top_p"] = config_.top_p;
  
  // Add API version
  payload["api-version"] = config_.api_version;
  
  return payload;
}

nlohmann::json CopilotTextAdapter::BuildConversationPayload(
    const std::vector<CopilotMessage>& messages) const {
  nlohmann::json payload;
  
  // Set the model
  payload["model"] = config_.model_name;
  
  // Convert each message to the format expected by Microsoft Copilot API
  nlohmann::json messages_array = nlohmann::json::array();
  for (const auto& message : messages) {
    nlohmann::json msg_obj;
    msg_obj["role"] = RoleToString(message.role);
    msg_obj["content"] = message.content;
    messages_array.push_back(msg_obj);
  }
  
  payload["messages"] = messages_array;
  
  // Add generation parameters
  payload["temperature"] = config_.temperature;
  payload["max_tokens"] = config_.max_tokens;
  payload["top_p"] = config_.top_p;
  
  // Add API version
  payload["api-version"] = config_.api_version;
  
  return payload;
}

std::string CopilotTextAdapter::RoleToString(CopilotMessage::Role role) const {
  switch (role) {
    case CopilotMessage::Role::USER:
      return "user";
    case CopilotMessage::Role::ASSISTANT:
      return "assistant";
    case CopilotMessage::Role::SYSTEM:
      return "system";
    default:
      LOG(ERROR) << "Unknown role type, defaulting to 'user'";
      return "user";
  }
}

void CopilotTextAdapter::SendRequest(
    const nlohmann::json& payload,
    CopilotResponseCallback callback) {
  // In a real implementation, this would use network services to send an HTTP request
  // For now, we'll simulate a response
  
  std::string json_str = payload.dump(2);
  DLOG(INFO) << "Copilot API Request payload: " << TruncateForLogging(json_str, 100);
  
  // Construct the headers (would be used in actual implementation)
  std::vector<std::string> headers;
  headers.push_back(std::string(kApiKeyHeader) + api_key_);
  headers.push_back(std::string(kContentTypeHeader));
  
  DLOG(INFO) << "Would send request to: " << endpoint_;
  
  // Simulate async processing
  base::ThreadPool::PostDelayedTask(
      FROM_HERE,
      {base::TaskPriority::BEST_EFFORT},
      base::BindOnce(&CopilotTextAdapter::HandleResponse,
                    weak_ptr_factory_.GetWeakPtr(),
                    // Simulate a response - in real implementation this would be the API response
                    R"({
                      "id": "copilot-chat-123",
                      "object": "chat.completion",
                      "created": 1677652288,
                      "model": "copilot-4",
                      "choices": [{
                        "index": 0,
                        "message": {
                          "role": "assistant",
                          "content": "This is a simulated response from the Microsoft Copilot API."
                        },
                        "finish_reason": "stop"
                      }],
                      "usage": {
                        "prompt_tokens": 9,
                        "completion_tokens": 12,
                        "total_tokens": 21
                      }
                    })",
                    std::move(callback)),
      base::Milliseconds(100));  // Simulate network delay
}

void CopilotTextAdapter::HandleResponse(
    const std::string& response_data,
    CopilotResponseCallback callback) {
  DLOG(INFO) << "Handling Copilot API response: " 
             << TruncateForLogging(response_data, 100);
  
  bool success = true;
  std::string result_text;
  
  try {
    // Parse the JSON response
    nlohmann::json response = nlohmann::json::parse(response_data);
    
    // Extract the response text
    if (response.contains("choices") && 
        !response["choices"].empty() && 
        response["choices"][0].contains("message") &&
        response["choices"][0]["message"].contains("content")) {
      
      result_text = response["choices"][0]["message"]["content"];
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

}  // namespace copilot
}  // namespace adapters
}  // namespace asol