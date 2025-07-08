// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "asol/adapters/claude/claude_text_adapter.h"

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
namespace claude {

namespace {
// Constants for Claude API
constexpr char kClaudeApiEndpoint[] = "https://api.anthropic.com/v1/messages";
constexpr char kAuthorizationHeader[] = "x-api-key: ";
constexpr char kAnthropicVersionHeader[] = "anthropic-version: ";
constexpr char kContentTypeHeader[] = "Content-Type: application/json";

// Helper function to truncate text for logging
std::string TruncateForLogging(const std::string& text, size_t max_length = 50) {
  if (text.length() <= max_length)
    return text;
  return text.substr(0, max_length) + "...";
}

}  // namespace

ClaudeTextAdapter::ClaudeTextAdapter() : config_() {
  LOG(INFO) << "ClaudeTextAdapter initialized with default configuration.";
}

ClaudeTextAdapter::ClaudeTextAdapter(const std::string& api_key) 
    : api_key_(api_key), config_() {
  LOG(INFO) << "ClaudeTextAdapter initialized with provided API key.";
}

ClaudeTextAdapter::~ClaudeTextAdapter() {
  LOG(INFO) << "ClaudeTextAdapter destroyed.";
}

void ClaudeTextAdapter::ProcessText(
    const std::string& text_input,
    ClaudeResponseCallback callback) {
  DLOG(INFO) << "Processing text with Claude Adapter: " 
             << TruncateForLogging(text_input);
  
  // Build the request payload for a single text prompt
  nlohmann::json payload = BuildRequestPayload(text_input);
  
  // Send the request to the Claude API
  SendRequest(payload, std::move(callback));
}

void ClaudeTextAdapter::ProcessConversation(
    const std::vector<ClaudeMessage>& messages,
    ClaudeResponseCallback callback) {
  DLOG(INFO) << "Processing conversation with " << messages.size() << " messages";
  
  // Build the request payload for a conversation
  nlohmann::json payload = BuildConversationPayload(messages);
  
  // Send the request to the Claude API
  SendRequest(payload, std::move(callback));
}

void ClaudeTextAdapter::SetRequestConfig(const ClaudeRequestConfig& config) {
  config_ = config;
  DLOG(INFO) << "Updated Claude request configuration. Model: " 
             << config_.model_name;
}

const ClaudeRequestConfig& ClaudeTextAdapter::GetRequestConfig() const {
  return config_;
}

void ClaudeTextAdapter::SetApiKey(const std::string& api_key) {
  api_key_ = api_key;
  DLOG(INFO) << "Updated API key.";
}

nlohmann::json ClaudeTextAdapter::BuildRequestPayload(
    const std::string& text_input) const {
  nlohmann::json payload;
  
  // Set the model
  payload["model"] = config_.model_name;
  
  // Add the messages array with system and user messages
  nlohmann::json messages = nlohmann::json::array();
  
  // Claude API expects a different format than OpenAI/Gemini
  // For a single text prompt, we'll create a simple user message
  payload["messages"] = nlohmann::json::array({
    {
      {"role", "user"},
      {"content", text_input}
    }
  });
  
  // Add system prompt if needed
  payload["system"] = "You are Claude, a helpful AI assistant created by Anthropic.";
  
  // Add generation parameters
  payload["temperature"] = config_.temperature;
  payload["max_tokens"] = config_.max_tokens;
  payload["top_p"] = config_.top_p;
  payload["top_k"] = config_.top_k;
  
  return payload;
}

nlohmann::json ClaudeTextAdapter::BuildConversationPayload(
    const std::vector<ClaudeMessage>& messages) const {
  nlohmann::json payload;
  
  // Set the model
  payload["model"] = config_.model_name;
  
  // Extract system message if present
  std::string system_content = "You are Claude, a helpful AI assistant created by Anthropic.";
  nlohmann::json messages_array = nlohmann::json::array();
  
  for (const auto& message : messages) {
    if (message.role == ClaudeMessage::Role::SYSTEM) {
      system_content = message.content;
      // Don't add system message to the messages array for Claude
      continue;
    }
    
    nlohmann::json msg_obj;
    msg_obj["role"] = RoleToString(message.role);
    msg_obj["content"] = message.content;
    messages_array.push_back(msg_obj);
  }
  
  payload["messages"] = messages_array;
  payload["system"] = system_content;
  
  // Add generation parameters
  payload["temperature"] = config_.temperature;
  payload["max_tokens"] = config_.max_tokens;
  payload["top_p"] = config_.top_p;
  payload["top_k"] = config_.top_k;
  
  return payload;
}

std::string ClaudeTextAdapter::RoleToString(ClaudeMessage::Role role) const {
  switch (role) {
    case ClaudeMessage::Role::USER:
      return "user";
    case ClaudeMessage::Role::ASSISTANT:
      return "assistant";
    case ClaudeMessage::Role::SYSTEM:
      // Claude handles system messages differently, but we'll return a value anyway
      return "system";
    default:
      LOG(ERROR) << "Unknown role type, defaulting to 'user'";
      return "user";
  }
}

void ClaudeTextAdapter::SendRequest(
    const nlohmann::json& payload,
    ClaudeResponseCallback callback) {
  // In a real implementation, this would use network services to send an HTTP request
  // For now, we'll simulate a response
  
  std::string json_str = payload.dump(2);
  DLOG(INFO) << "Claude API Request payload: " << TruncateForLogging(json_str, 100);
  
  // Construct the headers (would be used in actual implementation)
  std::vector<std::string> headers;
  headers.push_back(std::string(kAuthorizationHeader) + api_key_);
  headers.push_back(std::string(kAnthropicVersionHeader) + config_.anthropic_version);
  headers.push_back(std::string(kContentTypeHeader));
  
  DLOG(INFO) << "Would send request to: " << kClaudeApiEndpoint;
  
  // Simulate async processing
  base::ThreadPool::PostDelayedTask(
      FROM_HERE,
      {base::TaskPriority::BEST_EFFORT},
      base::BindOnce(&ClaudeTextAdapter::HandleResponse,
                    weak_ptr_factory_.GetWeakPtr(),
                    // Simulate a response - in real implementation this would be the API response
                    R"({
                      "id": "msg_01234567890",
                      "type": "message",
                      "role": "assistant",
                      "content": [
                        {
                          "type": "text",
                          "text": "This is a simulated response from the Claude API."
                        }
                      ],
                      "model": "claude-3-opus-20240229",
                      "stop_reason": "end_turn",
                      "stop_sequence": null,
                      "usage": {
                        "input_tokens": 10,
                        "output_tokens": 12
                      }
                    })",
                    std::move(callback)),
      base::Milliseconds(100));  // Simulate network delay
}

void ClaudeTextAdapter::HandleResponse(
    const std::string& response_data,
    ClaudeResponseCallback callback) {
  DLOG(INFO) << "Handling Claude API response: " 
             << TruncateForLogging(response_data, 100);
  
  bool success = true;
  std::string result_text;
  
  try {
    // Parse the JSON response
    nlohmann::json response = nlohmann::json::parse(response_data);
    
    // Extract the response text - Claude's format is different from OpenAI/Gemini
    if (response.contains("content") && 
        response["content"].is_array() && 
        !response["content"].empty() && 
        response["content"][0].contains("text")) {
      
      result_text = response["content"][0]["text"];
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

}  // namespace claude
}  // namespace adapters
}  // namespace asol