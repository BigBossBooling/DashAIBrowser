// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser_core/ui/semantic_search.h"

#include <sstream>
#include <algorithm>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_util.h"
#include "base/values.h"

namespace browser_core {
namespace ui {

namespace {

// Constants for AI prompts
constexpr char kSemanticSearchPrompt[] = 
    "Search the following web page content for information related to the query: \"{query}\". "
    "Find content that is semantically relevant to the query, even if it doesn't contain the exact keywords. "
    "Consider synonyms, related concepts, and contextual meaning. "
    "\n\nPage content:\n{page_content}\n\n"
    "Format response as JSON with the following fields: "
    "matches (array of objects with text, context, relevance_score, selector, start_offset, end_offset, match_reason), "
    "suggested_query (string), related_concepts (array of strings).";

// JavaScript for extracting page content
constexpr char kExtractPageContentScript[] = R"(
  (function() {
    // Extract main content
    const content = {
      title: document.title,
      url: window.location.href,
      text: document.body.innerText,
      elements: []
    };
    
    // Extract text elements with their selectors
    const textElements = document.querySelectorAll('p, h1, h2, h3, h4, h5, h6, li, td, th, div:not(:has(*))');
    for (let i = 0; i < textElements.length; i++) {
      const el = textElements[i];
      if (el.innerText.trim().length > 0) {
        // Create a unique selector for the element
        let selector = '';
        if (el.id) {
          selector = '#' + el.id;
        } else if (el.className && typeof el.className === 'string') {
          selector = el.tagName.toLowerCase() + '.' + 
                    el.className.trim().replace(/\s+/g, '.');
        } else {
          // Create a path selector
          let path = [];
          let currentEl = el;
          while (currentEl && currentEl.tagName !== 'HTML') {
            let selector = currentEl.tagName.toLowerCase();
            if (currentEl.id) {
              selector += '#' + currentEl.id;
              path.unshift(selector);
              break;
            } else if (currentEl.className && typeof currentEl.className === 'string') {
              selector += '.' + currentEl.className.trim().replace(/\s+/g, '.');
            }
            
            // Add nth-child if needed
            if (!currentEl.id) {
              let siblings = 1;
              let sibling = currentEl;
              while (sibling = sibling.previousElementSibling) {
                siblings++;
              }
              if (siblings > 1) {
                selector += ':nth-child(' + siblings + ')';
              }
            }
            
            path.unshift(selector);
            currentEl = currentEl.parentElement;
          }
          selector = path.join(' > ');
        }
        
        content.elements.push({
          text: el.innerText.trim(),
          selector: selector,
          tag: el.tagName.toLowerCase()
        });
      }
    }
    
    return JSON.stringify(content);
  })();
)";

// JavaScript for highlighting matches
constexpr char kHighlightMatchesScript[] = R"(
  (function(matches) {
    // Remove existing highlights
    const existingHighlights = document.querySelectorAll('.dashai-semantic-highlight');
    for (let i = 0; i < existingHighlights.length; i++) {
      const highlight = existingHighlights[i];
      const parent = highlight.parentNode;
      parent.replaceChild(document.createTextNode(highlight.textContent), highlight);
      parent.normalize();
    }
    
    // Create highlights for each match
    const highlightedMatches = [];
    for (let i = 0; i < matches.length; i++) {
      const match = matches[i];
      const elements = document.querySelectorAll(match.selector);
      
      for (let j = 0; j < elements.length; j++) {
        const element = elements[j];
        const text = element.textContent;
        
        // Find the match text in the element
        const matchText = match.text;
        const matchIndex = text.indexOf(matchText);
        
        if (matchIndex >= 0) {
          // Create a highlight span
          const highlightSpan = document.createElement('span');
          highlightSpan.className = 'dashai-semantic-highlight';
          highlightSpan.dataset.matchIndex = i;
          highlightSpan.style.backgroundColor = 'rgba(255, 255, 0, 0.3)';
          highlightSpan.style.color = 'inherit';
          highlightSpan.style.borderRadius = '2px';
          highlightSpan.textContent = matchText;
          
          // Replace the text with the highlight
          const range = document.createRange();
          const textNode = Array.from(element.childNodes).find(node => 
            node.nodeType === Node.TEXT_NODE && node.textContent.includes(matchText)
          );
          
          if (textNode) {
            const nodeText = textNode.textContent;
            const nodeMatchIndex = nodeText.indexOf(matchText);
            
            if (nodeMatchIndex >= 0) {
              // Split the text node and insert the highlight
              const beforeText = nodeText.substring(0, nodeMatchIndex);
              const afterText = nodeText.substring(nodeMatchIndex + matchText.length);
              
              const beforeNode = document.createTextNode(beforeText);
              const afterNode = document.createTextNode(afterText);
              
              element.replaceChild(afterNode, textNode);
              element.insertBefore(highlightSpan, afterNode);
              element.insertBefore(beforeNode, highlightSpan);
              
              highlightedMatches.push({
                element: highlightSpan,
                matchIndex: i,
                rect: highlightSpan.getBoundingClientRect()
              });
            }
          }
        }
      }
    }
    
    // Store the highlighted matches in a global variable
    window.dashai_semantic_matches = highlightedMatches;
    
    return highlightedMatches.length;
  })(arguments[0]);
)";

// JavaScript for navigating matches
constexpr char kNavigateMatchesScript[] = R"(
  (function(direction) {
    if (!window.dashai_semantic_matches || window.dashai_semantic_matches.length === 0) {
      return -1;
    }
    
    // Get the current match index
    let currentIndex = window.dashai_current_match_index || -1;
    
    // Calculate the next index
    if (direction === 'next') {
      currentIndex = (currentIndex + 1) % window.dashai_semantic_matches.length;
    } else {
      currentIndex = (currentIndex - 1 + window.dashai_semantic_matches.length) % window.dashai_semantic_matches.length;
    }
    
    // Update the current index
    window.dashai_current_match_index = currentIndex;
    
    // Get the current match
    const match = window.dashai_semantic_matches[currentIndex];
    
    // Update highlight styles
    for (let i = 0; i < window.dashai_semantic_matches.length; i++) {
      const m = window.dashai_semantic_matches[i];
      if (i === currentIndex) {
        m.element.style.backgroundColor = 'rgba(255, 165, 0, 0.5)';
        m.element.style.outline = '2px solid orange';
      } else {
        m.element.style.backgroundColor = 'rgba(255, 255, 0, 0.3)';
        m.element.style.outline = 'none';
      }
    }
    
    // Scroll to the match
    match.element.scrollIntoView({
      behavior: 'smooth',
      block: 'center'
    });
    
    return currentIndex;
  })(arguments[0]);
)";

