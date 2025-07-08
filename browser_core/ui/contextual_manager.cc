// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser_core/ui/contextual_manager.h"

#include <algorithm>
#include <sstream>
#include <utility>
#include <set>
#include <ctime>
#include <iomanip>

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
constexpr char kTaskDetectionPrompt[] = 
    "Analyze the user's recent browsing activity and detect potential tasks or goals "
    "the user might be trying to accomplish. For each detected task, provide a name, "
    "description, confidence score (0.0-1.0), and related topics.\n\n"
    "Recent browsing activity:\n{browsing_activity}\n\n"
    "User interests: {user_interests}\n\n"
    "Format response as JSON with an array of task objects, each containing: "
    "name, description, confidence_score, related_topics (array of strings), "
    "and related_urls (array of strings).";

constexpr char kContextSuggestionsPrompt[] =
    "Based on the user's current browsing context, suggest relevant actions, content, "
    "or tools that might help the user. Consider the current page content, detected entities, "
    "topics, and active tasks.\n\n"
    "Current page: {current_url} - {current_title}\n\n"
    "Detected entities: {entities}\n\n"
    "Detected topics: {topics}\n\n"
    "Active tasks: {active_tasks}\n\n"
    "User interests: {user_interests}\n\n"
    "Format response as JSON with an array of suggestion objects, each containing: "
    "title, description, type (one of: NAVIGATION, SEARCH, CONTENT, TOOL, REMINDER), "
    "action_url, and relevance_score (float 0.0-1.0).";

// Helper function to generate a unique ID
std::string GenerateUniqueId(const std::string& prefix) {
  static int counter = 0;
  std::stringstream ss;
  ss << prefix << "_" << std::time(nullptr) << "_" << counter++;
  return ss.str();
}

// Helper function to format a timestamp
std::string FormatTimestamp(const std::chrono::system_clock::time_point& time_point) {
  std::time_t time = std::chrono::system_clock::to_time_t(time_point);
  std::stringstream ss;
  ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
  return ss.str();
}

}  // namespace

ContextualManager::ContextualManager() = default;
ContextualManager::~ContextualManager() = default;

bool ContextualManager::Initialize(
    BrowserEngine* browser_engine,
    asol::core::AIServiceManager* ai_service_manager,
    asol::core::ContextManager* context_manager,
    ai::ContentUnderstanding* content_understanding) {
  if (!browser_engine || !ai_service_manager || !context_manager || !content_understanding) {
    return false;
  }

  browser_engine_ = browser_engine;
  ai_service_manager_ = ai_service_manager;
  context_manager_ = context_manager;
  content_understanding_ = content_understanding;
  
  // Initialize current context
  current_context_.timestamp = std::chrono::system_clock::now();
  
  return true;
}

void ContextualManager::UpdateContext(
    const std::string& url, 
    const std::string& title,
    const std::string& content) {
  if (!is_enabled_) {
    return;
  }

  // Store previous context in history (limit history size)
  if (context_history_.size() >= 20) {
    context_history_.erase(context_history_.begin());
  }
  context_history_.push_back(current_context_);
  
  // Update current context with basic information
  current_context_.active_url = url;
  current_context_.active_tab_title = title;
  current_context_.timestamp = std::chrono::system_clock::now();
  
  // Clear previous entities and topics
  current_context_.entities.clear();
  current_context_.topics.clear();
  
  // Analyze page content to update context
  AnalyzePageContent(url, title, content);
  
  // Periodically detect user tasks
  // In a real implementation, this would be throttled and possibly done in the background
  if (context_history_.size() % 5 == 0) {
    DetectUserTasks();
  }
}

void ContextualManager::GetContextSnapshot(ContextSnapshotCallback callback) {
  if (!is_enabled_) {
    ContextSnapshot empty_snapshot;
    empty_snapshot.timestamp = std::chrono::system_clock::now();
    std::move(callback).Run(empty_snapshot);
    return;
  }

  std::move(callback).Run(current_context_);
}

void ContextualManager::GetContextSuggestions(ContextSuggestionsCallback callback) {
  if (!is_enabled_) {
    std::move(callback).Run({});
    return;
  }

  GenerateContextSuggestions(std::move(callback));
}

void ContextualManager::GetUserTasks(UserTasksCallback callback) {
  if (!is_enabled_) {
    std::move(callback).Run({});
    return;
  }

  std::move(callback).Run(user_tasks_);
}

void ContextualManager::CreateUserTask(
    const std::string& name, 
    const std::string& description,
    UserTasksCallback callback) {
  if (!is_enabled_) {
    std::move(callback).Run(user_tasks_);
    return;
  }

  // Create new task
  UserTask task;
  task.id = GenerateUniqueId("task");
  task.name = name;
  task.description = description;
  task.start_time = std::chrono::system_clock::now();
  task.last_activity_time = task.start_time;
  task.confidence_score = 1.0f;  // User-created tasks have maximum confidence
  task.is_completed = false;
  
  // Add current URL to related URLs
  if (!current_context_.active_url.empty()) {
    task.related_urls.push_back(current_context_.active_url);
  }
  
  // Add current topics to related topics
  for (const auto& topic : current_context_.topics) {
    ContextTopic related_topic;
    related_topic.name = topic.name;
    related_topic.relevance_score = topic.relevance_score;
    task.related_topics.push_back(related_topic);
  }
  
  // Add to user tasks
  user_tasks_.push_back(task);
  
  // Update active tasks in current context
  current_context_.active_tasks.clear();
  for (const auto& task : user_tasks_) {
    if (!task.is_completed) {
      current_context_.active_tasks.push_back(task);
    }
  }
  
  std::move(callback).Run(user_tasks_);
}

