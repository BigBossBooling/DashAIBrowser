// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_CORE_UI_PREDICTIVE_OMNIBOX_H_
#define BROWSER_CORE_UI_PREDICTIVE_OMNIBOX_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "browser_core/ai/smart_suggestions.h"
#include "browser_core/ai/content_understanding.h"
#include "browser_core/engine/browser_engine.h"
#include "asol/core/ai_service_manager.h"
#include "asol/core/context_manager.h"

namespace browser_core {
namespace ui {

// PredictiveOmnibox enhances the address bar with AI-powered suggestions
// based on user input, current page context, and browsing history.
class PredictiveOmnibox {
 public:
  // Action suggestion types
  enum class ActionType {
    SUMMARIZE,        // Summarize the current page
    TRANSLATE,        // Translate the current page
    FIND_SIMILAR,     // Find similar content
    RESEARCH,         // Research a topic further
    SHOP_COMPARE,     // Compare shopping options
    SAVE_FOR_LATER,   // Save content for later reading
    SHARE,            // Share the current page
    ANALYZE           // Analyze the content
  };

  // Action suggestion
  struct ActionSuggestion {
    ActionType type;
    std::string display_text;
    std::string description;
    std::string icon_name;
    float relevance_score;
  };

  // Predictive suggestion
  struct PredictiveSuggestion {
    std::string text;
    std::string url;
    std::string description;
    float relevance_score;
    bool is_search_query;
    bool is_navigation;
    bool is_action;
    ActionSuggestion action;
  };

  // Suggestions result
  struct OmniboxSuggestions {
    bool success;
    std::vector<PredictiveSuggestion> suggestions;
    std::string error_message;
  };

  // Callback for suggestions
  using SuggestionsCallback = 
      base::OnceCallback<void(const OmniboxSuggestions&)>;

  // Callback for action execution
  using ActionExecutedCallback = 
      base::OnceCallback<void(bool success, const std::string& result)>;

  PredictiveOmnibox();
  ~PredictiveOmnibox();

  // Disallow copy and assign
  PredictiveOmnibox(const PredictiveOmnibox&) = delete;
  PredictiveOmnibox& operator=(const PredictiveOmnibox&) = delete;

  // Initialize with browser engine, AI service manager, and smart suggestions
  bool Initialize(BrowserEngine* browser_engine, 
                asol::core::AIServiceManager* ai_service_manager,
                asol::core::ContextManager* context_manager,
                ai::SmartSuggestions* smart_suggestions,
                ai::ContentUnderstanding* content_understanding);

  // Get suggestions based on user input and current context
  void GetSuggestions(const std::string& input, 
                    int current_tab_id,
                    SuggestionsCallback callback);

  // Execute an action suggestion
  void ExecuteAction(const ActionSuggestion& action,
                   int tab_id,
                   ActionExecutedCallback callback);

  // Enable/disable predictive features
  void Enable(bool enable);
  bool IsEnabled() const;

  // Get a weak pointer to this instance
  base::WeakPtr<PredictiveOmnibox> GetWeakPtr();

 private:
  // Helper methods
  void GenerateContextAwareSuggestions(const std::string& input,
                                     int tab_id,
                                     SuggestionsCallback callback);

  void GenerateActionSuggestions(int tab_id,
                               std::vector<ActionSuggestion>* actions);

  void MergeSuggestions(std::vector<PredictiveSuggestion>* merged_suggestions,
                      const std::vector<ai::SmartSuggestions::Suggestion>& smart_suggestions,
                      const std::vector<ActionSuggestion>& action_suggestions);

  void RankSuggestions(std::vector<PredictiveSuggestion>* suggestions);

  // Components
  BrowserEngine* browser_engine_ = nullptr;
  asol::core::AIServiceManager* ai_service_manager_ = nullptr;
  asol::core::ContextManager* context_manager_ = nullptr;
  ai::SmartSuggestions* smart_suggestions_ = nullptr;
  ai::ContentUnderstanding* content_understanding_ = nullptr;

  // State
  bool is_enabled_ = true;

  // For weak pointers
  base::WeakPtrFactory<PredictiveOmnibox> weak_ptr_factory_{this};
};

}  // namespace ui
}  // namespace browser_core

#endif  // BROWSER_CORE_UI_PREDICTIVE_OMNIBOX_H_