// JavaScript for clearing highlights
constexpr char kClearHighlightsScript[] = R"(
  (function() {
    // Remove existing highlights
    const existingHighlights = document.querySelectorAll('.dashai-semantic-highlight');
    for (let i = 0; i < existingHighlights.length; i++) {
      const highlight = existingHighlights[i];
      const parent = highlight.parentNode;
      parent.replaceChild(document.createTextNode(highlight.textContent), highlight);
      parent.normalize();
    }
    
    // Clear the global variables
    window.dashai_semantic_matches = null;
    window.dashai_current_match_index = -1;
    
    return true;
  })();
)";

}  // namespace

SemanticSearch::SemanticSearch() = default;
SemanticSearch::~SemanticSearch() = default;

bool SemanticSearch::Initialize(
    asol::core::AIServiceManager* ai_service_manager,
    ai::ContentUnderstanding* content_understanding) {
  if (!ai_service_manager || !content_understanding) {
    return false;
  }

  ai_service_manager_ = ai_service_manager;
  content_understanding_ = content_understanding;
  
  return true;
}

void SemanticSearch::Search(
    WebContents* web_contents,
    const std::string& query,
    SearchResultCallback callback) {
  if (!is_enabled_ || !web_contents) {
    SearchResult empty_result;
    empty_result.success = false;
    empty_result.error_message = "Semantic search is disabled or web contents is null";
    std::move(callback).Run(empty_result);
    return;
  }

  // Extract page content
  ExtractPageContent(web_contents, 
      base::BindOnce([](
          SemanticSearch* self,
          const std::string& query,
          SearchResultCallback callback,
          const std::string& page_content) {
        // Perform semantic search
        self->PerformSemanticSearch(page_content, query, std::move(callback));
      }, this, query, std::move(callback)));
}