void ContextualManager::CompleteUserTask(
    const std::string& task_id, UserTasksCallback callback) {
  if (!is_enabled_) {
    std::move(callback).Run(user_tasks_);
    return;
  }

  // Find and mark task as completed
  for (auto& task : user_tasks_) {
    if (task.id == task_id) {
      task.is_completed = true;
      break;
    }
  }
  
  // Update active tasks in current context
  current_context_.active_tasks.clear();
  for (const auto& task : user_tasks_) {
    if (!task.is_completed) {
      current_context_.active_tasks.push_back(task);
    }
  }
  
  std::move(callback).Run(user_tasks_);
}

void ContextualManager::Enable(bool enable) {
  is_enabled_ = enable;
}

bool ContextualManager::IsEnabled() const {
  return is_enabled_;
}

base::WeakPtr<ContextualManager> ContextualManager::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void ContextualManager::AnalyzePageContent(
    const std::string& url,
    const std::string& title,
    const std::string& content) {
  // Use content understanding to analyze the page
  content_understanding_->AnalyzeContent(
      content,
      base::BindOnce([](
          ContextualManager* self,
          std::string url,
          std::string title,
          const ai::ContentUnderstanding::AnalysisResult& result) {
        if (!result.success) {
          return;
        }
        
        // Update context with topics
        for (const auto& topic : result.topics) {
          ContextTopic context_topic;
          context_topic.name = topic.name;
          context_topic.relevance_score = topic.confidence;
          self->current_context_.topics.push_back(context_topic);
        }
        
        // Update context with entities
        for (const auto& entity : result.entities) {
          ContextEntity context_entity;
          context_entity.name = entity.name;
          context_entity.type = entity.type;
          context_entity.relevance_score = entity.confidence;
          self->current_context_.entities.push_back(context_entity);
        }
        
        // Update user tasks with current URL if relevant
        for (auto& task : self->user_tasks_) {
          if (task.is_completed) {
            continue;
          }
          
          // Check if current page is relevant to any active task
          bool is_relevant = false;
          
          // Check if topics match
          for (const auto& task_topic : task.related_topics) {
            for (const auto& page_topic : result.topics) {
              if (base::ToLowerASCII(task_topic.name) == base::ToLowerASCII(page_topic.name)) {
                is_relevant = true;
                break;
              }
            }
            if (is_relevant) break;
          }
          
          if (is_relevant) {
            // Add URL to task if not already present
            if (std::find(task.related_urls.begin(), task.related_urls.end(), url) == task.related_urls.end()) {
              task.related_urls.push_back(url);
            }
            
            // Update last activity time
            task.last_activity_time = std::chrono::system_clock::now();
          }
        }
        
        // Update active tasks in current context
        self->current_context_.active_tasks.clear();
        for (const auto& task : self->user_tasks_) {
          if (!task.is_completed) {
            self->current_context_.active_tasks.push_back(task);
          }
        }
      }, this, url, title));
}

