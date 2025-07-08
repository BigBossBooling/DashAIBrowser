// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_CORE_UI_CONTEXTUAL_MANAGER_H_
#define BROWSER_CORE_UI_CONTEXTUAL_MANAGER_H_

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <chrono>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "browser_core/ai/content_understanding.h"
#include "browser_core/engine/browser_engine.h"
#include "asol/core/ai_service_manager.h"
#include "asol/core/context_manager.h"

namespace browser_core {
namespace ui {

// ContextualManager provides intelligent context-aware browsing assistance
// by understanding user's current tasks, interests, and browsing patterns.
class ContextualManager {
 public:
  // Context entity representing a recognized entity in the current context
  struct ContextEntity {
    std::string name;
    std::string type;
    float relevance_score;
    std::string description;
  };

  // Context topic representing a recognized topic in the current context
  struct ContextTopic {
    std::string name;
    float relevance_score;
    std::vector<std::string> related_topics;
  };

  // User task representing a detected user activity or goal
  struct UserTask {
    std::string id;
    std::string name;
    std::string description;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point last_activity_time;
    float confidence_score;
    std::vector<std::string> related_urls;
    std::vector<ContextTopic> related_topics;
    bool is_completed;
  };

  // Context snapshot representing the current browsing context
  struct ContextSnapshot {
    std::string active_url;
    std::string active_tab_title;
    std::vector<ContextEntity> entities;
    std::vector<ContextTopic> topics;
    std::vector<UserTask> active_tasks;
    std::map<std::string, std::string> metadata;
    std::chrono::system_clock::time_point timestamp;
  };

  // Context suggestion representing a suggested action based on context
  struct ContextSuggestion {
    enum class Type {
      NAVIGATION,
      SEARCH,
      CONTENT,
      TOOL,
      REMINDER
    };

    std::string id;
    std::string title;
    std::string description;
    Type type;
    std::string action_url;
    float relevance_score;
    std::map<std::string, std::string> metadata;
  };

  // Callback for context snapshot
  using ContextSnapshotCallback = 
      base::OnceCallback<void(const ContextSnapshot&)>;

  // Callback for context suggestions
  using ContextSuggestionsCallback = 
      base::OnceCallback<void(const std::vector<ContextSuggestion>&)>;

  // Callback for user tasks
  using UserTasksCallback = 
      base::OnceCallback<void(const std::vector<UserTask>&)>;

  ContextualManager();
  ~ContextualManager();

  // Disallow copy and assign
  ContextualManager(const ContextualManager&) = delete;
  ContextualManager& operator=(const ContextualManager&) = delete;

  // Initialize with browser engine, AI service manager, and content understanding
  bool Initialize(BrowserEngine* browser_engine, 
                asol::core::AIServiceManager* ai_service_manager,
                asol::core::ContextManager* context_manager,
                ai::ContentUnderstanding* content_understanding);

  // Update context with current page information
  void UpdateContext(const std::string& url, 
                   const std::string& title,
                   const std::string& content);

  // Get current context snapshot
  void GetContextSnapshot(ContextSnapshotCallback callback);

  // Get context-aware suggestions
  void GetContextSuggestions(ContextSuggestionsCallback callback);

  // Get detected user tasks
  void GetUserTasks(UserTasksCallback callback);

  // Create a new user task
  void CreateUserTask(const std::string& name, 
                    const std::string& description,
                    UserTasksCallback callback);

  // Complete a user task
  void CompleteUserTask(const std::string& task_id, UserTasksCallback callback);

  // Enable/disable contextual manager
  void Enable(bool enable);
  bool IsEnabled() const;

  // Get a weak pointer to this instance
  base::WeakPtr<ContextualManager> GetWeakPtr();

 private:
  // Helper methods
  void AnalyzePageContent(const std::string& url,
                        const std::string& title,
                        const std::string& content);
  
  void DetectUserTasks();
  
  void GenerateContextSuggestions(ContextSuggestionsCallback callback);

  // Components
  BrowserEngine* browser_engine_ = nullptr;
  asol::core::AIServiceManager* ai_service_manager_ = nullptr;
  asol::core::ContextManager* context_manager_ = nullptr;
  ai::ContentUnderstanding* content_understanding_ = nullptr;

  // State
  bool is_enabled_ = true;
  ContextSnapshot current_context_;
  std::vector<UserTask> user_tasks_;
  std::vector<ContextSnapshot> context_history_;

  // For weak pointers
  base::WeakPtrFactory<ContextualManager> weak_ptr_factory_{this};
};

}  // namespace ui
}  // namespace browser_core