void SemanticSearch::ExtractPageContent(
    WebContents* web_contents,
    base::OnceCallback<void(const std::string&)> callback) {
  // Execute JavaScript to extract page content
  web_contents->ExecuteJavaScript(
      kExtractPageContentScript,
      base::BindOnce([](
          base::OnceCallback<void(const std::string&)> callback,
          const WebContents::JavaScriptResult& result) {
        if (!result.success) {
          std::move(callback).Run("{}");
          return;
        }
        
        std::move(callback).Run(result.result);
      }, std::move(callback)));
}

void SemanticSearch::PerformSemanticSearch(
    const std::string& page_content,
    const std::string& query,
    SearchResultCallback callback) {
  // Generate AI prompt for semantic search
  std::string prompt = GenerateSearchPrompt(page_content, query);
  
  // Request AI analysis
  ai_service_manager_->GetTextAdapter()->GenerateText(
      prompt,
      base::BindOnce([](
          SemanticSearch* self,
          SearchResultCallback callback,
          const asol::core::TextAdapter::GenerateTextResult& result) {
        if (!result.success) {
          SearchResult error_result;
          error_result.success = false;
          error_result.error_message = "Failed to generate AI analysis: " + result.error_message;
          std::move(callback).Run(error_result);
          return;
        }
        
        // Parse AI response
        SearchResult search_result = self->ParseSearchResponse(result.text);
        
        // Store current matches
        self->current_matches_ = search_result.matches;
        self->current_match_index_ = -1;
        
        std::move(callback).Run(search_result);
      }, this, std::move(callback)));
}

std::string SemanticSearch::GenerateSearchPrompt(
    const std::string& page_content,
    const std::string& query) {
  std::string prompt = kSemanticSearchPrompt;
  
  // Format query
  base::ReplaceSubstringsAfterOffset(&prompt, 0, "{query}", query);
  
  // Format page content (truncate if too long)
  std::string truncated_content = page_content;
  if (truncated_content.length() > 10000) {
    truncated_content = truncated_content.substr(0, 10000) + "... [content truncated]";
  }
  base::ReplaceSubstringsAfterOffset(&prompt, 0, "{page_content}", truncated_content);
  
  return prompt;
}

SemanticSearch::SearchResult SemanticSearch::ParseSearchResponse(
    const std::string& response) {
  SearchResult result;
  result.success = false;
  
  // Parse the JSON response
  absl::optional<base::Value> json = base::JSONReader::Read(response);
  if (!json || !json->is_dict()) {
    result.error_message = "Failed to parse AI response as JSON";
    return result;
  }
  
  const base::Value::Dict& dict = json->GetDict();
  
  // Extract matches
  const base::Value::List* matches_list = dict.FindList("matches");
  if (matches_list) {
    for (const auto& match : *matches_list) {
      if (!match.is_dict()) continue;
      
      const base::Value::Dict& match_dict = match.GetDict();
      
      SearchMatch search_match;
      search_match.text = match_dict.FindString("text").value_or("");
      search_match.context = match_dict.FindString("context").value_or("");
      search_match.relevance_score = match_dict.FindDouble("relevance_score").value_or(0.0);
      search_match.selector = match_dict.FindString("selector").value_or("");
      search_match.start_offset = match_dict.FindInt("start_offset").value_or(0);
      search_match.end_offset = match_dict.FindInt("end_offset").value_or(0);
      search_match.match_reason = match_dict.FindString("match_reason").value_or("");
      
      if (!search_match.text.empty() && !search_match.selector.empty()) {
        result.matches.push_back(search_match);
      }
    }
  }
  
  // Extract suggested query
  result.suggested_query = dict.FindString("suggested_query").value_or("");
  
  // Extract related concepts
  const base::Value::List* concepts_list = dict.FindList("related_concepts");
  if (concepts_list) {
    for (const auto& concept : *concepts_list) {
      if (concept.is_string()) {
        result.related_concepts.push_back(concept.GetString());
      }
    }
  }
  
  result.success = true;
  return result;
}