void ContextualManager::DetectUserTasks() {
  if (context_history_.empty()) {
    return;
  }

  // Build browsing activity for the prompt
  std::stringstream browsing_activity_stream;
  for (size_t i = 0; i < context_history_.size(); ++i) {
    const auto& context = context_history_[i];
    browsing_activity_stream << "Page " << i + 1 << ": " 
                           << context.active_tab_title 
                           << " (" << context.active_url << ")"
                           << ", Time: " << FormatTimestamp(context.timestamp);
    
    if (!context.topics.empty()) {
      browsing_activity_stream << ", Topics: ";
      for (size_t j = 0; j < context.topics.size() && j < 3; ++j) {
        if (j > 0) browsing_activity_stream << ", ";
        browsing_activity_stream << context.topics[j].name;
      }
    }
    
    browsing_activity_stream << "\n";
  }
  std::string browsing_activity = browsing_activity_stream.str();

  // Get user interests from context manager
  context_manager_->GetUserContext(
      base::BindOnce([](
          ContextualManager* self,
          std::string browsing_activity,
          const asol::core::ContextManager::UserContext& user_context) {
        // Build user interests string
        std::stringstream interests_stream;
        for (size_t i = 0; i < user_context.interests.size(); ++i) {
          if (i > 0) interests_stream << ", ";
          interests_stream << user_context.interests[i];
        }
        std::string user_interests = interests_stream.str();
        
        // Prepare AI prompt
        std::string prompt = kTaskDetectionPrompt;
        base::ReplaceSubstringsAfterOffset(&prompt, 0, "{browsing_activity}", browsing_activity);
        base::ReplaceSubstringsAfterOffset(&prompt, 0, "{user_interests}", user_interests);
        
        // Request AI task detection
        self->ai_service_manager_->GetTextAdapter()->GenerateText(
            prompt,
            base::BindOnce([](
                ContextualManager* self,
                const asol::core::TextAdapter::GenerateTextResult& text_result) {
              if (!text_result.success) {
                return;
              }
              
              // Parse AI response
              absl::optional<base::Value> json = base::JSONReader::Read(text_result.text);
              if (!json || !json->is_dict()) {
                return;
              }
              
              // Extract tasks
              const base::Value::List* tasks_list = json->GetDict().FindList("tasks");
              if (!tasks_list) {
                return;
              }
              
              // Process detected tasks
              std::vector<UserTask> detected_tasks;
              
              for (const auto& task_value : *tasks_list) {
                if (!task_value.is_dict()) continue;
                
                const base::Value::Dict& task_dict = task_value.GetDict();
                
                std::string name = task_dict.FindString("name").value_or("Unnamed Task");
                std::string description = task_dict.FindString("description").value_or("");
                float confidence_score = task_dict.FindDouble("confidence_score").value_or(0.5);
                
                // Skip low confidence tasks
                if (confidence_score < 0.6) {
                  continue;
                }
                
                // Create task
                UserTask task;
                task.id = GenerateUniqueId("task");
                task.name = name;
                task.description = description;
                task.start_time = std::chrono::system_clock::now();
                task.last_activity_time = task.start_time;
                task.confidence_score = confidence_score;
                task.is_completed = false;
                
                // Extract related topics
                const base::Value::List* topics_list = task_dict.FindList("related_topics");
                if (topics_list) {
                  for (const auto& topic_value : *topics_list) {
                    if (topic_value.is_string()) {
                      ContextTopic topic;
                      topic.name = topic_value.GetString();
                      topic.relevance_score = 0.8f;  // Default relevance
                      task.related_topics.push_back(topic);
                    }
                  }
                }
                
                // Extract related URLs
                const base::Value::List* urls_list = task_dict.FindList("related_urls");
                if (urls_list) {
                  for (const auto& url_value : *urls_list) {
                    if (url_value.is_string()) {
                      task.related_urls.push_back(url_value.GetString());
                    }
                  }
                }
                
                detected_tasks.push_back(task);
              }
              
              // Merge with existing tasks
              for (const auto& detected_task : detected_tasks) {
                // Check if similar task already exists
                bool task_exists = false;
                
                for (auto& existing_task : self->user_tasks_) {
                  // Simple similarity check based on name
                  if (base::ToLowerASCII(existing_task.name) == base::ToLowerASCII(detected_task.name) ||
                      base::ToLowerASCII(existing_task.name).find(base::ToLowerASCII(detected_task.name)) != std::string::npos ||
                      base::ToLowerASCII(detected_task.name).find(base::ToLowerASCII(existing_task.name)) != std::string::npos) {
                    task_exists = true;
                    
                    // Update existing task if detected task has higher confidence
                    if (detected_task.confidence_score > existing_task.confidence_score) {
                      existing_task.description = detected_task.description;
                      existing_task.confidence_score = detected_task.confidence_score;
                    }
                    
                    // Merge related URLs
                    for (const auto& url : detected_task.related_urls) {
                      if (std::find(existing_task.related_urls.begin(), existing_task.related_urls.end(), url) == existing_task.related_urls.end()) {
                        existing_task.related_urls.push_back(url);
                      }
                    }
                    
                    // Merge related topics
                    for (const auto& topic : detected_task.related_topics) {
                      bool topic_exists = false;
                      for (const auto& existing_topic : existing_task.related_topics) {
                        if (base::ToLowerASCII(existing_topic.name) == base::ToLowerASCII(topic.name)) {
                          topic_exists = true;
                          break;
                        }
                      }
                      
                      if (!topic_exists) {
                        existing_task.related_topics.push_back(topic);
                      }
                    }
                    
                    break;
                  }
                }
                
                // Add new task if it doesn't exist
                if (!task_exists) {
                  self->user_tasks_.push_back(detected_task);
                }
              }
              
              // Update active tasks in current context
              self->current_context_.active_tasks.clear();
              for (const auto& task : self->user_tasks_) {
                if (!task.is_completed) {
                  self->current_context_.active_tasks.push_back(task);
                }
              }
            }, self));
      }, this, browsing_activity));
}

