// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "asol/adapters/claude/claude_service_provider.h"

#include <utility>

#include "base/logging.h"
#include "base/strings/string_util.h"

namespace asol {
namespace adapters {
namespace claude {

namespace {
// Constants for Claude provider
constexpr char kProviderId[] = "claude";
constexpr char kProviderName[] = "Anthropic Claude";
constexpr char kProviderVersion[] = "1.0.0";

// Configuration keys
constexpr char kConfigKeyApiKey[] = "api_key";
constexpr char kConfigKeyModel[] = "model";
constexpr char kConfigKeyTemperature[] = "temperature";
constexpr char kConfigKeyMaxTokens[] = "max_tokens";
constexpr char kConfigKeyAnthropicVersion[] = "anthropic_version";
}  // namespace

ClaudeServiceProvider::ClaudeServiceProvider() {
  claude_adapter_ = std::make_unique<ClaudeTextAdapter>();
  LOG(INFO) << "ClaudeServiceProvider initialized with default configuration.";
}

ClaudeServiceProvider::ClaudeServiceProvider(const std::string& api_key) {
  claude_adapter_ = std::make_unique<ClaudeTextAdapter>(api_key);
  config_[kConfigKeyApiKey] = api_key;
  LOG(INFO) << "ClaudeServiceProvider initialized with provided API key.";
}

ClaudeServiceProvider::~ClaudeServiceProvider() {
  LOG(INFO) << "ClaudeServiceProvider destroyed.";
}

std::string ClaudeServiceProvider::GetProviderId() const {
  return kProviderId;
}

std::string ClaudeServiceProvider::GetProviderName() const {
  return kProviderName;
}

std::string ClaudeServiceProvider::GetProviderVersion() const {
  return kProviderVersion;
}

core::AIServiceProvider::Capabilities ClaudeServiceProvider::GetCapabilities() const {
  Capabilities capabilities;
  capabilities.supports_text_generation = true;
  capabilities.supports_text_summarization = true;
  capabilities.supports_content_analysis = true;
  capabilities.supports_code_generation = true;
  capabilities.supports_question_answering = true;
  capabilities.supports_translation = true;
  capabilities.supports_context = true;
  
  // Add supported languages
  capabilities.supported_languages = {
    "en", "es", "fr", "de", "it", "pt", "nl", "ru", "zh", "ja", "ko", "ar"
  };
  
  return capabilities;
}

bool ClaudeServiceProvider::SupportsTaskType(TaskType task_type) const {
  switch (task_type) {
    case TaskType::TEXT_GENERATION:
    case TaskType::TEXT_SUMMARIZATION:
    case TaskType::CONTENT_ANALYSIS:
    case TaskType::CODE_GENERATION:
    case TaskType::QUESTION_ANSWERING:
    case TaskType::TRANSLATION:
      return true;
    case TaskType::IMAGE_ANALYSIS:
    case TaskType::CUSTOM:
    default:
      return false;
  }
}

void ClaudeServiceProvider::ProcessRequest(
    const AIRequestParams& params,
    AIResponseCallback callback) {
  DLOG(INFO) << "Processing request with Claude provider. Task type: "
             << static_cast<int>(params.task_type);
  
  // Route to the appropriate handler based on task type
  switch (params.task_type) {
    case TaskType::TEXT_GENERATION:
      ProcessTextGeneration(params, std::move(callback));
      break;
    case TaskType::TEXT_SUMMARIZATION:
      ProcessTextSummarization(params, std::move(callback));
      break;
    case TaskType::CONTENT_ANALYSIS:
      ProcessContentAnalysis(params, std::move(callback));
      break;
    case TaskType::CODE_GENERATION:
      ProcessCodeGeneration(params, std::move(callback));
      break;
    case TaskType::QUESTION_ANSWERING:
      ProcessQuestionAnswering(params, std::move(callback));
      break;
    case TaskType::TRANSLATION:
      ProcessTranslation(params, std::move(callback));
      break;
    default:
      std::move(callback).Run(false, "Unsupported task type for Claude provider");
      break;
  }
}

void ClaudeServiceProvider::Configure(
    const std::unordered_map<std::string, std::string>& config) {
  // Update the configuration
  for (const auto& [key, value] : config) {
    config_[key] = value;
  }
  
  // Apply configuration to the adapter
  if (config.find(kConfigKeyApiKey) != config.end()) {
    claude_adapter_->SetApiKey(config.at(kConfigKeyApiKey));
  }
  
  // Update adapter configuration
  ClaudeRequestConfig adapter_config = claude_adapter_->GetRequestConfig();
  
  if (config.find(kConfigKeyModel) != config.end()) {
    adapter_config.model_name = config.at(kConfigKeyModel);
  }
  
  if (config.find(kConfigKeyTemperature) != config.end()) {
    try {
      adapter_config.temperature = std::stof(config.at(kConfigKeyTemperature));
    } catch (const std::exception& e) {
      LOG(ERROR) << "Failed to parse temperature: " << e.what();
    }
  }
  
  if (config.find(kConfigKeyMaxTokens) != config.end()) {
    try {
      adapter_config.max_tokens = std::stoi(config.at(kConfigKeyMaxTokens));
    } catch (const std::exception& e) {
      LOG(ERROR) << "Failed to parse max_tokens: " << e.what();
    }
  }
  
  if (config.find(kConfigKeyAnthropicVersion) != config.end()) {
    adapter_config.anthropic_version = config.at(kConfigKeyAnthropicVersion);
  }
  
  claude_adapter_->SetRequestConfig(adapter_config);
  
  LOG(INFO) << "ClaudeServiceProvider configuration updated.";
}

std::unordered_map<std::string, std::string> ClaudeServiceProvider::GetConfiguration() const {
  return config_;
}

void ClaudeServiceProvider::ProcessTextGeneration(
    const AIRequestParams& params,
    AIResponseCallback callback) {
  // If we have context, use conversation API
  if (!params.context_id.empty() && params.custom_params.find("context_messages") != params.custom_params.end()) {
    // Parse context messages from the custom params
    // In a real implementation, this would come from the ContextManager
    std::vector<core::ContextMessage> context_messages;
    // Assume context_messages are serialized in the custom_params
    
    // Convert to Claude message format
    std::vector<ClaudeMessage> claude_messages = ConvertToClaudeMessages(context_messages);
    
    // Add the current user message
    ClaudeMessage user_message;
    user_message.role = ClaudeMessage::Role::USER;
    user_message.content = params.input_text;
    claude_messages.push_back(user_message);
    
    // Process the conversation
    claude_adapter_->ProcessConversation(
        claude_messages,
        base::BindOnce(&ClaudeServiceProvider::OnClaudeResponse,
                      weak_ptr_factory_.GetWeakPtr(),
                      std::move(callback)));
  } else {
    // No context, just process the text
    claude_adapter_->ProcessText(
        params.input_text,
        base::BindOnce(&ClaudeServiceProvider::OnClaudeResponse,
                      weak_ptr_factory_.GetWeakPtr(),
                      std::move(callback)));
  }
}

void ClaudeServiceProvider::ProcessTextSummarization(
    const AIRequestParams& params,
    AIResponseCallback callback) {
  // Add a system message to guide the model
  std::vector<ClaudeMessage> messages;
  
  ClaudeMessage system_message;
  system_message.role = ClaudeMessage::Role::SYSTEM;
  system_message.content = "Summarize the following text concisely while preserving the key information.";
  
  ClaudeMessage user_message;
  user_message.role = ClaudeMessage::Role::USER;
  user_message.content = params.input_text;
  
  messages.push_back(system_message);
  messages.push_back(user_message);
  
  // Process the conversation
  claude_adapter_->ProcessConversation(
      messages,
      base::BindOnce(&ClaudeServiceProvider::OnClaudeResponse,
                    weak_ptr_factory_.GetWeakPtr(),
                    std::move(callback)));
}

void ClaudeServiceProvider::ProcessContentAnalysis(
    const AIRequestParams& params,
    AIResponseCallback callback) {
  // Add a system message to guide the model
  std::vector<ClaudeMessage> messages;
  
  ClaudeMessage system_message;
  system_message.role = ClaudeMessage::Role::SYSTEM;
  system_message.content = "Analyze the following content. Identify key topics, entities, sentiment, and main points.";
  
  ClaudeMessage user_message;
  user_message.role = ClaudeMessage::Role::USER;
  user_message.content = params.input_text;
  
  messages.push_back(system_message);
  messages.push_back(user_message);
  
  // Process the conversation
  claude_adapter_->ProcessConversation(
      messages,
      base::BindOnce(&ClaudeServiceProvider::OnClaudeResponse,
                    weak_ptr_factory_.GetWeakPtr(),
                    std::move(callback)));
}

void ClaudeServiceProvider::ProcessQuestionAnswering(
    const AIRequestParams& params,
    AIResponseCallback callback) {
  // If we have context, use conversation API
  if (!params.context_id.empty() && params.custom_params.find("context_messages") != params.custom_params.end()) {
    // Parse context messages from the custom params
    // In a real implementation, this would come from the ContextManager
    std::vector<core::ContextMessage> context_messages;
    // Assume context_messages are serialized in the custom_params
    
    // Convert to Claude message format
    std::vector<ClaudeMessage> claude_messages = ConvertToClaudeMessages(context_messages);
    
    // Add the current user message
    ClaudeMessage user_message;
    user_message.role = ClaudeMessage::Role::USER;
    user_message.content = params.input_text;
    claude_messages.push_back(user_message);
    
    // Process the conversation
    claude_adapter_->ProcessConversation(
        claude_messages,
        base::BindOnce(&ClaudeServiceProvider::OnClaudeResponse,
                      weak_ptr_factory_.GetWeakPtr(),
                      std::move(callback)));
  } else {
    // No context, just process the text
    claude_adapter_->ProcessText(
        params.input_text,
        base::BindOnce(&ClaudeServiceProvider::OnClaudeResponse,
                      weak_ptr_factory_.GetWeakPtr(),
                      std::move(callback)));
  }
}

void ClaudeServiceProvider::ProcessCodeGeneration(
    const AIRequestParams& params,
    AIResponseCallback callback) {
  // Add a system message to guide the model
  std::vector<ClaudeMessage> messages;
  
  ClaudeMessage system_message;
  system_message.role = ClaudeMessage::Role::SYSTEM;
  system_message.content = "You are Claude, a helpful coding assistant. Generate clean, efficient, and well-documented code based on the user's requirements.";
  
  ClaudeMessage user_message;
  user_message.role = ClaudeMessage::Role::USER;
  user_message.content = params.input_text;
  
  messages.push_back(system_message);
  messages.push_back(user_message);
  
  // Process the conversation
  claude_adapter_->ProcessConversation(
      messages,
      base::BindOnce(&ClaudeServiceProvider::OnClaudeResponse,
                    weak_ptr_factory_.GetWeakPtr(),
                    std::move(callback)));
}

void ClaudeServiceProvider::ProcessTranslation(
    const AIRequestParams& params,
    AIResponseCallback callback) {
  // Add a system message to guide the model
  std::vector<ClaudeMessage> messages;
  
  // Check if target language is specified
  std::string target_language = "English";  // Default
  if (params.custom_params.find("target_language") != params.custom_params.end()) {
    target_language = params.custom_params.at("target_language");
  }
  
  ClaudeMessage system_message;
  system_message.role = ClaudeMessage::Role::SYSTEM;
  system_message.content = "Translate the following text to " + target_language + ". Maintain the original meaning, tone, and style as closely as possible.";
  
  ClaudeMessage user_message;
  user_message.role = ClaudeMessage::Role::USER;
  user_message.content = params.input_text;
  
  messages.push_back(system_message);
  messages.push_back(user_message);
  
  // Process the conversation
  claude_adapter_->ProcessConversation(
      messages,
      base::BindOnce(&ClaudeServiceProvider::OnClaudeResponse,
                    weak_ptr_factory_.GetWeakPtr(),
                    std::move(callback)));
}

std::vector<ClaudeMessage> ClaudeServiceProvider::ConvertToClaudeMessages(
    const std::vector<core::ContextMessage>& context_messages) {
  std::vector<ClaudeMessage> claude_messages;
  
  for (const auto& context_message : context_messages) {
    ClaudeMessage claude_message;
    
    // Convert role
    switch (context_message.role) {
      case core::ContextMessage::Role::USER:
        claude_message.role = ClaudeMessage::Role::USER;
        break;
      case core::ContextMessage::Role::ASSISTANT:
        claude_message.role = ClaudeMessage::Role::ASSISTANT;
        break;
      case core::ContextMessage::Role::SYSTEM:
        claude_message.role = ClaudeMessage::Role::SYSTEM;
        break;
      default:
        LOG(ERROR) << "Unknown context message role, defaulting to USER";
        claude_message.role = ClaudeMessage::Role::USER;
        break;
    }
    
    claude_message.content = context_message.content;
    claude_messages.push_back(claude_message);
  }
  
  return claude_messages;
}

void ClaudeServiceProvider::OnClaudeResponse(
    AIResponseCallback callback,
    bool success,
    const std::string& response) {
  // Just pass through the response for now
  // In a real implementation, we might do additional processing
  std::move(callback).Run(success, response);
}

}  // namespace claude
}  // namespace adapters
}  // namespace asol