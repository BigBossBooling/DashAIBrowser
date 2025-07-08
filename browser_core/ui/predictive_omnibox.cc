// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser_core/ui/predictive_omnibox.h"

#include <algorithm>
#include <sstream>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "browser_core/engine/tab.h"
#include "browser_core/engine/web_contents.h"

namespace browser_core {
namespace ui {

namespace {

// Constants for AI prompts
constexpr char kSuggestionPrompt[] = 
    "Based on the user's input '{input}' and the current page context, "
    "generate relevant suggestions. Current page title: {title}, URL: {url}. "
    "Page topics: {topics}. User's recent browsing context: {context}. "
    "Format response as JSON with fields: suggestions (array of objects with "
    "text, url, description, relevance_score, is_search_query, is_navigation).";

constexpr char kActionPrompt[] =
    "Based on the current page content, suggest relevant actions the user might want "
    "to take. Current page title: {title}, URL: {url}. Page topics: {topics}. "
    "Format response as JSON with fields: actions (array of objects with "
    "type, display_text, description, relevance_score).";

// Helper function to create an action suggestion
ActionSuggestion CreateActionSuggestion(
    PredictiveOmnibox::ActionType type,
    const std::string& display_text,
    const std::string& description,
    const std::string& icon_name,
    float relevance_score) {
  ActionSuggestion action;
  action.type = type;
  action.display_text = display_text;
  action.description = description;
  action.icon_name = icon_name;
  action.relevance_score = relevance_score;
  return action;
}

// Helper function to parse action type from string
PredictiveOmnibox::ActionType ParseActionType(const std::string& type_str) {
  if (type_str == "SUMMARIZE") return PredictiveOmnibox::ActionType::SUMMARIZE;
  if (type_str == "TRANSLATE") return PredictiveOmnibox::ActionType::TRANSLATE;
  if (type_str == "FIND_SIMILAR") return PredictiveOmnibox::ActionType::FIND_SIMILAR;
  if (type_str == "RESEARCH") return PredictiveOmnibox::ActionType::RESEARCH;
  if (type_str == "SHOP_COMPARE") return PredictiveOmnibox::ActionType::SHOP_COMPARE;
  if (type_str == "SAVE_FOR_LATER") return PredictiveOmnibox::ActionType::SAVE_FOR_LATER;
  if (type_str == "SHARE") return PredictiveOmnibox::ActionType::SHARE;
  if (type_str == "ANALYZE") return PredictiveOmnibox::ActionType::ANALYZE;
  
  // Default
  return PredictiveOmnibox::ActionType::SUMMARIZE;
}

// Helper function to get action type string
std::string GetActionTypeString(PredictiveOmnibox::ActionType type) {
  switch (type) {
    case PredictiveOmnibox::ActionType::SUMMARIZE: return "SUMMARIZE";
    case PredictiveOmnibox::ActionType::TRANSLATE: return "TRANSLATE";
    case PredictiveOmnibox::ActionType::FIND_SIMILAR: return "FIND_SIMILAR";
    case PredictiveOmnibox::ActionType::RESEARCH: return "RESEARCH";
    case PredictiveOmnibox::ActionType::SHOP_COMPARE: return "SHOP_COMPARE";
    case PredictiveOmnibox::ActionType::SAVE_FOR_LATER: return "SAVE_FOR_LATER";
    case PredictiveOmnibox::ActionType::SHARE: return "SHARE";
    case PredictiveOmnibox::ActionType::ANALYZE: return "ANALYZE";
    default: return "UNKNOWN";
  }
}

}  // namespace

PredictiveOmnibox::PredictiveOmnibox() = default;
PredictiveOmnibox::~PredictiveOmnibox() = default;

bool PredictiveOmnibox::Initialize(
    BrowserEngine* browser_engine,
    asol::core::AIServiceManager* ai_service_manager,
    asol::core::ContextManager* context_manager,
    ai::SmartSuggestions* smart_suggestions,
    ai::ContentUnderstanding* content_understanding) {
  if (!browser_engine || !ai_service_manager || !context_manager || 
      !smart_suggestions || !content_understanding) {
    return false;
  }

  browser_engine_ = browser_engine;
  ai_service_manager_ = ai_service_manager;
  context_manager_ = context_manager;
  smart_suggestions_ = smart_suggestions;
  content_understanding_ = content_understanding;
  return true;
}

void PredictiveOmnibox::GetSuggestions(
    const std::string& input,
    int current_tab_id,
    SuggestionsCallback callback) {
  if (!is_enabled_ || input.empty()) {
    OmniboxSuggestions empty_result;
    empty_result.success = false;
    empty_result.error_message = "Predictive features are disabled or input is empty";
    std::move(callback).Run(empty_result);
    return;
  }

  // Generate context-aware suggestions
  GenerateContextAwareSuggestions(input, current_tab_id, std::move(callback));
}

void PredictiveOmnibox::GenerateContextAwareSuggestions(
    const std::string& input,
    int tab_id,
    SuggestionsCallback callback) {
  // Get the current tab
  Tab* tab = browser_engine_->GetTabById(tab_id);
  if (!tab) {
    OmniboxSuggestions error_result;
    error_result.success = false;
    error_result.error_message = "Tab not found";
    std::move(callback).Run(error_result);
    return;
  }

  // Get page information
  std::string page_url = tab->GetURL();
  std::string page_title = tab->GetTitle();

  // Get content understanding for the current page
  content_understanding_->AnalyzeContent(
      tab_id,
      base::BindOnce([](
          PredictiveOmnibox* self,
          const std::string& input,
          int tab_id,
          std::string page_url,
          std::string page_title,
          SuggestionsCallback callback,
          const ai::ContentUnderstanding::AnalysisResult& analysis_result) {
        if (!analysis_result.success) {
          // Fall back to smart suggestions without content understanding
          self->smart_suggestions_->GetSuggestionsForQuery(
              input,
              base::BindOnce([](
                  PredictiveOmnibox* self,
                  int tab_id,
                  SuggestionsCallback callback,
                  const ai::SmartSuggestions::SuggestionsResult& smart_result) {
                // Generate action suggestions
                std::vector<ActionSuggestion> action_suggestions;
                self->GenerateActionSuggestions(tab_id, &action_suggestions);

                // Convert smart suggestions to predictive suggestions
                OmniboxSuggestions result;
                result.success = smart_result.success;
                result.error_message = smart_result.error_message;
                
                // Merge suggestions
                self->MergeSuggestions(&result.suggestions, 
                                     smart_result.suggestions, 
                                     action_suggestions);
                
                // Rank suggestions
                self->RankSuggestions(&result.suggestions);
                
                std::move(callback).Run(result);
              }, self, tab_id, std::move(callback)));
          return;
        }

        // Build topics string
        std::stringstream topics_stream;
        for (const auto& topic : analysis_result.topics) {
          topics_stream << topic.name << " (" << topic.confidence << "), ";
        }
        std::string topics_str = topics_stream.str();
        if (!topics_str.empty()) {
          topics_str = topics_str.substr(0, topics_str.length() - 2);  // Remove trailing ", "
        }

        // Get browsing context from context manager
        self->context_manager_->GetUserContext(
            base::BindOnce([](
                PredictiveOmnibox* self,
                const std::string& input,
                int tab_id,
                std::string page_url,
                std::string page_title,
                std::string topics_str,
                const ai::ContentUnderstanding::AnalysisResult& analysis_result,
                SuggestionsCallback callback,
                const asol::core::ContextManager::UserContext& user_context) {
              // Prepare AI prompt
              std::string prompt = kSuggestionPrompt;
              base::ReplaceSubstringsAfterOffset(&prompt, 0, "{input}", input);
              base::ReplaceSubstringsAfterOffset(&prompt, 0, "{title}", page_title);
              base::ReplaceSubstringsAfterOffset(&prompt, 0, "{url}", page_url);
              base::ReplaceSubstringsAfterOffset(&prompt, 0, "{topics}", topics_str);
              base::ReplaceSubstringsAfterOffset(&prompt, 0, "{context}", user_context.recent_browsing_summary);

              // Request AI suggestions
              self->ai_service_manager_->GetTextAdapter()->GenerateText(
                  prompt,
                  base::BindOnce([](
                      PredictiveOmnibox* self,
                      int tab_id,
                      const ai::ContentUnderstanding::AnalysisResult& analysis_result,
                      SuggestionsCallback callback,
                      const asol::core::TextAdapter::GenerateTextResult& text_result) {
                    if (!text_result.success) {
                      // Fall back to smart suggestions
                      self->smart_suggestions_->GetSuggestionsForCurrentPage(
                          tab_id,
                          base::BindOnce([](
                              PredictiveOmnibox* self,
                              int tab_id,
                              SuggestionsCallback callback,
                              const ai::SmartSuggestions::SuggestionsResult& smart_result) {
                            // Generate action suggestions
                            std::vector<ActionSuggestion> action_suggestions;
                            self->GenerateActionSuggestions(tab_id, &action_suggestions);

                            // Convert smart suggestions to predictive suggestions
                            OmniboxSuggestions result;
                            result.success = smart_result.success;
                            result.error_message = smart_result.error_message;
                            
                            // Merge suggestions
                            self->MergeSuggestions(&result.suggestions, 
                                                 smart_result.suggestions, 
                                                 action_suggestions);
                            
                            // Rank suggestions
                            self->RankSuggestions(&result.suggestions);
                            
                            std::move(callback).Run(result);
                          }, self, tab_id, std::move(callback)));
                      return;
                    }

                    // Parse AI response
                    absl::optional<base::Value> json = base::JSONReader::Read(text_result.text);
                    if (!json || !json->is_dict()) {
                      OmniboxSuggestions error_result;
                      error_result.success = false;
                      error_result.error_message = "Failed to parse AI response";
                      std::move(callback).Run(error_result);
                      return;
                    }

                    // Extract suggestions
                    const base::Value::List* suggestions_list = json->GetDict().FindList("suggestions");
                    if (!suggestions_list) {
                      OmniboxSuggestions error_result;
                      error_result.success = false;
                      error_result.error_message = "No suggestions found in AI response";
                      std::move(callback).Run(error_result);
                      return;
                    }

                    // Convert to predictive suggestions
                    OmniboxSuggestions result;
                    result.success = true;
                    
                    for (const auto& suggestion_value : *suggestions_list) {
                      if (!suggestion_value.is_dict()) continue;
                      
                      const base::Value::Dict& suggestion_dict = suggestion_value.GetDict();
                      
                      PredictiveSuggestion suggestion;
                      suggestion.text = suggestion_dict.FindString("text").value_or("");
                      suggestion.url = suggestion_dict.FindString("url").value_or("");
                      suggestion.description = suggestion_dict.FindString("description").value_or("");
                      suggestion.relevance_score = suggestion_dict.FindDouble("relevance_score").value_or(0.0);
                      suggestion.is_search_query = suggestion_dict.FindBool("is_search_query").value_or(false);
                      suggestion.is_navigation = suggestion_dict.FindBool("is_navigation").value_or(false);
                      suggestion.is_action = false;  // Will be set for action suggestions
                      
                      if (!suggestion.text.empty()) {
                        result.suggestions.push_back(suggestion);
                      }
                    }

                    // Generate action suggestions
                    std::vector<ActionSuggestion> action_suggestions;
                    self->GenerateActionSuggestions(tab_id, &action_suggestions);
                    
                    // Merge with action suggestions
                    self->MergeSuggestions(&result.suggestions, 
                                         {}, // Already added AI suggestions
                                         action_suggestions);
                    
                    // Rank suggestions
                    self->RankSuggestions(&result.suggestions);
                    
                    std::move(callback).Run(result);
                  }, self, tab_id, analysis_result, std::move(callback)));
            }, self, input, tab_id, page_url, page_title, topics_str, analysis_result, std::move(callback)));
      }, this, input, tab_id, page_url, page_title, std::move(callback)));
}

void PredictiveOmnibox::GenerateActionSuggestions(
    int tab_id,
    std::vector<ActionSuggestion>* actions) {
  if (!actions) return;
  
  // Get the current tab
  Tab* tab = browser_engine_->GetTabById(tab_id);
  if (!tab) return;
  
  // Get page information
  std::string page_url = tab->GetURL();
  std::string page_title = tab->GetTitle();
  
  // Add default actions based on URL patterns
  if (base::StartsWith(page_url, "https://www.youtube.com/watch")) {
    actions->push_back(CreateActionSuggestion(
        ActionType::SUMMARIZE,
        "Summarize this video",
        "Get a concise summary of this video's content",
        "summarize_icon",
        0.9f));
  } else if (base::EndsWith(page_url, ".pdf")) {
    actions->push_back(CreateActionSuggestion(
        ActionType::SUMMARIZE,
        "Summarize this PDF",
        "Get a concise summary of this PDF document",
        "summarize_icon",
        0.9f));
  } else if (base::StartsWith(page_url, "https://www.amazon.com/") ||
             base::StartsWith(page_url, "https://www.ebay.com/") ||
             base::StartsWith(page_url, "https://www.walmart.com/")) {
    actions->push_back(CreateActionSuggestion(
        ActionType::SHOP_COMPARE,
        "Compare prices",
        "Find better deals for this product",
        "shop_icon",
        0.9f));
  } else if (base::StartsWith(page_url, "https://github.com/")) {
    actions->push_back(CreateActionSuggestion(
        ActionType::ANALYZE,
        "Analyze repository",
        "Get insights about this GitHub repository",
        "analyze_icon",
        0.9f));
  } else {
    // Default actions for general pages
    actions->push_back(CreateActionSuggestion(
        ActionType::SUMMARIZE,
        "Summarize this page",
        "Get a concise summary of this page's content",
        "summarize_icon",
        0.8f));
    
    actions->push_back(CreateActionSuggestion(
        ActionType::FIND_SIMILAR,
        "Find similar content",
        "Discover related articles and resources",
        "find_icon",
        0.7f));
  }
  
  // Add translate action if the page might be in a foreign language
  // This would normally use language detection, but we'll simplify for the example
  if (base::StartsWith(page_url, "https://www.") && 
      !base::EndsWith(page_url, ".com") &&
      !base::EndsWith(page_url, ".org") &&
      !base::EndsWith(page_url, ".net") &&
      !base::EndsWith(page_url, ".edu")) {
    actions->push_back(CreateActionSuggestion(
        ActionType::TRANSLATE,
        "Translate this page",
        "Translate this page to your preferred language",
        "translate_icon",
        0.8f));
  }
  
  // Add research action for educational content
  if (base::StartsWith(page_url, "https://en.wikipedia.org/") ||
      base::StartsWith(page_url, "https://www.britannica.com/") ||
      base::StartsWith(page_url, "https://www.khanacademy.org/")) {
    actions->push_back(CreateActionSuggestion(
        ActionType::RESEARCH,
        "Research this topic",
        "Find more in-depth information about this topic",
        "research_icon",
        0.9f));
  }
}

void PredictiveOmnibox::MergeSuggestions(
    std::vector<PredictiveSuggestion>* merged_suggestions,
    const std::vector<ai::SmartSuggestions::Suggestion>& smart_suggestions,
    const std::vector<ActionSuggestion>& action_suggestions) {
  if (!merged_suggestions) return;
  
  // Add smart suggestions
  for (const auto& smart_suggestion : smart_suggestions) {
    PredictiveSuggestion suggestion;
    suggestion.text = smart_suggestion.text;
    suggestion.url = smart_suggestion.url;
    suggestion.description = smart_suggestion.description;
    suggestion.relevance_score = smart_suggestion.relevance_score;
    
    // Determine suggestion type
    suggestion.is_search_query = 
        (smart_suggestion.type == ai::SmartSuggestions::SuggestionType::SEARCH_QUERY);
    suggestion.is_navigation = 
        (smart_suggestion.type == ai::SmartSuggestions::SuggestionType::NAVIGATION);
    suggestion.is_action = false;
    
    merged_suggestions->push_back(suggestion);
  }
  
  // Add action suggestions
  for (const auto& action : action_suggestions) {
    PredictiveSuggestion suggestion;
    suggestion.text = action.display_text;
    suggestion.description = action.description;
    suggestion.relevance_score = action.relevance_score;
    suggestion.is_search_query = false;
    suggestion.is_navigation = false;
    suggestion.is_action = true;
    suggestion.action = action;
    
    merged_suggestions->push_back(suggestion);
  }
}

void PredictiveOmnibox::RankSuggestions(
    std::vector<PredictiveSuggestion>* suggestions) {
  if (!suggestions) return;
  
  // Sort by relevance score (descending)
  std::sort(suggestions->begin(), suggestions->end(),
            [](const PredictiveSuggestion& a, const PredictiveSuggestion& b) {
              return a.relevance_score > b.relevance_score;
            });
  
  // Limit to top 10 suggestions
  if (suggestions->size() > 10) {
    suggestions->resize(10);
  }
}

void PredictiveOmnibox::ExecuteAction(
    const ActionSuggestion& action,
    int tab_id,
    ActionExecutedCallback callback) {
  if (!is_enabled_) {
    std::move(callback).Run(false, "Predictive features are disabled");
    return;
  }
  
  // Get the current tab
  Tab* tab = browser_engine_->GetTabById(tab_id);
  if (!tab) {
    std::move(callback).Run(false, "Tab not found");
    return;
  }
  
  // Get page information
  std::string page_url = tab->GetURL();
  std::string page_title = tab->GetTitle();
  
  // Execute the action based on its type
  switch (action.type) {
    case ActionType::SUMMARIZE: {
      // Get the web contents
      WebContents* web_contents = tab->GetWebContents();
      if (!web_contents) {
        std::move(callback).Run(false, "Web contents not available");
        return;
      }
      
      // Get the page content
      web_contents->GetPageContent(base::BindOnce([](
          asol::core::AIServiceManager* ai_service_manager,
          std::string page_title,
          ActionExecutedCallback callback,
          const std::string& content) {
        // Prepare summarization prompt
        std::string prompt = "Summarize the following content in 3-5 concise bullet points:\n\n";
        prompt += "Title: " + page_title + "\n\n";
        prompt += content;
        
        // Request summarization from AI
        ai_service_manager->GetTextAdapter()->GenerateText(
            prompt,
            base::BindOnce([](
                ActionExecutedCallback callback,
                const asol::core::TextAdapter::GenerateTextResult& result) {
              if (!result.success) {
                std::move(callback).Run(false, "Failed to generate summary: " + result.error_message);
                return;
              }
              
              std::move(callback).Run(true, result.text);
            }, std::move(callback)));
      }, ai_service_manager_, page_title, std::move(callback)));
      break;
    }
    
    case ActionType::TRANSLATE: {
      // Get the web contents
      WebContents* web_contents = tab->GetWebContents();
      if (!web_contents) {
        std::move(callback).Run(false, "Web contents not available");
        return;
      }
      
      // Get the page content
      web_contents->GetPageContent(base::BindOnce([](
          asol::core::AIServiceManager* ai_service_manager,
          ActionExecutedCallback callback,
          const std::string& content) {
        // Prepare translation prompt
        std::string prompt = "Translate the following content to English:\n\n";
        prompt += content;
        
        // Request translation from AI
        ai_service_manager->GetTextAdapter()->GenerateText(
            prompt,
            base::BindOnce([](
                ActionExecutedCallback callback,
                const asol::core::TextAdapter::GenerateTextResult& result) {
              if (!result.success) {
                std::move(callback).Run(false, "Failed to translate: " + result.error_message);
                return;
              }
              
              std::move(callback).Run(true, result.text);
            }, std::move(callback)));
      }, ai_service_manager_, std::move(callback)));
      break;
    }
    
    case ActionType::FIND_SIMILAR: {
      // Prepare search query based on page title
      std::string search_query = "similar to: " + page_title;
      
      // Return the search query for the browser to execute
      std::move(callback).Run(true, search_query);
      break;
    }
    
    case ActionType::RESEARCH: {
      // Get the web contents
      WebContents* web_contents = tab->GetWebContents();
      if (!web_contents) {
        std::move(callback).Run(false, "Web contents not available");
        return;
      }
      
      // Get the page content
      web_contents->GetPageContent(base::BindOnce([](
          asol::core::AIServiceManager* ai_service_manager,
          std::string page_title,
          ActionExecutedCallback callback,
          const std::string& content) {
        // Prepare research prompt
        std::string prompt = "Based on this content, suggest 5 specific research questions to explore this topic further:\n\n";
        prompt += "Title: " + page_title + "\n\n";
        prompt += content;
        
        // Request research suggestions from AI
        ai_service_manager->GetTextAdapter()->GenerateText(
            prompt,
            base::BindOnce([](
                ActionExecutedCallback callback,
                const asol::core::TextAdapter::GenerateTextResult& result) {
              if (!result.success) {
                std::move(callback).Run(false, "Failed to generate research suggestions: " + result.error_message);
                return;
              }
              
              std::move(callback).Run(true, result.text);
            }, std::move(callback)));
      }, ai_service_manager_, page_title, std::move(callback)));
      break;
    }
    
    case ActionType::SHOP_COMPARE: {
      // Extract product name from page title
      std::string product_name = page_title;
      
      // Return the product search query for the browser to execute
      std::string search_query = "compare prices for: " + product_name;
      std::move(callback).Run(true, search_query);
      break;
    }
    
    case ActionType::SAVE_FOR_LATER: {
      // In a real implementation, this would save the page to a reading list
      // For this example, we'll just return a success message
      std::move(callback).Run(true, "Page saved to reading list: " + page_title);
      break;
    }
    
    case ActionType::SHARE: {
      // In a real implementation, this would open a share dialog
      // For this example, we'll just return a success message
      std::move(callback).Run(true, "Share dialog opened for: " + page_title);
      break;
    }
    
    case ActionType::ANALYZE: {
      // Get the web contents
      WebContents* web_contents = tab->GetWebContents();
      if (!web_contents) {
        std::move(callback).Run(false, "Web contents not available");
        return;
      }
      
      // Get the page content
      web_contents->GetPageContent(base::BindOnce([](
          asol::core::AIServiceManager* ai_service_manager,
          std::string page_title,
          std::string page_url,
          ActionExecutedCallback callback,
          const std::string& content) {
        // Prepare analysis prompt
        std::string prompt = "Analyze the following content and provide key insights:\n\n";
        prompt += "Title: " + page_title + "\n";
        prompt += "URL: " + page_url + "\n\n";
        prompt += content;
        
        // Request analysis from AI
        ai_service_manager->GetTextAdapter()->GenerateText(
            prompt,
            base::BindOnce([](
                ActionExecutedCallback callback,
                const asol::core::TextAdapter::GenerateTextResult& result) {
              if (!result.success) {
                std::move(callback).Run(false, "Failed to analyze content: " + result.error_message);
                return;
              }
              
              std::move(callback).Run(true, result.text);
            }, std::move(callback)));
      }, ai_service_manager_, page_title, page_url, std::move(callback)));
      break;
    }
    
    default: {
      std::move(callback).Run(false, "Unsupported action type");
      break;
    }
  }
}

void PredictiveOmnibox::Enable(bool enable) {
  is_enabled_ = enable;
}

bool PredictiveOmnibox::IsEnabled() const {
  return is_enabled_;
}

base::WeakPtr<PredictiveOmnibox> PredictiveOmnibox::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

}  // namespace ui
}  // namespace browser_core