void ContextualManager::GenerateContextSuggestions(ContextSuggestionsCallback callback) {
  if (current_context_.active_url.empty()) {
    std::move(callback).Run({});
    return;
  }

  // Build entities string
  std::stringstream entities_stream;
  for (size_t i = 0; i < current_context_.entities.size(); ++i) {
    if (i > 0) entities_stream << ", ";
    entities_stream << current_context_.entities[i].name << " (" << current_context_.entities[i].type << ")";
  }
  std::string entities = entities_stream.str();
  
  // Build topics string
  std::stringstream topics_stream;
  for (size_t i = 0; i < current_context_.topics.size(); ++i) {
    if (i > 0) topics_stream << ", ";
    topics_stream << current_context_.topics[i].name;
  }
  std::string topics = topics_stream.str();
  
  // Build active tasks string
  std::stringstream tasks_stream;
  for (size_t i = 0; i < current_context_.active_tasks.size(); ++i) {
    if (i > 0) tasks_stream << "; ";
    tasks_stream << current_context_.active_tasks[i].name << ": " << current_context_.active_tasks[i].description;
  }
  std::string active_tasks = tasks_stream.str();
  
  // Get user interests from context manager
  context_manager_->GetUserContext(
      base::BindOnce([](
          ContextualManager* self,
          std::string current_url,
          std::string current_title,
          std::string entities,
          std::string topics,
          std::string active_tasks,
          ContextSuggestionsCallback callback,
          const asol::core::ContextManager::UserContext& user_context) {
        // Build user interests string
        std::stringstream interests_stream;
        for (size_t i = 0; i < user_context.interests.size(); ++i) {
          if (i > 0) interests_stream << ", ";
          interests_stream << user_context.interests[i];
        }
        std::string user_interests = interests_stream.str();
        
        // Prepare AI prompt
        std::string prompt = kContextSuggestionsPrompt;
        base::ReplaceSubstringsAfterOffset(&prompt, 0, "{current_url}", current_url);
        base::ReplaceSubstringsAfterOffset(&prompt, 0, "{current_title}", current_title);
        base::ReplaceSubstringsAfterOffset(&prompt, 0, "{entities}", entities);
        base::ReplaceSubstringsAfterOffset(&prompt, 0, "{topics}", topics);
        base::ReplaceSubstringsAfterOffset(&prompt, 0, "{active_tasks}", active_tasks);
        base::ReplaceSubstringsAfterOffset(&prompt, 0, "{user_interests}", user_interests);
        
        // Request AI suggestions
        self->ai_service_manager_->GetTextAdapter()->GenerateText(
            prompt,
            base::BindOnce([](
                ContextualManager* self,
                ContextSuggestionsCallback callback,
                const asol::core::TextAdapter::GenerateTextResult& text_result) {
              if (!text_result.success) {
                std::move(callback).Run({});
                return;
              }
              
              // Parse AI response
              absl::optional<base::Value> json = base::JSONReader::Read(text_result.text);
              if (!json || !json->is_dict()) {
                std::move(callback).Run({});
                return;
              }
              
              // Extract suggestions
              const base::Value::List* suggestions_list = json->GetDict().FindList("suggestions");
              if (!suggestions_list) {
                std::move(callback).Run({});
                return;
              }
              
              // Process suggestions
              std::vector<ContextSuggestion> context_suggestions;
              
              for (const auto& suggestion_value : *suggestions_list) {
                if (!suggestion_value.is_dict()) continue;
                
                const base::Value::Dict& suggestion_dict = suggestion_value.GetDict();
                
                std::string title = suggestion_dict.FindString("title").value_or("Unnamed Suggestion");
                std::string description = suggestion_dict.FindString("description").value_or("");
                std::string type_str = suggestion_dict.FindString("type").value_or("CONTENT");
                std::string action_url = suggestion_dict.FindString("action_url").value_or("");
                float relevance_score = suggestion_dict.FindDouble("relevance_score").value_or(0.5);
                
                // Skip low relevance suggestions
                if (relevance_score < 0.6) {
                  continue;
                }
                
                // Create suggestion
                ContextSuggestion suggestion;
                suggestion.id = GenerateUniqueId("suggestion");
                suggestion.title = title;
                suggestion.description = description;
                suggestion.action_url = action_url;
                suggestion.relevance_score = relevance_score;
                
                // Parse type
                if (type_str == "NAVIGATION") {
                  suggestion.type = ContextSuggestion::Type::NAVIGATION;
                } else if (type_str == "SEARCH") {
                  suggestion.type = ContextSuggestion::Type::SEARCH;
                } else if (type_str == "TOOL") {
                  suggestion.type = ContextSuggestion::Type::TOOL;
                } else if (type_str == "REMINDER") {
                  suggestion.type = ContextSuggestion::Type::REMINDER;
                } else {
                  suggestion.type = ContextSuggestion::Type::CONTENT;
                }
                
                // Extract metadata
                const base::Value::Dict* metadata_dict = suggestion_dict.FindDict("metadata");
                if (metadata_dict) {
                  for (auto it = metadata_dict->begin(); it != metadata_dict->end(); ++it) {
                    if (it->second.is_string()) {
                      suggestion.metadata[it->first] = it->second.GetString();
                    }
                  }
                }
                
                context_suggestions.push_back(suggestion);
              }
              
              // Sort by relevance score (descending)
              std::sort(context_suggestions.begin(), context_suggestions.end(),
                      [](const ContextSuggestion& a, const ContextSuggestion& b) {
                        return a.relevance_score > b.relevance_score;
                      });
              
              std::move(callback).Run(context_suggestions);
            }, self, std::move(callback)));
      }, this, current_context_.active_url, current_context_.active_tab_title, 
      entities, topics, active_tasks, std::move(callback)));
}

}  // namespace ui
}  // namespace browser_core// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser_core/ui/contextual_manager.h"

