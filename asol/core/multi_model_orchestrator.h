// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASOL_CORE_MULTI_MODEL_ORCHESTRATOR_H_
#define ASOL_CORE_MULTI_MODEL_ORCHESTRATOR_H_

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "asol/core/ai_service_manager.h"
#include "asol/core/ai_service_provider.h"

namespace asol {
namespace core {

// MultiModelOrchestrator manages multiple AI models and selects the best one for each task.
class MultiModelOrchestrator {
 public:
  // Model selection strategy
  enum class SelectionStrategy {
    BEST_PERFORMANCE,  // Select the model with the best performance for the task
    LOWEST_LATENCY,    // Select the model with the lowest latency
    LOWEST_COST,       // Select the model with the lowest cost
    HIGHEST_QUALITY,   // Select the model with the highest quality results
    BALANCED,          // Balance performance, latency, cost, and quality
    CUSTOM            // Custom selection logic
  };

  // Model performance metrics
  struct ModelMetrics {
    std::string provider_id;
    AIServiceManager::TaskType task_type;
    float success_rate;
    float average_latency_ms;
    float cost_per_request;
    float quality_score;
    int request_count;
    base::Time last_updated;
  };

  // Model selection result
  struct ModelSelectionResult {
    std::string selected_provider_id;
    std::vector<std::string> fallback_provider_ids;
    std::string selection_reason;
  };

  MultiModelOrchestrator();
  ~MultiModelOrchestrator();

  // Disallow copy and assign
  MultiModelOrchestrator(const MultiModelOrchestrator&) = delete;
  MultiModelOrchestrator& operator=(const MultiModelOrchestrator&) = delete;

  // Initialize with AI service manager
  bool Initialize(AIServiceManager* ai_service_manager);

  // Select the best model for a task
  ModelSelectionResult SelectModelForTask(AIServiceManager::TaskType task_type);

  // Process a request with automatic model selection
  void ProcessRequest(const AIServiceManager::AIRequestParams& params,
                    AIServiceManager::AIResponseCallback callback);

  // Process a request with fallback support
  void ProcessRequestWithFallback(const AIServiceManager::AIRequestParams& params,
                                AIServiceManager::AIResponseCallback callback);

  // Update model metrics
  void UpdateModelMetrics(const std::string& provider_id,
                        AIServiceManager::TaskType task_type,
                        bool success,
                        float latency_ms,
                        float quality_score);

  // Get metrics for a model
  ModelMetrics GetModelMetrics(const std::string& provider_id,
                             AIServiceManager::TaskType task_type) const;

  // Get all model metrics
  std::vector<ModelMetrics> GetAllModelMetrics() const;

  // Set model selection strategy
  void SetSelectionStrategy(SelectionStrategy strategy);
  SelectionStrategy GetSelectionStrategy() const;

  // Set custom model selection function
  using CustomModelSelectionFunction = 
      base::RepeatingCallback<std::string(AIServiceManager::TaskType,
                                        const std::vector<ModelMetrics>&)>;
  
  void SetCustomSelectionFunction(CustomModelSelectionFunction function);

  // Enable/disable automatic model selection
  void EnableAutoSelection(bool enable);
  bool IsAutoSelectionEnabled() const;

  // Get a weak pointer to this instance
  base::WeakPtr<MultiModelOrchestrator> GetWeakPtr();

 private:
  // Helper methods
  void OnRequestProcessed(AIServiceManager::AIResponseCallback callback,
                        const std::string& provider_id,
                        base::TimeTicks start_time,
                        bool success,
                        const std::string& response);
  
  void TryFallbackProvider(const AIServiceManager::AIRequestParams& params,
                         const std::vector<std::string>& fallback_providers,
                         size_t current_index,
                         AIServiceManager::AIResponseCallback callback);
  
  float CalculateQualityScore(const std::string& response);

  // AI service manager
  AIServiceManager* ai_service_manager_ = nullptr;

  // Model metrics
  std::unordered_map<std::string, std::unordered_map<AIServiceManager::TaskType, ModelMetrics>> model_metrics_;

  // Selection strategy
  SelectionStrategy selection_strategy_ = SelectionStrategy::BALANCED;
  CustomModelSelectionFunction custom_selection_function_;
  bool auto_selection_enabled_ = true;

  // For weak pointers
  base::WeakPtrFactory<MultiModelOrchestrator> weak_ptr_factory_{this};
};

}  // namespace core
}  // namespace asol

#endif  // ASOL_CORE_MULTI_MODEL_ORCHESTRATOR_H_