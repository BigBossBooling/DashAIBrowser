// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_CORE_AI_SMART_SUGGESTIONS_H_
#define BROWSER_CORE_AI_SMART_SUGGESTIONS_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "asol/core/ai_service_manager.h"
#include "browser_core/engine/browser_engine.h"
#include "browser_core/ai/content_understanding.h"

namespace browser_core {
namespace ai {

// SmartSuggestions provides intelligent suggestions based on browsing context.
class SmartSuggestions {
 public:
  // Suggestion types
  enum class SuggestionType {
    SEARCH_QUERY,       // Suggested search queries
    RELATED_CONTENT,    // Related articles, videos, etc.
    NAVIGATION,         // Suggested websites to visit
    ACTION,             // Suggested browser actions
    RESEARCH,           // Research suggestions
    LEARNING,           // Learning resources
    PRODUCTIVITY        // Productivity suggestions
  };

  // Suggestion item
  struct Suggestion {
    std::string text;
    std::string description;
    std::string url;
    SuggestionType type;
    float relevance_score;
    std::string icon_url;
  };

  // Suggestions result
  struct SuggestionsResult {
    bool success;
    std::vector<Suggestion> suggestions;
    std::string error_message;
  };

  // Callback for suggestions
  using SuggestionsCallback = 
      base::OnceCallback<void(const SuggestionsResult&)>;

  SmartSuggestions();
  ~SmartSuggestions();

  // Disallow copy and assign
  SmartSuggestions(const SmartSuggestions&) = delete;
  SmartSuggestions& operator=(const SmartSuggestions&) = delete;

  // Initialize with browser engine, AI service manager, and content understanding
  bool Initialize(BrowserEngine* browser_engine, 
                asol::core::AIServiceManager* ai_service_manager,
                ContentUnderstanding* content_understanding);

  // Get suggestions based on current page
  void GetSuggestionsForCurrentPage(int tab_id, 
                                  SuggestionsCallback callback);

  // Get suggestions based on search query
  void GetSuggestionsForQuery(const std::string& query, 
                            SuggestionsCallback callback);

  // Get suggestions based on browsing history
  void GetSuggestionsFromHistory(SuggestionsCallback callback);

  // Get suggestions for research on a topic
  void GetResearchSuggestions(const std::string& topic, 
                            SuggestionsCallback callback);

  // Get learning resource suggestions
  void GetLearningSuggestions(const std::string& topic, 
                            SuggestionsCallback callback);

  // Get productivity suggestions
  void GetProductivitySuggestions(SuggestionsCallback callback);

  // Enable/disable suggestions
  void Enable(bool enable);
  bool IsEnabled() const;

  // Get a weak pointer to this instance
  base::WeakPtr<SmartSuggestions> GetWeakPtr();

 private:
  // Helper methods
  void AnalyzePageForSuggestions(int tab_id, 
                               SuggestionsCallback callback);
  
  void GenerateSuggestionsFromTopics(const std::vector<ContentUnderstanding::Topic>& topics,
                                   SuggestionsCallback callback);
  
  void GenerateSuggestionsFromEntities(const std::vector<ContentUnderstanding::Entity>& entities,
                                     SuggestionsCallback callback);
  
  void ParseSuggestionsResponse(const std::string& response, 
                              SuggestionType type,
                              std::vector<Suggestion>* suggestions);

  // Browser engine, AI service manager, and content understanding
  BrowserEngine* browser_engine_ = nullptr;
  asol::core::AIServiceManager* ai_service_manager_ = nullptr;
  ContentUnderstanding* content_understanding_ = nullptr;

  // State
  bool is_enabled_ = true;

  // For weak pointers
  base::WeakPtrFactory<SmartSuggestions> weak_ptr_factory_{this};
};

}  // namespace ai
}  // namespace browser_core

#endif  // BROWSER_CORE_AI_SMART_SUGGESTIONS_H_