#include <algorithm>
#include <sstream>
#include <utility>
#include <set>
#include <ctime>
#include <iomanip>

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
constexpr char kTaskDetectionPrompt[] = 
    "Analyze the user's recent browsing activity and detect potential tasks or goals "
    "the user might be trying to accomplish. For each detected task, provide a name, "
    "description, confidence score (0.0-1.0), and related topics.\n\n"
    "Recent browsing activity:\n{browsing_activity}\n\n"
    "User interests: {user_interests}\n\n"
    "Format response as JSON with an array of task objects, each containing: "
    "name, description, confidence_score, related_topics (array of strings), "
    "and related_urls (array of strings).";

constexpr char kContextSuggestionsPrompt[] =
    "Based on the user's current browsing context, suggest relevant actions, content, "
    "or tools that might help the user. Consider the current page content, detected entities, "
    "topics, and active tasks.\n\n"
    "Current page: {current_url} - {current_title}\n\n"
    "Detected entities: {entities}\n\n"
    "Detected topics: {topics}\n\n"
    "Active tasks: {active_tasks}\n\n"
    "User interests: {user_interests}\n\n"
    "Format response as JSON with an array of suggestion objects, each containing: "
    "title, description, type (one of: NAVIGATION, SEARCH, CONTENT, TOOL, REMINDER), "
    "action_url, and relevance_score (float 0.0-1.0).";

// Helper function to generate a unique ID
std::string GenerateUniqueId(const std::string& prefix) {
  static int counter = 0;
  std::stringstream ss;
  ss << prefix << "_" << std::time(nullptr) << "_" << counter++;
  return ss.str();
}

// Helper function to format a timestamp
std::string FormatTimestamp(const std::chrono::system_clock::time_point& time_point) {
  std::time_t time = std::chrono::system_clock::to_time_t(time_point);
  std::stringstream ss;
  ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
  return ss.str();
}

}  // namespace

ContextualManager::ContextualManager() = default;
ContextualManager::~ContextualManager() = default;

bool ContextualManager::Initialize(
    BrowserEngine* browser_engine,
    asol::core::AIServiceManager* ai_service_manager,
    asol::core::ContextManager* context_manager,
    ai::ContentUnderstanding* content_understanding) {
  if (!browser_engine || !ai_service_manager || !context_manager || !content_understanding) {
    return false;
  }

  browser_engine_ = browser_engine;
  ai_service_manager_ = ai_service_manager;
  context_manager_ = context_manager;
  content_understanding_ = content_understanding;
  
  // Initialize current context
  current_context_.timestamp = std::chrono::system_clock::now();
  
  return true;
}

void ContextualManager::UpdateContext(
    const std::string& url, 
    const std::string& title,
    const std::string& content) {
  if (!is_enabled_) {
    return;
  }

  // Store previous context in history (limit history size)
  if (context_history_.size() >= 20) {
    context_history_.erase(context_history_.begin());
  }
  context_history_.push_back(current_context_);
  
  // Update current context with basic information
  current_context_.active_url = url;
  current_context_.active_tab_title = title;
  current_context_.timestamp = std::chrono::system_clock::now();
  
  // Clear previous entities and topics
  current_context_.entities.clear();
  current_context_.topics.clear();
  
  // Analyze page content to update context
  AnalyzePageContent(url, title, content);
  
  // Periodically detect user tasks
  // In a real implementation, this would be throttled and possibly done in the background
  if (context_history_.size() % 5 == 0) {
    DetectUserTasks();
  }
}

void ContextualManager::GetContextSnapshot(ContextSnapshotCallback callback) {
  if (!is_enabled_) {
    ContextSnapshot empty_snapshot;
    empty_snapshot.timestamp = std::chrono::system_clock::now();
    std::move(callback).Run(empty_snapshot);
    return;
  }

  std::move(callback).Run(current_context_);
}

void ContextualManager::GetContextSuggestions(ContextSuggestionsCallback callback) {
  if (!is_enabled_) {
    std::move(callback).Run({});
    return;
  }

  GenerateContextSuggestions(std::move(callback));
}

void ContextualManager::GetUserTasks(UserTasksCallback callback) {
  if (!is_enabled_) {
    std::move(callback).Run({});
    return;
  }

  std::move(callback).Run(user_tasks_);
}

void ContextualManager::CreateUserTask(
    const std::string& name, 
    const std::string& description,
    UserTasksCallback callback) {
  if (!is_enabled_) {
    std::move(callback).Run(user_tasks_);
    return;
  }

  // Create new task
  UserTask task;
  task.id = GenerateUniqueId("task");
  task.name = name;
  task.description = description;
  task.start_time = std::chrono::system_clock::now();
  task.last_activity_time = task.start_time;
  task.confidence_score = 1.0f;  // User-created tasks have maximum confidence
  task.is_completed = false;
  
  // Add current URL to related URLs
  if (!current_context_.active_url.empty()) {
    task.related_urls.push_back(current_context_.active_url);
  }
  
  // Add current topics to related topics
  for (const auto& topic : current_context_.topics) {
    ContextTopic related_topic;
    related_topic.name = topic.name;
    related_topic.relevance_score = topic.relevance_score;
    task.related_topics.push_back(related_topic);
  }
  
  // Add to user tasks
  user_tasks_.push_back(task);
  
  // Update active tasks in current context
  current_context_.active_tasks.clear();
  for (const auto& task : user_tasks_) {
    if (!task.is_completed) {
      current_context_.active_tasks.push_back(task);
    }
  }
  
  std::move(callback).Run(user_tasks_);
}

