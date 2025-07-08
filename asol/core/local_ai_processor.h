// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASOL_CORE_LOCAL_AI_PROCESSOR_H_
#define ASOL_CORE_LOCAL_AI_PROCESSOR_H_

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "asol/core/ai_service_provider.h"

namespace asol {
namespace core {

// LocalAIProcessor provides AI processing capabilities that run locally
// on the user's device for privacy-sensitive operations.
class LocalAIProcessor : public AIServiceProvider {
 public:
  // Local model types
  enum class ModelType {
    TEXT_SMALL,      // Small text model for basic tasks
    TEXT_MEDIUM,     // Medium text model for more complex tasks
    VISION_SMALL,    // Small vision model for basic image analysis
    EMBEDDING,       // Embedding model for text representations
    CLASSIFICATION,  // Classification model for content categorization
    CUSTOM           // Custom model
  };

  // Model status
  enum class ModelStatus {
    NOT_LOADED,
    LOADING,
    READY,
    ERROR
  };

  // Model info
  struct ModelInfo {
    ModelType type;
    std::string name;
    std::string version;
    size_t size_bytes;
    ModelStatus status;
    std::string error_message;
  };

  LocalAIProcessor();
  ~LocalAIProcessor() override;

  // AIServiceProvider implementation
  std::string GetProviderId() const override;
  std::string GetProviderName() const override;
  std::string GetProviderVersion() const override;
  Capabilities GetCapabilities() const override;
  bool SupportsTaskType(TaskType task_type) const override;
  void ProcessRequest(const AIRequestParams& params, 
                    AIResponseCallback callback) override;
  void Configure(const std::unordered_map<std::string, std::string>& config) override;
  std::unordered_map<std::string, std::string> GetConfiguration() const override;

  // Local AI specific methods
  bool Initialize();

  // Model management
  void LoadModel(ModelType type, 
               base::OnceCallback<void(bool success)> callback);
  
  void UnloadModel(ModelType type);
  
  ModelStatus GetModelStatus(ModelType type) const;
  
  std::vector<ModelInfo> GetLoadedModels() const;

  // Text processing
  void ProcessText(const std::string& text,
                 AIResponseCallback callback);

  // Image processing
  void ProcessImage(const std::vector<uint8_t>& image_data,
                  AIResponseCallback callback);

  // Text embedding
  void GenerateEmbedding(const std::string& text,
                       base::OnceCallback<void(const std::vector<float>&)> callback);

  // Content classification
  void ClassifyContent(const std::string& content,
                     base::OnceCallback<void(const std::unordered_map<std::string, float>&)> callback);

  // Set processing priority (0-10, where 10 is highest)
  void SetProcessingPriority(int priority);
  int GetProcessingPriority() const;

  // Enable/disable local processing
  void Enable(bool enable);
  bool IsEnabled() const;

  // Get a weak pointer to this instance
  base::WeakPtr<LocalAIProcessor> GetWeakPtr();

 private:
  // Helper methods
  void ProcessTextGeneration(const AIRequestParams& params, 
                           AIResponseCallback callback);
  
  void ProcessTextSummarization(const AIRequestParams& params, 
                              AIResponseCallback callback);
  
  void ProcessContentAnalysis(const AIRequestParams& params, 
                            AIResponseCallback callback);
  
  void ProcessClassification(const AIRequestParams& params, 
                           AIResponseCallback callback);

  // Private implementation
  class Impl;
  std::unique_ptr<Impl> impl_;

  // Configuration
  std::unordered_map<std::string, std::string> config_;
  bool is_enabled_ = true;
  int processing_priority_ = 5;

  // For weak pointers
  base::WeakPtrFactory<LocalAIProcessor> weak_ptr_factory_{this};
};

}  // namespace core
}  // namespace asol

#endif  // ASOL_CORE_LOCAL_AI_PROCESSOR_H_