void SemanticSearch::HighlightMatches(
    WebContents* web_contents,
    const std::vector<SearchMatch>& matches) {
  if (!is_enabled_ || !web_contents || matches.empty()) {
    return;
  }
  
  // Convert matches to JSON for JavaScript
  base::Value::List matches_list;
  for (const auto& match : matches) {
    base::Value::Dict match_dict;
    match_dict.Set("text", match.text);
    match_dict.Set("selector", match.selector);
    match_dict.Set("relevance_score", match.relevance_score);
    matches_list.Append(std::move(match_dict));
  }
  
  // Execute JavaScript to highlight matches
  web_contents->ExecuteJavaScript(
      GetHighlightMatchesScript(),
      base::BindOnce([](
          SemanticSearch* self,
          const WebContents::JavaScriptResult& result) {
        if (result.success) {
          // Reset current match index
          self->current_match_index_ = -1;
        }
      }, this));
}

void SemanticSearch::NavigateToNextMatch(WebContents* web_contents) {
  if (!is_enabled_ || !web_contents || current_matches_.empty()) {
    return;
  }
  
  // Execute JavaScript to navigate to next match
  web_contents->ExecuteJavaScript(
      GetNavigateMatchesScript(),
      base::BindOnce([](
          SemanticSearch* self,
          const WebContents::JavaScriptResult& result) {
        if (result.success) {
          // Update current match index
          self->current_match_index_ = std::stoi(result.result);
        }
      }, this));
}

void SemanticSearch::NavigateToPreviousMatch(WebContents* web_contents) {
  if (!is_enabled_ || !web_contents || current_matches_.empty()) {
    return;
  }
  
  // Execute JavaScript to navigate to previous match
  web_contents->ExecuteJavaScript(
      GetNavigateMatchesScript(),
      base::BindOnce([](
          SemanticSearch* self,
          const WebContents::JavaScriptResult& result) {
        if (result.success) {
          // Update current match index
          self->current_match_index_ = std::stoi(result.result);
        }
      }, this));
}

void SemanticSearch::ClearHighlights(WebContents* web_contents) {
  if (!web_contents) {
    return;
  }
  
  // Execute JavaScript to clear highlights
  web_contents->ExecuteJavaScript(
      GetClearHighlightsScript(),
      base::BindOnce([](
          SemanticSearch* self,
          const WebContents::JavaScriptResult& result) {
        if (result.success) {
          // Clear current matches
          self->current_matches_.clear();
          self->current_match_index_ = -1;
        }
      }, this));
}

void SemanticSearch::Enable(bool enable) {
  is_enabled_ = enable;
}

bool SemanticSearch::IsEnabled() const {
  return is_enabled_;
}

const char* SemanticSearch::GetHighlightMatchesScript() {
  return kHighlightMatchesScript;
}

const char* SemanticSearch::GetNavigateMatchesScript() {
  return kNavigateMatchesScript;
}

const char* SemanticSearch::GetClearHighlightsScript() {
  return kClearHighlightsScript;
}

base::WeakPtr<SemanticSearch> SemanticSearch::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

}  // namespace ui
}  // namespace browser_core