void ContextualManager::CompleteUserTask(
    const std::string& task_id, UserTasksCallback callback) {
  if (!is_enabled_) {
    std::move(callback).Run(user_tasks_);
    return;
  }

  // Find and mark task as completed
  for (auto& task : user_tasks_) {
    if (task.id == task_id) {
      task.is_completed = true;
      break;
    }
  }
  
  // Update active tasks in current context
  current_context_.active_tasks.clear();
  for (const auto& task : user_tasks_) {
    if (!task.is_completed) {
      current_context_.active_tasks.push_back(task);
    }
  }
  
  std::move(callback).Run(user_tasks_);
}

void ContextualManager::Enable(bool enable) {
  is_enabled_ = enable;
}

bool ContextualManager::IsEnabled() const {
  return is_enabled_;
}

base::WeakPtr<ContextualManager> ContextualManager::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void ContextualManager::AnalyzePageContent(
    const std::string& url,
    const std::string& title,
    const std::string& content) {
  // Use content understanding to analyze the page
  content_understanding_->AnalyzeContent(
      content,
      base::BindOnce([](
          ContextualManager* self,
          std::string url,
          std::string title,
          const ai::ContentUnderstanding::AnalysisResult& result) {
        if (!result.success) {
          return;
        }
        
        // Update context with topics
        for (const auto& topic : result.topics) {
          ContextTopic context_topic;
          context_topic.name = topic.name;
          context_topic.relevance_score = topic.confidence;
          self->current_context_.topics.push_back(context_topic);
        }
        
        // Update context with entities
        for (const auto& entity : result.entities) {
          ContextEntity context_entity;
          context_entity.name = entity.name;
          context_entity.type = entity.type;
          context_entity.relevance_score = entity.confidence;
          self->current_context_.entities.push_back(context_entity);
        }
        
        // Update user tasks with current URL if relevant
        for (auto& task : self->user_tasks_) {
          if (task.is_completed) {
            continue;
          }
          
          // Check if current page is relevant to any active task
          bool is_relevant = false;
          
          // Check if topics match
          for (const auto& task_topic : task.related_topics) {
            for (const auto& page_topic : result.topics) {
              if (base::ToLowerASCII(task_topic.name) == base::ToLowerASCII(page_topic.name)) {
                is_relevant = true;
                break;
              }
            }
            if (is_relevant) break;
          }
          
          if (is_relevant) {
            // Add URL to task if not already present
            if (std::find(task.related_urls.begin(), task.related_urls.end(), url) == task.related_urls.end()) {
              task.related_urls.push_back(url);
            }
            
            // Update last activity time
            task.last_activity_time = std::chrono::system_clock::now();
          }
        }
        
        // Update active tasks in current context
        self->current_context_.active_tasks.clear();
        for (const auto& task : self->user_tasks_) {
          if (!task.is_completed) {
            self->current_context_.active_tasks.push_back(task);
          }
        }
      }, this, url, title));
}

