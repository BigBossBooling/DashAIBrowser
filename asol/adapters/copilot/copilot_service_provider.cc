// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "asol/adapters/copilot/copilot_service_provider.h"

#include <utility>

#include "base/logging.h"
#include "base/strings/string_util.h"

namespace asol {
namespace adapters {
namespace copilot {

namespace {
// Constants for Copilot provider
constexpr char kProviderId[] = "copilot";
constexpr char kProviderName[] = "Microsoft Copilot";
constexpr char kProviderVersion[] = "1.0.0";

// Configuration keys
constexpr char kConfigKeyApiKey[] = "api_key";
constexpr char kConfigKeyEndpoint[] = "endpoint";
constexpr char kConfigKeyModel[] = "model";
constexpr char kConfigKeyTemperature[] = "temperature";
constexpr char kConfigKeyMaxTokens[] = "max_tokens";
constexpr char kConfigKeyApiVersion[] = "api_version";
}  // namespace

CopilotServiceProvider::CopilotServiceProvider() {
  copilot_adapter_ = std::make_unique<CopilotTextAdapter>();
  LOG(INFO) << "CopilotServiceProvider initialized with default configuration.";
}

CopilotServiceProvider::CopilotServiceProvider(const std::string& api_key) {
  copilot_adapter_ = std::make_unique<CopilotTextAdapter>(api_key);
  config_[kConfigKeyApiKey] = api_key;
  LOG(INFO) << "CopilotServiceProvider initialized with provided API key.";
}

CopilotServiceProvider::~CopilotServiceProvider() {
  LOG(INFO) << "CopilotServiceProvider destroyed.";
}

std::string CopilotServiceProvider::GetProviderId() const {
  return kProviderId;
}

std::string CopilotServiceProvider::GetProviderName() const {
  return kProviderName;
}

std::string CopilotServiceProvider::GetProviderVersion() const {
  return kProviderVersion;
}

core::AIServiceProvider::Capabilities CopilotServiceProvider::GetCapabilities() const {
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

bool CopilotServiceProvider::SupportsTaskType(TaskType task_type) const {
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

void CopilotServiceProvider::ProcessRequest(
    const AIRequestParams& params,
    AIResponseCallback callback) {
  DLOG(INFO) << "Processing request with Copilot provider. Task type: "
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
      std::move(callback).Run(false, "Unsupported task type for Copilot provider");
      break;
  }
}

void CopilotServiceProvider::Configure(
    const std::unordered_map<std::string, std::string>& config) {
  // Update the configuration
  for (const auto& [key, value] : config) {
    config_[key] = value;
  }
  
  // Apply configuration to the adapter
  if (config.find(kConfigKeyApiKey) != config.end()) {
    copilot_adapter_->SetApiKey(config.at(kConfigKeyApiKey));
  }
  
  if (config.find(kConfigKeyEndpoint) != config.end()) {
    copilot_adapter_->SetEndpoint(config.at(kConfigKeyEndpoint));
  }
  
  // Update adapter configuration
  CopilotRequestConfig adapter_config = copilot_adapter_->GetRequestConfig();
  
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
  
  if (config.find(kConfigKeyApiVersion) != config.end()) {
    adapter_config.api_version = config.at(kConfigKeyApiVersion);
  }
  
  copilot_adapter_->SetRequestConfig(adapter_config);
  
  LOG(INFO) << "CopilotServiceProvider configuration updated.";
}

std::unordered_map<std::string, std::string> CopilotServiceProvider::GetConfiguration() const {
  return config_;
}

void CopilotServiceProvider::ProcessTextGeneration(
    const AIRequestParams& params,
    AIResponseCallback callback) {
  // If we have context, use conversation API
  if (!params.context_id.empty() && params.custom_params.find("context_messages") != params.custom_params.end()) {
    // Parse context messages from the custom params
    // In a real implementation, this would come from the ContextManager
    std::vector<core::ContextMessage> context_messages;
    // Assume context_messages are serialized in the custom_params
    
    // Convert to Copilot message format
    std::vector<CopilotMessage> copilot_messages = ConvertToCopilotMessages(context_messages);
    
    // Add the current user message
    CopilotMessage user_message;
    user_message.role = CopilotMessage::Role::USER;
    user_message.content = params.input_text;
    copilot_messages.push_back(user_message);
    
    // Process the conversation
    copilot_adapter_->ProcessConversation(
        copilot_messages,
        base::BindOnce(&CopilotServiceProvider::OnCopilotResponse,
                      weak_ptr_factory_.GetWeakPtr(),
                      std::move(callback)));
  } else {
    // No context, just process the text
    copilot_adapter_->ProcessText(
        params.input_text,
        base::BindOnce(&CopilotServiceProvider::OnCopilotResponse,
                      weak_ptr_factory_.GetWeakPtr(),
                      std::move(callback)));
  }
}

void CopilotServiceProvider::ProcessTextSummarization(
    const AIRequestParams& params,
    AIResponseCallback callback) {
  // Add a system message to guide the model
  std::vector<CopilotMessage> messages;
  
  CopilotMessage system_message;
  system_message.role = CopilotMessage::Role::SYSTEM;
  system_message.content = "Summarize the following text concisely while preserving the key information.";
  
  CopilotMessage user_message;
  user_message.role = CopilotMessage::Role::USER;
  user_message.content = params.input_text;
  
  messages.push_back(system_message);
  messages.push_back(user_message);
  
  // Process the conversation
  copilot_adapter_->ProcessConversation(
      messages,
      base::BindOnce(&CopilotServiceProvider::OnCopilotResponse,
                    weak_ptr_factory_.GetWeakPtr(),
                    std::move(callback)));
}

void CopilotServiceProvider::ProcessContentAnalysis(
    const AIRequestParams& params,
    AIResponseCallback callback) {
  // Add a system message to guide the model
  std::vector<CopilotMessage> messages;
  
  CopilotMessage system_message;
  system_message.role = CopilotMessage::Role::SYSTEM;
  system_message.content = "Analyze the following content. Identify key topics, entities, sentiment, and main points.";
  
  CopilotMessage user_message;
  user_message.role = CopilotMessage::Role::USER;
  user_message.content = params.input_text;
  
  messages.push_back(system_message);
  messages.push_back(user_message);
  
  // Process the conversation
  copilot_adapter_->ProcessConversation(
      messages,
      base::BindOnce(&CopilotServiceProvider::OnCopilotResponse,
                    weak_ptr_factory_.GetWeakPtr(),
                    std::move(callback)));
}

void CopilotServiceProvider::ProcessQuestionAnswering(
    const AIRequestParams& params,
    AIResponseCallback callback) {
  // If we have context, use conversation API
  if (!params.context_id.empty() && params.custom_params.find("context_messages") != params.custom_params.end()) {
    // Parse context messages from the custom params
    // In a real implementation, this would come from the ContextManager
    std::vector<core::ContextMessage> context_messages;
    // Assume context_messages are serialized in the custom_params
    
    // Convert to Copilot message format
    std::vector<CopilotMessage> copilot_messages = ConvertToCopilotMessages(context_messages);
    
    // Add the current user message
    CopilotMessage user_message;
    user_message.role = CopilotMessage::Role::USER;
    user_message.content = params.input_text;
    copilot_messages.push_back(user_message);
    
    // Process the conversation
    copilot_adapter_->ProcessConversation(
        copilot_messages,
        base::BindOnce(&CopilotServiceProvider::OnCopilotResponse,
                      weak_ptr_factory_.GetWeakPtr(),
                      std::move(callback)));
  } else {
    // No context, just process the text
    copilot_adapter_->ProcessText(
        params.input_text,
        base::BindOnce(&CopilotServiceProvider::OnCopilotResponse,
                      weak_ptr_factory_.GetWeakPtr(),
                      std::move(callback)));
  }
}

void CopilotServiceProvider::ProcessCodeGeneration(
    const AIRequestParams& params,
    AIResponseCallback callback) {
  // Add a system message to guide the model
  std::vector<CopilotMessage> messages;
  
  CopilotMessage system_message;
  system_message.role = CopilotMessage::Role::SYSTEM;
  system_message.content = "You are Microsoft Copilot, a helpful coding assistant. Generate clean, efficient, and well-documented code based on the user's requirements.";
  
  CopilotMessage user_message;
  user_message.role = CopilotMessage::Role::USER;
  user_message.content = params.input_text;
  
  messages.push_back(system_message);
  messages.push_back(user_message);
  
  // Process the conversation
  copilot_adapter_->ProcessConversation(
      messages,
      base::BindOnce(&CopilotServiceProvider::OnCopilotResponse,
                    weak_ptr_factory_.GetWeakPtr(),
                    std::move(callback)));
}

void CopilotServiceProvider::ProcessTranslation(
    const AIRequestParams& params,
    AIResponseCallback callback) {
  // Add a system message to guide the model
  std::vector<CopilotMessage> messages;
  
  // Check if target language is specified
  std::string target_language = "English";  // Default
  if (params.custom_params.find("target_language") != params.custom_params.end()) {
    target_language = params.custom_params.at("target_language");
  }
  
  CopilotMessage system_message;
  system_message.role = CopilotMessage::Role::SYSTEM;
  system_message.content = "Translate the following text to " + target_language + ". Maintain the original meaning, tone, and style as closely as possible.";
  
  CopilotMessage user_message;
  user_message.role = CopilotMessage::Role::USER;
  user_message.content = params.input_text;
  
  messages.push_back(system_message);
  messages.push_back(user_message);
  
  // Process the conversation
  copilot_adapter_->ProcessConversation(
      messages,
      base::BindOnce(&CopilotServiceProvider::OnCopilotResponse,
                    weak_ptr_factory_.GetWeakPtr(),
                    std::move(callback)));
}

std::vector<CopilotMessage> CopilotServiceProvider::ConvertToCopilotMessages(
    const std::vector<core::ContextMessage>& context_messages) {
  std::vector<CopilotMessage> copilot_messages;
  
  for (const auto& context_message : context_messages) {
    CopilotMessage copilot_message;
    
    // Convert role
    switch (context_message.role) {
      case core::ContextMessage::Role::USER:
        copilot_message.role = CopilotMessage::Role::USER;
        break;
      case core::ContextMessage::Role::ASSISTANT:
        copilot_message.role = CopilotMessage::Role::ASSISTANT;
        break;
      case core::ContextMessage::Role::SYSTEM:
        copilot_message.role = CopilotMessage::Role::SYSTEM;
        break;
      default:
        LOG(ERROR) << "Unknown context message role, defaulting to USER";
        copilot_message.role = CopilotMessage::Role::USER;
        break;
    }
    
    copilot_message.content = context_message.content;
    copilot_messages.push_back(copilot_message);
  }
  
  return copilot_messages;
}

void CopilotServiceProvider::OnCopilotResponse(
    AIResponseCallback callback,
    bool success,
    const std::string& response) {
  // Just pass through the response for now
  // In a real implementation, we might do additional processing
  std::move(callback).Run(success, response);
}

}  // namespace copilot
}  // namespace adapters
}  // namespace asol