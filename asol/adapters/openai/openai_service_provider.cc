// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "asol/adapters/openai/openai_service_provider.h"

#include <utility>

#include "base/logging.h"
#include "base/strings/string_util.h"

namespace asol {
namespace adapters {
namespace openai {

namespace {
// Constants for OpenAI provider
constexpr char kProviderId[] = "openai";
constexpr char kProviderName[] = "OpenAI";
constexpr char kProviderVersion[] = "1.0.0";

// Configuration keys
constexpr char kConfigKeyApiKey[] = "api_key";
constexpr char kConfigKeyModel[] = "model";
constexpr char kConfigKeyTemperature[] = "temperature";
constexpr char kConfigKeyMaxTokens[] = "max_tokens";
constexpr char kConfigKeyOrganizationId[] = "organization_id";
}  // namespace

OpenAIServiceProvider::OpenAIServiceProvider() {
  openai_adapter_ = std::make_unique<OpenAITextAdapter>();
  LOG(INFO) << "OpenAIServiceProvider initialized with default configuration.";
}

OpenAIServiceProvider::OpenAIServiceProvider(const std::string& api_key) {
  openai_adapter_ = std::make_unique<OpenAITextAdapter>(api_key);
  config_[kConfigKeyApiKey] = api_key;
  LOG(INFO) << "OpenAIServiceProvider initialized with provided API key.";
}

OpenAIServiceProvider::~OpenAIServiceProvider() {
  LOG(INFO) << "OpenAIServiceProvider destroyed.";
}

std::string OpenAIServiceProvider::GetProviderId() const {
  return kProviderId;
}

std::string OpenAIServiceProvider::GetProviderName() const {
  return kProviderName;
}

std::string OpenAIServiceProvider::GetProviderVersion() const {
  return kProviderVersion;
}

core::AIServiceProvider::Capabilities OpenAIServiceProvider::GetCapabilities() const {
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

bool OpenAIServiceProvider::SupportsTaskType(TaskType task_type) const {
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

void OpenAIServiceProvider::ProcessRequest(
    const AIRequestParams& params,
    AIResponseCallback callback) {
  DLOG(INFO) << "Processing request with OpenAI provider. Task type: "
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
      std::move(callback).Run(false, "Unsupported task type for OpenAI provider");
      break;
  }
}

void OpenAIServiceProvider::Configure(
    const std::unordered_map<std::string, std::string>& config) {
  // Update the configuration
  for (const auto& [key, value] : config) {
    config_[key] = value;
  }
  
  // Apply configuration to the adapter
  if (config.find(kConfigKeyApiKey) != config.end()) {
    openai_adapter_->SetApiKey(config.at(kConfigKeyApiKey));
  }
  
  // Update adapter configuration
  OpenAIRequestConfig adapter_config = openai_adapter_->GetRequestConfig();
  
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
  
  if (config.find(kConfigKeyOrganizationId) != config.end()) {
    adapter_config.organization_id = config.at(kConfigKeyOrganizationId);
  }
  
  openai_adapter_->SetRequestConfig(adapter_config);
  
  LOG(INFO) << "OpenAIServiceProvider configuration updated.";
}

std::unordered_map<std::string, std::string> OpenAIServiceProvider::GetConfiguration() const {
  return config_;
}

void OpenAIServiceProvider::ProcessTextGeneration(
    const AIRequestParams& params,
    AIResponseCallback callback) {
  // If we have context, use conversation API
  if (!params.context_id.empty() && params.custom_params.find("context_messages") != params.custom_params.end()) {
    // Parse context messages from the custom params
    // In a real implementation, this would come from the ContextManager
    std::vector<core::ContextMessage> context_messages;
    // Assume context_messages are serialized in the custom_params
    
    // Convert to OpenAI message format
    std::vector<OpenAIMessage> openai_messages = ConvertToOpenAIMessages(context_messages);
    
    // Add the current user message
    OpenAIMessage user_message;
    user_message.role = OpenAIMessage::Role::USER;
    user_message.content = params.input_text;
    openai_messages.push_back(user_message);
    
    // Process the conversation
    openai_adapter_->ProcessConversation(
        openai_messages,
        base::BindOnce(&OpenAIServiceProvider::OnOpenAIResponse,
                      weak_ptr_factory_.GetWeakPtr(),
                      std::move(callback)));
  } else {
    // No context, just process the text
    openai_adapter_->ProcessText(
        params.input_text,
        base::BindOnce(&OpenAIServiceProvider::OnOpenAIResponse,
                      weak_ptr_factory_.GetWeakPtr(),
                      std::move(callback)));
  }
}

void OpenAIServiceProvider::ProcessTextSummarization(
    const AIRequestParams& params,
    AIResponseCallback callback) {
  // Add a system message to guide the model
  std::vector<OpenAIMessage> messages;
  
  OpenAIMessage system_message;
  system_message.role = OpenAIMessage::Role::SYSTEM;
  system_message.content = "Summarize the following text concisely while preserving the key information.";
  
  OpenAIMessage user_message;
  user_message.role = OpenAIMessage::Role::USER;
  user_message.content = params.input_text;
  
  messages.push_back(system_message);
  messages.push_back(user_message);
  
  // Process the conversation
  openai_adapter_->ProcessConversation(
      messages,
      base::BindOnce(&OpenAIServiceProvider::OnOpenAIResponse,
                    weak_ptr_factory_.GetWeakPtr(),
                    std::move(callback)));
}

void OpenAIServiceProvider::ProcessContentAnalysis(
    const AIRequestParams& params,
    AIResponseCallback callback) {
  // Add a system message to guide the model
  std::vector<OpenAIMessage> messages;
  
  OpenAIMessage system_message;
  system_message.role = OpenAIMessage::Role::SYSTEM;
  system_message.content = "Analyze the following content. Identify key topics, entities, sentiment, and main points.";
  
  OpenAIMessage user_message;
  user_message.role = OpenAIMessage::Role::USER;
  user_message.content = params.input_text;
  
  messages.push_back(system_message);
  messages.push_back(user_message);
  
  // Process the conversation
  openai_adapter_->ProcessConversation(
      messages,
      base::BindOnce(&OpenAIServiceProvider::OnOpenAIResponse,
                    weak_ptr_factory_.GetWeakPtr(),
                    std::move(callback)));
}

void OpenAIServiceProvider::ProcessQuestionAnswering(
    const AIRequestParams& params,
    AIResponseCallback callback) {
  // If we have context, use conversation API
  if (!params.context_id.empty() && params.custom_params.find("context_messages") != params.custom_params.end()) {
    // Parse context messages from the custom params
    // In a real implementation, this would come from the ContextManager
    std::vector<core::ContextMessage> context_messages;
    // Assume context_messages are serialized in the custom_params
    
    // Convert to OpenAI message format
    std::vector<OpenAIMessage> openai_messages = ConvertToOpenAIMessages(context_messages);
    
    // Add the current user message
    OpenAIMessage user_message;
    user_message.role = OpenAIMessage::Role::USER;
    user_message.content = params.input_text;
    openai_messages.push_back(user_message);
    
    // Process the conversation
    openai_adapter_->ProcessConversation(
        openai_messages,
        base::BindOnce(&OpenAIServiceProvider::OnOpenAIResponse,
                      weak_ptr_factory_.GetWeakPtr(),
                      std::move(callback)));
  } else {
    // No context, just process the text
    openai_adapter_->ProcessText(
        params.input_text,
        base::BindOnce(&OpenAIServiceProvider::OnOpenAIResponse,
                      weak_ptr_factory_.GetWeakPtr(),
                      std::move(callback)));
  }
}

void OpenAIServiceProvider::ProcessCodeGeneration(
    const AIRequestParams& params,
    AIResponseCallback callback) {
  // Add a system message to guide the model
  std::vector<OpenAIMessage> messages;
  
  OpenAIMessage system_message;
  system_message.role = OpenAIMessage::Role::SYSTEM;
  system_message.content = "You are a helpful coding assistant. Generate clean, efficient, and well-documented code based on the user's requirements.";
  
  OpenAIMessage user_message;
  user_message.role = OpenAIMessage::Role::USER;
  user_message.content = params.input_text;
  
  messages.push_back(system_message);
  messages.push_back(user_message);
  
  // Use a code-specific model if available
  OpenAIRequestConfig config = openai_adapter_->GetRequestConfig();
  std::string original_model = config.model_name;
  
  // Check if we should use a code-specific model
  if (config.model_name == "gpt-4o") {
    // GPT-4o is already good for code
  } else if (config.model_name == "gpt-3.5-turbo") {
    // Could switch to a more code-focused model if needed
  }
  
  // Process the conversation
  openai_adapter_->ProcessConversation(
      messages,
      base::BindOnce(&OpenAIServiceProvider::OnOpenAIResponse,
                    weak_ptr_factory_.GetWeakPtr(),
                    std::move(callback)));
  
  // Restore the original model if we changed it
  if (config.model_name != original_model) {
    config.model_name = original_model;
    openai_adapter_->SetRequestConfig(config);
  }
}

void OpenAIServiceProvider::ProcessTranslation(
    const AIRequestParams& params,
    AIResponseCallback callback) {
  // Add a system message to guide the model
  std::vector<OpenAIMessage> messages;
  
  // Check if target language is specified
  std::string target_language = "English";  // Default
  if (params.custom_params.find("target_language") != params.custom_params.end()) {
    target_language = params.custom_params.at("target_language");
  }
  
  OpenAIMessage system_message;
  system_message.role = OpenAIMessage::Role::SYSTEM;
  system_message.content = "Translate the following text to " + target_language + ". Maintain the original meaning, tone, and style as closely as possible.";
  
  OpenAIMessage user_message;
  user_message.role = OpenAIMessage::Role::USER;
  user_message.content = params.input_text;
  
  messages.push_back(system_message);
  messages.push_back(user_message);
  
  // Process the conversation
  openai_adapter_->ProcessConversation(
      messages,
      base::BindOnce(&OpenAIServiceProvider::OnOpenAIResponse,
                    weak_ptr_factory_.GetWeakPtr(),
                    std::move(callback)));
}

std::vector<OpenAIMessage> OpenAIServiceProvider::ConvertToOpenAIMessages(
    const std::vector<core::ContextMessage>& context_messages) {
  std::vector<OpenAIMessage> openai_messages;
  
  for (const auto& context_message : context_messages) {
    OpenAIMessage openai_message;
    
    // Convert role
    switch (context_message.role) {
      case core::ContextMessage::Role::USER:
        openai_message.role = OpenAIMessage::Role::USER;
        break;
      case core::ContextMessage::Role::ASSISTANT:
        openai_message.role = OpenAIMessage::Role::ASSISTANT;
        break;
      case core::ContextMessage::Role::SYSTEM:
        openai_message.role = OpenAIMessage::Role::SYSTEM;
        break;
      default:
        LOG(ERROR) << "Unknown context message role, defaulting to USER";
        openai_message.role = OpenAIMessage::Role::USER;
        break;
    }
    
    openai_message.content = context_message.content;
    openai_messages.push_back(openai_message);
  }
  
  return openai_messages;
}

void OpenAIServiceProvider::OnOpenAIResponse(
    AIResponseCallback callback,
    bool success,
    const std::string& response) {
  // Just pass through the response for now
  // In a real implementation, we might do additional processing
  std::move(callback).Run(success, response);
}

}  // namespace openai
}  // namespace adapters
}  // namespace asol