void ContextualManager::DetectUserTasks() {
  if (context_history_.empty()) {
    return;
  }

  // Build browsing activity for the prompt
  std::stringstream browsing_activity_stream;
  for (size_t i = 0; i < context_history_.size(); ++i) {
    const auto& context = context_history_[i];
    browsing_activity_stream << "Page " << i + 1 << ": " 
                           << context.active_tab_title 
                           << " (" << context.active_url << ")"
                           << ", Time: " << FormatTimestamp(context.timestamp);
    
    if (!context.topics.empty()) {
      browsing_activity_stream << ", Topics: ";
      for (size_t j = 0; j < context.topics.size() && j < 3; ++j) {
        if (j > 0) browsing_activity_stream << ", ";
        browsing_activity_stream << context.topics[j].name;
      }
    }
    
    browsing_activity_stream << "\n";
  }
  std::string browsing_activity = browsing_activity_stream.str();

  // Get user interests from context manager
  context_manager_->GetUserContext(
      base::BindOnce([](
          ContextualManager* self,
          std::string browsing_activity,
          const asol::core::ContextManager::UserContext& user_context) {
        // Build user interests string
        std::stringstream interests_stream;
        for (size_t i = 0; i < user_context.interests.size(); ++i) {
          if (i > 0) interests_stream << ", ";
          interests_stream << user_context.interests[i];
        }
        std::string user_interests = interests_stream.str();
        
        // Prepare AI prompt
        std::string prompt = kTaskDetectionPrompt;
        base::ReplaceSubstringsAfterOffset(&prompt, 0, "{browsing_activity}", browsing_activity);
        base::ReplaceSubstringsAfterOffset(&prompt, 0, "{user_interests}", user_interests);
        
        // Request AI task detection
        self->ai_service_manager_->GetTextAdapter()->GenerateText(
            prompt,
            base::BindOnce([](
                ContextualManager* self,
                const asol::core::TextAdapter::GenerateTextResult& text_result) {
              if (!text_result.success) {
                return;
              }
              
              // Parse AI response
              absl::optional<base::Value> json = base::JSONReader::Read(text_result.text);
              if (!json || !json->is_dict()) {
                return;
              }
              
              // Extract tasks
              const base::Value::List* tasks_list = json->GetDict().FindList("tasks");
              if (!tasks_list) {
                return;
              }
              
              // Process detected tasks
              std::vector<UserTask> detected_tasks;
              
              for (const auto& task_value : *tasks_list) {
                if (!task_value.is_dict()) continue;
                
                const base::Value::Dict& task_dict = task_value.GetDict();
                
                std::string name = task_dict.FindString("name").value_or("Unnamed Task");
                std::string description = task_dict.FindString("description").value_or("");
                float confidence_score = task_dict.FindDouble("confidence_score").value_or(0.5);
                
                // Skip low confidence tasks
                if (confidence_score < 0.6) {
                  continue;
                }
                
                // Create task
                UserTask task;
                task.id = GenerateUniqueId("task");
                task.name = name;
                task.description = description;
                task.start_time = std::chrono::system_clock::now();
                task.last_activity_time = task.start_time;
                task.confidence_score = confidence_score;
                task.is_completed = false;
                
                // Extract related topics
                const base::Value::List* topics_list = task_dict.FindList("related_topics");
                if (topics_list) {
                  for (const auto& topic_value : *topics_list) {
                    if (topic_value.is_string()) {
                      ContextTopic topic;
                      topic.name = topic_value.GetString();
                      topic.relevance_score = 0.8f;  // Default relevance
                      task.related_topics.push_back(topic);
                    }
                  }
                }
                
                // Extract related URLs
                const base::Value::List* urls_list = task_dict.FindList("related_urls");
                if (urls_list) {
                  for (const auto& url_value : *urls_list) {
                    if (url_value.is_string()) {
                      task.related_urls.push_back(url_value.GetString());
                    }
                  }
                }
                
                detected_tasks.push_back(task);
              }
              
              // Merge with existing tasks
              for (const auto& detected_task : detected_tasks) {
                // Check if similar task already exists
                bool task_exists = false;
                
                for (auto& existing_task : self->user_tasks_) {
                  // Simple similarity check based on name
                  if (base::ToLowerASCII(existing_task.name) == base::ToLowerASCII(detected_task.name) ||
                      base::ToLowerASCII(existing_task.name).find(base::ToLowerASCII(detected_task.name)) != std::string::npos ||
                      base::ToLowerASCII(detected_task.name).find(base::ToLowerASCII(existing_task.name)) != std::string::npos) {
                    task_exists = true;
                    
                    // Update existing task if detected task has higher confidence
                    if (detected_task.confidence_score > existing_task.confidence_score) {
                      existing_task.description = detected_task.description;
                      existing_task.confidence_score = detected_task.confidence_score;
                    }
                    
                    // Merge related URLs
                    for (const auto& url : detected_task.related_urls) {
                      if (std::find(existing_task.related_urls.begin(), existing_task.related_urls.end(), url) == existing_task.related_urls.end()) {
                        existing_task.related_urls.push_back(url);
                      }
                    }
                    
                    // Merge related topics
                    for (const auto& topic : detected_task.related_topics) {
                      bool topic_exists = false;
                      for (const auto& existing_topic : existing_task.related_topics) {
                        if (base::ToLowerASCII(existing_topic.name) == base::ToLowerASCII(topic.name)) {
                          topic_exists = true;
                          break;
                        }
                      }
                      
                      if (!topic_exists) {
                        existing_task.related_topics.push_back(topic);
                      }
                    }
                    
                    break;
                  }
                }
                
                // Add new task if it doesn't exist
                if (!task_exists) {
                  self->user_tasks_.push_back(detected_task);
                }
              }
              
              // Update active tasks in current context
              self->current_context_.active_tasks.clear();
              for (const auto& task : self->user_tasks_) {
                if (!task.is_completed) {
                  self->current_context_.active_tasks.push_back(task);
                }
              }
            }, self));
      }, this, browsing_activity));
}

