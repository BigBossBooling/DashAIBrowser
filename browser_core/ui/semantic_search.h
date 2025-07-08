// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_CORE_UI_SEMANTIC_SEARCH_H_
#define BROWSER_CORE_UI_SEMANTIC_SEARCH_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "browser_core/engine/web_contents.h"
#include "asol/core/ai_service_manager.h"
#include "browser_core/ai/content_understanding.h"

namespace browser_core {
namespace ui {

// SemanticSearch enhances the traditional "Find on Page" functionality
// with AI-powered semantic understanding, allowing users to find content
// based on meaning rather than just keywords.
class SemanticSearch {
 public:
  // Search match information
  struct SearchMatch {
    std::string text;
    std::string context;
    float relevance_score;
    std::string selector;
    int start_offset;
    int end_offset;
    std::string match_reason;
  };

  // Search result
  struct SearchResult {
    bool success;
    std::string error_message;
    std::vector<SearchMatch> matches;
    std::string suggested_query;
    std::vector<std::string> related_concepts;
  };

  // Callback for search results
  using SearchResultCallback = 
      base::OnceCallback<void(const SearchResult&)>;

  SemanticSearch();
  ~SemanticSearch();

  // Disallow copy and assign
  SemanticSearch(const SemanticSearch&) = delete;
  SemanticSearch& operator=(const SemanticSearch&) = delete;

  // Initialize with AI service manager and content understanding
  bool Initialize(asol::core::AIServiceManager* ai_service_manager,
                ai::ContentUnderstanding* content_understanding);

  // Search for content semantically
  void Search(WebContents* web_contents,
            const std::string& query,
            SearchResultCallback callback);

  // Highlight matches in the page
  void HighlightMatches(WebContents* web_contents,
                      const std::vector<SearchMatch>& matches);

  // Navigate to next/previous match
  void NavigateToNextMatch(WebContents* web_contents);
  void NavigateToPreviousMatch(WebContents* web_contents);

  // Clear highlights
  void ClearHighlights(WebContents* web_contents);

  // Enable/disable semantic search
  void Enable(bool enable);
  bool IsEnabled() const;

  // Get a weak pointer to this instance
  base::WeakPtr<SemanticSearch> GetWeakPtr();

 private:
  // Helper methods
  void ExtractPageContent(WebContents* web_contents,
                        base::OnceCallback<void(const std::string&)> callback);

  void PerformSemanticSearch(const std::string& page_content,
                           const std::string& query,
                           SearchResultCallback callback);

  std::string GenerateSearchPrompt(const std::string& page_content,
                                 const std::string& query);

  SearchResult ParseSearchResponse(const std::string& response);

  // JavaScript for highlighting matches
  static const char* GetHighlightMatchesScript();

  // JavaScript for navigating matches
  static const char* GetNavigateMatchesScript();

  // JavaScript for clearing highlights
  static const char* GetClearHighlightsScript();

  // Components
  asol::core::AIServiceManager* ai_service_manager_ = nullptr;
  ai::ContentUnderstanding* content_understanding_ = nullptr;

  // State
  bool is_enabled_ = true;
  int current_match_index_ = -1;
  std::vector<SearchMatch> current_matches_;

  // For weak pointers
  base::WeakPtrFactory<SemanticSearch> weak_ptr_factory_{this};
};

}  // namespace ui
}  // namespace browser_core

#endif  // BROWSER_CORE_UI_SEMANTIC_SEARCH_H_