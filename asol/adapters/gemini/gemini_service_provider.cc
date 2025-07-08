// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "asol/adapters/gemini/gemini_service_provider.h"

#include <utility>

#include "base/logging.h"
#include "base/strings/string_util.h"
#include "asol/core/context_manager.h"

namespace asol {
namespace adapters {
namespace gemini {

namespace {

// Helper function to create system prompts for different tasks
std::string CreateSystemPromptForTask(
    core::AIServiceProvider::TaskType task_type,
    const std::unordered_map<std::string, std::string>& custom_params) {
  
  switch (task_type) {
    case core::AIServiceProvider::TaskType::TEXT_SUMMARIZATION:
      return "Summarize the following text concisely while preserving the key information:";
      
    case core::AIServiceProvider::TaskType::CONTENT_ANALYSIS:
      return "Analyze the following content and provide insights about its main topics, "
             "key points, sentiment, and any notable entities mentioned:";
      
    case core::AIServiceProvider::TaskType::CODE_GENERATION:
      return "You are a helpful coding assistant. Generate clean, efficient, and well-documented "
             "code based on the following requirements:";
      
    case core::AIServiceProvider::TaskType::QUESTION_ANSWERING:
      return "You are a helpful assistant. Provide accurate, concise answers to questions. "
             "If you're unsure, acknowledge the limitations of your knowledge.";
      
    case core::AIServiceProvider::TaskType::TRANSLATION: {
      std::string target_language = "English"; // Default
      auto it = custom_params.find("target_language");
      if (it != custom_params.end()) {
        target_language = it->second;
      }
      return "Translate the following text to " + target_language + ":";
    }
      
    case core::AIServiceProvider::TaskType::TEXT_GENERATION:
    default:
      return "You are a helpful assistant. Respond to the following:";
  }
}

}  // namespace

GeminiServiceProvider::GeminiServiceProvider() {
  gemini_adapter_ = std::make_unique<GeminiTextAdapter>();
  
  // Set default configuration
  config_["model"] = "gemini-pro";
  config_["temperature"] = "0.7";
  config_["max_output_tokens"] = "1024";
}

GeminiServiceProvider::GeminiServiceProvider(const std::string& api_key) {
  gemini_adapter_ = std::make_unique<GeminiTextAdapter>(api_key);
  
  // Set default configuration
  config_["model"] = "gemini-pro";
  config_["temperature"] = "0.7";
  config_["max_output_tokens"] = "1024";
  config_["api_key"] = api_key;
}

GeminiServiceProvider::~GeminiServiceProvider() = default;

std::string GeminiServiceProvider::GetProviderId() const {
  return "gemini";
}

std::string GeminiServiceProvider::GetProviderName() const {
  return "Google Gemini";
}

std::string GeminiServiceProvider::GetProviderVersion() const {
  return "1.0";
}

core::AIServiceProvider::Capabilities GeminiServiceProvider::GetCapabilities() const {
  Capabilities capabilities;
  
  capabilities.supports_text_generation = true;
  capabilities.supports_text_summarization = true;
  capabilities.supports_content_analysis = true;
  capabilities.supports_code_generation = true;
  capabilities.supports_question_answering = true;
  capabilities.supports_translation = true;
  capabilities.supports_context = true;
  
  // Gemini Pro doesn't support image analysis in this implementation
  capabilities.supports_image_analysis = false;
  
  // Supported languages
  capabilities.supported_languages = {
    "English", "Spanish", "French", "German", "Chinese", "Japanese",
    "Korean", "Arabic", "Russian", "Portuguese", "Italian"
  };
  
  return capabilities;
}

bool GeminiServiceProvider::SupportsTaskType(TaskType task_type) const {
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

void GeminiServiceProvider::ProcessRequest(
    const AIRequestParams& params,
    AIResponseCallback callback) {
  
  DLOG(INFO) << "Processing request with Gemini provider. Task type: " 
             << static_cast<int>(params.task_type);
  
  // Route to the appropriate handler based on task type
  switch (params.task_type) {
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
    case TaskType::TEXT_GENERATION:
    default:
      ProcessTextGeneration(params, std::move(callback));
      break;
  }
}

void GeminiServiceProvider::Configure(
    const std::unordered_map<std::string, std::string>& config) {
  
  // Update configuration
  for (const auto& pair : config) {
    config_[pair.first] = pair.second;
  }
  
  // Apply configuration to the Gemini adapter
  if (config.find("api_key") != config.end()) {
    gemini_adapter_->SetApiKey(config.at("api_key"));
  }
  
  // Update adapter configuration
  GeminiRequestConfig adapter_config = gemini_adapter_->GetRequestConfig();
  
  if (config.find("model") != config.end()) {
    adapter_config.model_name = config.at("model");
  }
  
  if (config.find("temperature") != config.end()) {
    adapter_config.temperature = std::stof(config.at("temperature"));
  }
  
  if (config.find("max_output_tokens") != config.end()) {
    adapter_config.max_output_tokens = std::stoi(config.at("max_output_tokens"));
  }
  
  if (config.find("top_p") != config.end()) {
    adapter_config.top_p = std::stof(config.at("top_p"));
  }
  
  if (config.find("top_k") != config.end()) {
    adapter_config.top_k = std::stof(config.at("top_k"));
  }
  
  gemini_adapter_->SetRequestConfig(adapter_config);
  
  DLOG(INFO) << "Gemini provider configured with model: " << adapter_config.model_name;
}

std::unordered_map<std::string, std::string> GeminiServiceProvider::GetConfiguration() const {
  return config_;
}

void GeminiServiceProvider::ProcessTextGeneration(
    const AIRequestParams& params,
    AIResponseCallback callback) {
  
  if (params.context_id.empty()) {
    // Simple text generation without context
    gemini_adapter_->ProcessText(
        params.input_text,
        base::BindOnce(&GeminiServiceProvider::OnGeminiResponse,
                      weak_ptr_factory_.GetWeakPtr(),
                      std::move(callback)));
  } else {
    // Text generation with conversation context
    // In a real implementation, we would retrieve the context from a context manager
    // For now, we'll just use the input text
    
    // Create a simple conversation
    std::vector<GeminiMessage> messages;
    
    // Add system message
    GeminiMessage system_message;
    system_message.role = GeminiMessage::Role::SYSTEM;
    system_message.content = CreateSystemPromptForTask(params.task_type, params.custom_params);
    messages.push_back(system_message);
    
    // Add user message
    GeminiMessage user_message;
    user_message.role = GeminiMessage::Role::USER;
    user_message.content = params.input_text;
    messages.push_back(user_message);
    
    // Process the conversation
    gemini_adapter_->ProcessConversation(
        messages,
        base::BindOnce(&GeminiServiceProvider::OnGeminiResponse,
                      weak_ptr_factory_.GetWeakPtr(),
                      std::move(callback)));
  }
}

void GeminiServiceProvider::ProcessTextSummarization(
    const AIRequestParams& params,
    AIResponseCallback callback) {
  
  // Create a prompt for summarization
  std::string prompt = CreateSystemPromptForTask(params.task_type, params.custom_params);
  prompt += "\n\n" + params.input_text;
  
  // Process the summarization request
  gemini_adapter_->ProcessText(
      prompt,
      base::BindOnce(&GeminiServiceProvider::OnGeminiResponse,
                    weak_ptr_factory_.GetWeakPtr(),
                    std::move(callback)));
}

void GeminiServiceProvider::ProcessContentAnalysis(
    const AIRequestParams& params,
    AIResponseCallback callback) {
  
  // Create a prompt for content analysis
  std::string prompt = CreateSystemPromptForTask(params.task_type, params.custom_params);
  prompt += "\n\n" + params.input_text;
  
  // Process the content analysis request
  gemini_adapter_->ProcessText(
      prompt,
      base::BindOnce(&GeminiServiceProvider::OnGeminiResponse,
                    weak_ptr_factory_.GetWeakPtr(),
                    std::move(callback)));
}

void GeminiServiceProvider::ProcessQuestionAnswering(
    const AIRequestParams& params,
    AIResponseCallback callback) {
  
  if (params.context_id.empty()) {
    // Simple question answering without context
    std::string prompt = CreateSystemPromptForTask(params.task_type, params.custom_params);
    prompt += "\n\nQuestion: " + params.input_text;
    
    gemini_adapter_->ProcessText(
        prompt,
        base::BindOnce(&GeminiServiceProvider::OnGeminiResponse,
                      weak_ptr_factory_.GetWeakPtr(),
                      std::move(callback)));
  } else {
    // Question answering with conversation context
    // In a real implementation, we would retrieve the context from a context manager
    // For now, we'll just use the input text
    
    // Create a simple conversation
    std::vector<GeminiMessage> messages;
    
    // Add system message
    GeminiMessage system_message;
    system_message.role = GeminiMessage::Role::SYSTEM;
    system_message.content = CreateSystemPromptForTask(params.task_type, params.custom_params);
    messages.push_back(system_message);
    
    // Add user message
    GeminiMessage user_message;
    user_message.role = GeminiMessage::Role::USER;
    user_message.content = params.input_text;
    messages.push_back(user_message);
    
    // Process the conversation
    gemini_adapter_->ProcessConversation(
        messages,
        base::BindOnce(&GeminiServiceProvider::OnGeminiResponse,
                      weak_ptr_factory_.GetWeakPtr(),
                      std::move(callback)));
  }
}

void GeminiServiceProvider::ProcessCodeGeneration(
    const AIRequestParams& params,
    AIResponseCallback callback) {
  
  // Create a prompt for code generation
  std::string prompt = CreateSystemPromptForTask(params.task_type, params.custom_params);
  prompt += "\n\n" + params.input_text;
  
  // Add language specification if provided
  auto it = params.custom_params.find("language");
  if (it != params.custom_params.end()) {
    prompt += "\n\nPlease write the code in " + it->second + ".";
  }
  
  // Process the code generation request
  gemini_adapter_->ProcessText(
      prompt,
      base::BindOnce(&GeminiServiceProvider::OnGeminiResponse,
                    weak_ptr_factory_.GetWeakPtr(),
                    std::move(callback)));
}

void GeminiServiceProvider::ProcessTranslation(
    const AIRequestParams& params,
    AIResponseCallback callback) {
  
  // Create a prompt for translation
  std::string prompt = CreateSystemPromptForTask(params.task_type, params.custom_params);
  prompt += "\n\n" + params.input_text;
  
  // Process the translation request
  gemini_adapter_->ProcessText(
      prompt,
      base::BindOnce(&GeminiServiceProvider::OnGeminiResponse,
                    weak_ptr_factory_.GetWeakPtr(),
                    std::move(callback)));
}

std::vector<GeminiMessage> GeminiServiceProvider::ConvertToGeminiMessages(
    const std::vector<core::ContextMessage>& context_messages) {
  
  std::vector<GeminiMessage> gemini_messages;
  gemini_messages.reserve(context_messages.size());
  
  for (const auto& message : context_messages) {
    GeminiMessage gemini_message;
    
    // Convert role
    switch (message.role) {
      case core::ContextMessage::Role::USER:
        gemini_message.role = GeminiMessage::Role::USER;
        break;
      case core::ContextMessage::Role::ASSISTANT:
        gemini_message.role = GeminiMessage::Role::MODEL;
        break;
      case core::ContextMessage::Role::SYSTEM:
        gemini_message.role = GeminiMessage::Role::SYSTEM;
        break;
    }
    
    gemini_message.content = message.content;
    gemini_messages.push_back(gemini_message);
  }
  
  return gemini_messages;
}

void GeminiServiceProvider::OnGeminiResponse(
    AIResponseCallback callback,
    bool success,
    const std::string& response) {
  
  // Pass the response directly to the caller
  std::move(callback).Run(success, response);
}

}  // namespace gemini
}  // namespace adapters
}  // namespace asol