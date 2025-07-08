// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASOL_CORE_AI_SERVICE_PROVIDER_H_
#define ASOL_CORE_AI_SERVICE_PROVIDER_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "base/callback.h"

namespace asol {
namespace core {

// AIServiceProvider is the interface for all AI service providers.
// Each provider (like Gemini, OpenAI, etc.) implements this interface.
class AIServiceProvider {
 public:
  // Response callback for AI requests
  using AIResponseCallback = 
      base::OnceCallback<void(bool success, const std::string& response)>;

  // AI task types (same as in AIServiceManager)
  enum class TaskType {
    TEXT_GENERATION,
    TEXT_SUMMARIZATION,
    CONTENT_ANALYSIS,
    IMAGE_ANALYSIS,
    CODE_GENERATION,
    QUESTION_ANSWERING,
    TRANSLATION,
    VOICE_ANALYSIS,
    AUDIO_PROCESSING,
    MULTIMODAL_INTERACTION,
    CUSTOM
  };

  // AI request parameters
  struct AIRequestParams {
    TaskType task_type;
    std::string input_text;
    std::string context_id;  // For maintaining conversation context
    std::unordered_map<std::string, std::string> custom_params;
  };

  // Provider capabilities
  struct Capabilities {
    bool supports_text_generation = false;
    bool supports_text_summarization = false;
    bool supports_content_analysis = false;
    bool supports_image_analysis = false;
    bool supports_code_generation = false;
    bool supports_question_answering = false;
    bool supports_translation = false;
    bool supports_voice_analysis = false;
    bool supports_audio_processing = false;
    bool supports_multimodal_interaction = false;
    bool supports_streaming = false;
    bool supports_context = false;
    std::vector<std::string> supported_languages;
    std::unordered_map<std::string, std::string> custom_capabilities;
  };

  AIServiceProvider();
  virtual ~AIServiceProvider();

  // Get provider information
  virtual std::string GetProviderId() const = 0;
  virtual std::string GetProviderName() const = 0;
  virtual std::string GetProviderVersion() const = 0;
  virtual Capabilities GetCapabilities() const = 0;

  // Check if the provider supports a specific task type
  virtual bool SupportsTaskType(TaskType task_type) const = 0;

  // Process an AI request
  virtual void ProcessRequest(const AIRequestParams& params, 
                            AIResponseCallback callback) = 0;

  // Configure the provider
  virtual void Configure(const std::unordered_map<std::string, std::string>& config) = 0;

  // Get the current configuration
  virtual std::unordered_map<std::string, std::string> GetConfiguration() const = 0;
};

}  // namespace core
}  // namespace asol

#endif  // ASOL_CORE_AI_SERVICE_PROVIDER_H_