void ContextualManager::GenerateContextSuggestions(ContextSuggestionsCallback callback) {
  if (current_context_.active_url.empty()) {
    std::move(callback).Run({});
    return;
  }

  // Build entities string
  std::stringstream entities_stream;
  for (size_t i = 0; i < current_context_.entities.size(); ++i) {
    if (i > 0) entities_stream << ", ";
    entities_stream << current_context_.entities[i].name << " (" << current_context_.entities[i].type << ")";
  }
  std::string entities = entities_stream.str();
  
  // Build topics string
  std::stringstream topics_stream;
  for (size_t i = 0; i < current_context_.topics.size(); ++i) {
    if (i > 0) topics_stream << ", ";
    topics_stream << current_context_.topics[i].name;
  }
  std::string topics = topics_stream.str();
  
  // Build active tasks string
  std::stringstream tasks_stream;
  for (size_t i = 0; i < current_context_.active_tasks.size(); ++i) {
    if (i > 0) tasks_stream << "; ";
    tasks_stream << current_context_.active_tasks[i].name << ": " << current_context_.active_tasks[i].description;
  }
  std::string active_tasks = tasks_stream.str();
  
  // Get user interests from context manager
  context_manager_->GetUserContext(
      base::BindOnce([](
          ContextualManager* self,
          std::string current_url,
          std::string current_title,
          std::string entities,
          std::string topics,
          std::string active_tasks,
          ContextSuggestionsCallback callback,
          const asol::core::ContextManager::UserContext& user_context) {
        // Build user interests string
        std::stringstream interests_stream;
        for (size_t i = 0; i < user_context.interests.size(); ++i) {
          if (i > 0) interests_stream << ", ";
          interests_stream << user_context.interests[i];
        }
        std::string user_interests = interests_stream.str();
        
        // Prepare AI prompt
        std::string prompt = kContextSuggestionsPrompt;
        base::ReplaceSubstringsAfterOffset(&prompt, 0, "{current_url}", current_url);
        base::ReplaceSubstringsAfterOffset(&prompt, 0, "{current_title}", current_title);
        base::ReplaceSubstringsAfterOffset(&prompt, 0, "{entities}", entities);
        base::ReplaceSubstringsAfterOffset(&prompt, 0, "{topics}", topics);
        base::ReplaceSubstringsAfterOffset(&prompt, 0, "{active_tasks}", active_tasks);
        base::ReplaceSubstringsAfterOffset(&prompt, 0, "{user_interests}", user_interests);
        
        // Request AI suggestions
        self->ai_service_manager_->GetTextAdapter()->GenerateText(
            prompt,
            base::BindOnce([](
                ContextualManager* self,
                ContextSuggestionsCallback callback,
                const asol::core::TextAdapter::GenerateTextResult& text_result) {
              if (!text_result.success) {
                std::move(callback).Run({});
                return;
              }
              
              // Parse AI response
              absl::optional<base::Value> json = base::JSONReader::Read(text_result.text);
              if (!json || !json->is_dict()) {
                std::move(callback).Run({});
                return;
              }
              
              // Extract suggestions
              const base::Value::List* suggestions_list = json->GetDict().FindList("suggestions");
              if (!suggestions_list) {
                std::move(callback).Run({});
                return;
              }
              
              // Process suggestions
              std::vector<ContextSuggestion> context_suggestions;
              
              for (const auto& suggestion_value : *suggestions_list) {
                if (!suggestion_value.is_dict()) continue;
                
                const base::Value::Dict& suggestion_dict = suggestion_value.GetDict();
                
                std::string title = suggestion_dict.FindString("title").value_or("Unnamed Suggestion");
                std::string description = suggestion_dict.FindString("description").value_or("");
                std::string type_str = suggestion_dict.FindString("type").value_or("CONTENT");
                std::string action_url = suggestion_dict.FindString("action_url").value_or("");
                float relevance_score = suggestion_dict.FindDouble("relevance_score").value_or(0.5);
                
                // Skip low relevance suggestions
                if (relevance_score < 0.6) {
                  continue;
                }
                
                // Create suggestion
                ContextSuggestion suggestion;
                suggestion.id = GenerateUniqueId("suggestion");
                suggestion.title = title;
                suggestion.description = description;
                suggestion.action_url = action_url;
                suggestion.relevance_score = relevance_score;
                
                // Parse type
                if (type_str == "NAVIGATION") {
                  suggestion.type = ContextSuggestion::Type::NAVIGATION;
                } else if (type_str == "SEARCH") {
                  suggestion.type = ContextSuggestion::Type::SEARCH;
                } else if (type_str == "TOOL") {
                  suggestion.type = ContextSuggestion::Type::TOOL;
                } else if (type_str == "REMINDER") {
                  suggestion.type = ContextSuggestion::Type::REMINDER;
                } else {
                  suggestion.type = ContextSuggestion::Type::CONTENT;
                }
                
                // Extract metadata
                const base::Value::Dict* metadata_dict = suggestion_dict.FindDict("metadata");
                if (metadata_dict) {
                  for (auto it = metadata_dict->begin(); it != metadata_dict->end(); ++it) {
                    if (it->second.is_string()) {
                      suggestion.metadata[it->first] = it->second.GetString();
                    }
                  }
                }
                
                context_suggestions.push_back(suggestion);
              }
              
              // Sort by relevance score (descending)
              std::sort(context_suggestions.begin(), context_suggestions.end(),
                      [](const ContextSuggestion& a, const ContextSuggestion& b) {
                        return a.relevance_score > b.relevance_score;
                      });
              
              std::move(callback).Run(context_suggestions);
            }, self, std::move(callback)));
      }, this, current_context_.active_url, current_context_.active_tab_title, 
      entities, topics, active_tasks, std::move(callback)));
}

}  // namespace ui
}  // namespace browser_core