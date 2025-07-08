// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASOL_CORE_AI_SERVICE_MANAGER_H_
#define ASOL_CORE_AI_SERVICE_MANAGER_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "asol/core/ai_service_provider.h"
#include "asol/core/context_manager.h"

namespace asol {
namespace core {

// AIServiceManager is the central coordinator for AI services in the browser.
// It manages different AI service providers and routes requests to the appropriate one.
class AIServiceManager {
 public:
  // Response callback for AI requests
  using AIResponseCallback = 
      base::OnceCallback<void(bool success, const std::string& response)>;

  // AI task types
  enum class TaskType {
    TEXT_GENERATION,
    TEXT_SUMMARIZATION,
    CONTENT_ANALYSIS,
    IMAGE_ANALYSIS,
    CODE_GENERATION,
    QUESTION_ANSWERING,
    TRANSLATION,
    CUSTOM
  };

  // AI request parameters
  struct AIRequestParams {
    TaskType task_type = TaskType::TEXT_GENERATION;
    std::string input_text;
    std::string context_id;  // For maintaining conversation context
    std::string provider_id; // Specific provider to use, or empty for default
    std::unordered_map<std::string, std::string> custom_params;
  };

  AIServiceManager();
  ~AIServiceManager();

  // Disallow copy and assign
  AIServiceManager(const AIServiceManager&) = delete;
  AIServiceManager& operator=(const AIServiceManager&) = delete;

  // Initialize the service manager
  bool Initialize();

  // Register an AI service provider
  void RegisterProvider(std::unique_ptr<AIServiceProvider> provider);

  // Get a provider by ID
  AIServiceProvider* GetProviderById(const std::string& provider_id);

  // Get all registered providers
  std::vector<AIServiceProvider*> GetAllProviders();

  // Get the default provider for a task type
  AIServiceProvider* GetDefaultProviderForTask(TaskType task_type);

  // Set the default provider for a task type
  void SetDefaultProviderForTask(TaskType task_type, const std::string& provider_id);

  // Process an AI request
  void ProcessRequest(const AIRequestParams& params, AIResponseCallback callback);

  // Get the context manager
  ContextManager* GetContextManager();

  // Get a weak pointer to this instance
  base::WeakPtr<AIServiceManager> GetWeakPtr();

 private:
  // Private implementation
  class Impl;
  std::unique_ptr<Impl> impl_;

  // For weak pointers
  base::WeakPtrFactory<AIServiceManager> weak_ptr_factory_{this};
};

}  // namespace core
}  // namespace asol

#endif  // ASOL_CORE_AI_SERVICE_MANAGER_H_