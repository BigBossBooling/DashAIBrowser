// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_CORE_AI_BROWSER_AI_INTEGRATION_H_
#define BROWSER_CORE_AI_BROWSER_AI_INTEGRATION_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "asol/core/ai_service_manager.h"
#include "browser_core/engine/browser_engine.h"
#include "browser_core/engine/tab.h"
#include "browser_core/engine/web_contents.h"

namespace browser_core {
namespace ai {

// BrowserAIIntegration connects the browser engine with AI services.
class BrowserAIIntegration {
 public:
  // AI feature types
  enum class FeatureType {
    PAGE_SUMMARIZATION,
    CONTENT_ANALYSIS,
    SMART_SEARCH,
    QUESTION_ANSWERING,
    CODE_ASSISTANCE,
    TRANSLATION,
    WRITING_ASSISTANCE,
    RESEARCH_ASSISTANT
  };

  // AI feature result
  struct FeatureResult {
    bool success = false;
    std::string result;
    std::string error_message;
  };

  // Callback for AI feature results
  using FeatureResultCallback = 
      base::OnceCallback<void(const FeatureResult&)>;

  BrowserAIIntegration();
  ~BrowserAIIntegration();

  // Disallow copy and assign
  BrowserAIIntegration(const BrowserAIIntegration&) = delete;
  BrowserAIIntegration& operator=(const BrowserAIIntegration&) = delete;

  // Initialize with browser engine and AI service manager
  bool Initialize(BrowserEngine* browser_engine, 
                asol::core::AIServiceManager* ai_service_manager);

  // Page summarization
  void SummarizePage(int tab_id, FeatureResultCallback callback);

  // Content analysis
  void AnalyzePageContent(int tab_id, FeatureResultCallback callback);

  // Smart search
  void PerformSmartSearch(const std::string& query, 
                        FeatureResultCallback callback);

  // Question answering
  void AnswerQuestion(const std::string& question, 
                    int tab_id,
                    FeatureResultCallback callback);

  // Code assistance
  void ProvideCodeAssistance(const std::string& code, 
                           const std::string& language,
                           FeatureResultCallback callback);

  // Translation
  void TranslateContent(const std::string& content, 
                      const std::string& target_language,
                      FeatureResultCallback callback);

  // Writing assistance
  void ProvideWritingAssistance(const std::string& text, 
                              const std::string& task,
                              FeatureResultCallback callback);

  // Research assistant
  void ResearchTopic(const std::string& topic, 
                   FeatureResultCallback callback);

  // Enable/disable AI features
  void EnableFeature(FeatureType feature, bool enable);
  bool IsFeatureEnabled(FeatureType feature) const;

  // Get a weak pointer to this instance
  base::WeakPtr<BrowserAIIntegration> GetWeakPtr();

 private:
  // Helper methods
  void ExtractPageContent(int tab_id, 
                        base::OnceCallback<void(const std::string&)> callback);
  
  void OnAIResponse(FeatureResultCallback callback,
                  bool success,
                  const std::string& response);

  // Browser engine and AI service manager
  BrowserEngine* browser_engine_ = nullptr;
  asol::core::AIServiceManager* ai_service_manager_ = nullptr;

  // Enabled features
  std::unordered_map<FeatureType, bool> enabled_features_;

  // For weak pointers
  base::WeakPtrFactory<BrowserAIIntegration> weak_ptr_factory_{this};
};

}  // namespace ai
}  // namespace browser_core

#endif  // BROWSER_CORE_AI_BROWSER_AI_INTEGRATION_H_