#endif  // BROWSER_CORE_UI_CONTEXTUAL_MANAGER_H_// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_CORE_UI_CONTEXTUAL_MANAGER_H_
#define BROWSER_CORE_UI_CONTEXTUAL_MANAGER_H_

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <chrono>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "browser_core/ai/content_understanding.h"
#include "browser_core/engine/browser_engine.h"
#include "asol/core/ai_service_manager.h"
#include "asol/core/context_manager.h"

namespace browser_core {
namespace ui {

// ContextualManager provides intelligent context-aware browsing assistance
// by understanding user's current tasks, interests, and browsing patterns.
class ContextualManager {
 public:
  // Context entity representing a recognized entity in the current context
  struct ContextEntity {
    std::string name;
    std::string type;
    float relevance_score;
    std::string description;
  };

  // Context topic representing a recognized topic in the current context
  struct ContextTopic {
    std::string name;
    float relevance_score;
    std::vector<std::string> related_topics;
  };

  // User task representing a detected user activity or goal
  struct UserTask {
    std::string id;
    std::string name;
    std::string description;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point last_activity_time;
    float confidence_score;
    std::vector<std::string> related_urls;
    std::vector<ContextTopic> related_topics;
    bool is_completed;
  };

  // Context snapshot representing the current browsing context
  struct ContextSnapshot {
    std::string active_url;
    std::string active_tab_title;
    std::vector<ContextEntity> entities;
    std::vector<ContextTopic> topics;
    std::vector<UserTask> active_tasks;
    std::map<std::string, std::string> metadata;
    std::chrono::system_clock::time_point timestamp;
  };

  // Context suggestion representing a suggested action based on context
  struct ContextSuggestion {
    enum class Type {
      NAVIGATION,
      SEARCH,
      CONTENT,
      TOOL,
      REMINDER
    };

    std::string id;
    std::string title;
    std::string description;
    Type type;
    std::string action_url;
    float relevance_score;
    std::map<std::string, std::string> metadata;
  };

  // Callback for context snapshot
  using ContextSnapshotCallback = 
      base::OnceCallback<void(const ContextSnapshot&)>;

  // Callback for context suggestions
  using ContextSuggestionsCallback = 
      base::OnceCallback<void(const std::vector<ContextSuggestion>&)>;

  // Callback for user tasks
  using UserTasksCallback = 
      base::OnceCallback<void(const std::vector<UserTask>&)>;

  ContextualManager();
  ~ContextualManager();

  // Disallow copy and assign
  ContextualManager(const ContextualManager&) = delete;
  ContextualManager& operator=(const ContextualManager&) = delete;

  // Initialize with browser engine, AI service manager, and content understanding
  bool Initialize(BrowserEngine* browser_engine, 
                asol::core::AIServiceManager* ai_service_manager,
                asol::core::ContextManager* context_manager,
                ai::ContentUnderstanding* content_understanding);

  // Update context with current page information
  void UpdateContext(const std::string& url, 
                   const std::string& title,
                   const std::string& content);

  // Get current context snapshot
  void GetContextSnapshot(ContextSnapshotCallback callback);

  // Get context-aware suggestions
  void GetContextSuggestions(ContextSuggestionsCallback callback);

  // Get detected user tasks
  void GetUserTasks(UserTasksCallback callback);

  // Create a new user task
  void CreateUserTask(const std::string& name, 
                    const std::string& description,
                    UserTasksCallback callback);

  // Complete a user task
  void CompleteUserTask(const std::string& task_id, UserTasksCallback callback);

  // Enable/disable contextual manager
  void Enable(bool enable);
  bool IsEnabled() const;

  // Get a weak pointer to this instance
  base::WeakPtr<ContextualManager> GetWeakPtr();

 private:
  // Helper methods
  void AnalyzePageContent(const std::string& url,
                        const std::string& title,
                        const std::string& content);
  
  void DetectUserTasks();
  
  void GenerateContextSuggestions(ContextSuggestionsCallback callback);

  // Components
  BrowserEngine* browser_engine_ = nullptr;
  asol::core::AIServiceManager* ai_service_manager_ = nullptr;
  asol::core::ContextManager* context_manager_ = nullptr;
  ai::ContentUnderstanding* content_understanding_ = nullptr;

  // State
  bool is_enabled_ = true;
  ContextSnapshot current_context_;
  std::vector<UserTask> user_tasks_;
  std::vector<ContextSnapshot> context_history_;

  // For weak pointers
  base::WeakPtrFactory<ContextualManager> weak_ptr_factory_{this};
};

}  // namespace ui
}  // namespace browser_core

#endif  // BROWSER_CORE_UI_CONTEXTUAL_MANAGER_H_