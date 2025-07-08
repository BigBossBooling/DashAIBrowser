// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <chrono>
#include <iomanip>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/task/single_thread_task_executor.h"
#include "browser_core/engine/browser_engine.h"
#include "browser_core/engine/tab.h"
#include "browser_core/ui/contextual_manager.h"
#include "browser_core/ai/content_understanding.h"
#include "asol/adapters/adapter_factory.h"
#include "asol/core/ai_service_manager.h"
#include "asol/core/context_manager.h"

// Mock implementations for testing
namespace {

// Mock BrowserEngine implementation
class MockBrowserEngine : public browser_core::BrowserEngine {
 public:
  MockBrowserEngine() = default;
  ~MockBrowserEngine() override = default;

  // BrowserEngine implementation
  bool Initialize() override { return true; }
  void Shutdown() override {}
  browser_core::Tab* CreateTab() override { return nullptr; }
  bool CloseTab(int tab_id) override { return true; }
  browser_core::Tab* GetActiveTab() override { return nullptr; }
  browser_core::Tab* GetTabById(int tab_id) override { return nullptr; }
  std::vector<browser_core::Tab*> GetAllTabs() override { return {}; }
  void SetActiveTab(int tab_id) override {}
  void AddTabCreatedCallback(TabCreatedCallback callback) override {}
  void AddTabClosedCallback(TabClosedCallback callback) override {}
  void AddTabActivatedCallback(TabActivatedCallback callback) override {}
};

// Mock ContentUnderstanding implementation
class MockContentUnderstanding : public browser_core::ai::ContentUnderstanding {
 public:
  MockContentUnderstanding() = default;
  ~MockContentUnderstanding() override = default;

  void AnalyzeContent(const std::string& content, AnalysisCallback callback) override {
    AnalysisResult result;
    result.success = true;
    result.content = content;
    
    // Add mock topics based on content
    if (content.find("AI") != std::string::npos || content.find("artificial intelligence") != std::string::npos) {
      Topic ai_topic;
      ai_topic.name = "Artificial Intelligence";
      ai_topic.confidence = 0.95f;
      result.topics.push_back(ai_topic);
    }
    
    if (content.find("machine learning") != std::string::npos || content.find("ML") != std::string::npos) {
      Topic ml_topic;
      ml_topic.name = "Machine Learning";
      ml_topic.confidence = 0.9f;
      result.topics.push_back(ml_topic);
    }
    
    if (content.find("neural") != std::string::npos || content.find("deep learning") != std::string::npos) {
      Topic dl_topic;
      dl_topic.name = "Deep Learning";
      dl_topic.confidence = 0.85f;
      result.topics.push_back(dl_topic);
    }
    
    if (content.find("shopping") != std::string::npos || content.find("product") != std::string::npos) {
      Topic shopping_topic;
      shopping_topic.name = "Shopping";
      shopping_topic.confidence = 0.9f;
      result.topics.push_back(shopping_topic);
    }
    
    if (content.find("news") != std::string::npos || content.find("article") != std::string::npos) {
      Topic news_topic;
      news_topic.name = "News";
      news_topic.confidence = 0.9f;
      result.topics.push_back(news_topic);
    }
    
    // Add mock entities
    if (content.find("Google") != std::string::npos) {
      Entity entity;
      entity.name = "Google";
      entity.type = "Organization";
      entity.confidence = 0.95f;
      result.entities.push_back(entity);
    }
    
    if (content.find("Python") != std::string::npos) {
      Entity entity;
      entity.name = "Python";
      entity.type = "Programming Language";
      entity.confidence = 0.9f;
      result.entities.push_back(entity);
    }
    
    // Add mock sentiment
    result.sentiment.score = 0.7f;  // Positive sentiment
    
    std::move(callback).Run(result);
  }
};

// Mock ContextManager implementation
class MockContextManager : public asol::core::ContextManager {
 public:
  MockContextManager() = default;
  ~MockContextManager() override = default;

  void GetUserContext(GetUserContextCallback callback) override {
    UserContext context;
    context.user_id = "user123";
    context.recent_browsing_summary = "Recently browsed pages about AI technology, "
                                     "machine learning, online shopping, and news articles.";
    context.interests = {"Artificial Intelligence", "Technology", "Online Shopping"};
    context.preferences = {{"theme", "dark"}, {"language", "en"}};
    
    std::move(callback).Run(context);
  }

  void UpdateUserContext(const UserContext& context) override {}
  void ClearUserContext() override {}
};

// Mock TextAdapter implementation
class MockTextAdapter : public asol::core::TextAdapter {
 public:
  MockTextAdapter() = default;
  ~MockTextAdapter() override = default;

  void GenerateText(const std::string& prompt, GenerateTextCallback callback) override {
    GenerateTextResult result;
    result.success = true;
    
    // Simple mock response based on prompt
    if (prompt.find("task") != std::string::npos && prompt.find("detect") != std::string::npos) {
      // Task detection response
      result.text = R"({
        "tasks": [
          {
            "name": "Research AI Technologies",
            "description": "Learning about the latest developments in artificial intelligence and machine learning",
            "confidence_score": 0.95,
            "related_topics": ["Artificial Intelligence", "Machine Learning", "Deep Learning"],
            "related_urls": ["https://example.com/ai-research", "https://example.com/machine-learning-intro"]
          },
          {
            "name": "Online Shopping",
            "description": "Looking for electronics and deals on various products",
            "confidence_score": 0.85,
            "related_topics": ["Shopping", "E-commerce", "Electronics"],
            "related_urls": ["https://example.com/shopping/electronics", "https://example.com/shopping/deals"]
          },
          {
            "name": "Stay Updated on Tech News",
            "description": "Following the latest technology and science news",
            "confidence_score": 0.8,
            "related_topics": ["Technology", "News", "Science"],
            "related_urls": ["https://example.com/news/tech", "https://example.com/news/science"]
          }
        ]
      })";
    } else if (prompt.find("suggest") != std::string::npos) {
      // Context suggestions response
      result.text = R"({
        "suggestions": [
          {
            "title": "Advanced AI Techniques",
            "description": "Explore advanced techniques in artificial intelligence and machine learning",
            "type": "NAVIGATION",
            "action_url": "https://example.com/advanced-ai-techniques",
            "relevance_score": 0.95
          },
          {
            "title": "Compare ML Frameworks",
            "description": "Compare popular machine learning frameworks like TensorFlow, PyTorch, and scikit-learn",
            "type": "CONTENT",
            "action_url": "https://example.com/ml-framework-comparison",
            "relevance_score": 0.9
          },
          {
            "title": "Search for AI Courses",
            "description": "Find online courses to learn more about artificial intelligence",
            "type": "SEARCH",
            "action_url": "https://example.com/search?q=ai+courses",
            "relevance_score": 0.85
          },
          {
            "title": "Save AI Research for Later",
            "description": "Bookmark this page to continue your AI research later",
            "type": "TOOL",
            "action_url": "bookmark://current",
            "relevance_score": 0.8
          },
          {
            "title": "Continue Shopping Research",
            "description": "Return to your shopping research for electronics",
            "type": "REMINDER",
            "action_url": "https://example.com/shopping/electronics",
            "relevance_score": 0.75
          }
        ]
      })";
    } else {
      result.text = "Generated response for: " + prompt.substr(0, 50) + "...";
    }
    
    std::move(callback).Run(result);
  }
};

// Mock AIServiceManager implementation
class MockAIServiceManager : public asol::core::AIServiceManager {
 public:
  MockAIServiceManager() : text_adapter_(std::make_unique<MockTextAdapter>()) {}
  ~MockAIServiceManager() override = default;

  bool Initialize() override { return true; }
  void Shutdown() override {}
  asol::core::TextAdapter* GetTextAdapter() override { return text_adapter_.get(); }
  asol::core::ImageAdapter* GetImageAdapter() override { return nullptr; }
  asol::core::AudioAdapter* GetAudioAdapter() override { return nullptr; }
  asol::core::VideoAdapter* GetVideoAdapter() override { return nullptr; }
  void SetActiveProvider(const std::string& provider_id) override {}
  std::string GetActiveProvider() const override { return "mock_provider"; }
  std::vector<std::string> GetAvailableProviders() const override { return {"mock_provider"}; }

 private:
  std::unique_ptr<MockTextAdapter> text_adapter_;
};

// Helper function to format a timestamp
std::string FormatTimestamp(const std::chrono::system_clock::time_point& time_point) {
  std::time_t time = std::chrono::system_clock::to_time_t(time_point);
  std::stringstream ss;
  ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
  return ss.str();
}

// Helper function to print context snapshot
void PrintContextSnapshot(const browser_core::ui::ContextualManager::ContextSnapshot& snapshot) {
  std::cout << "Context Snapshot:" << std::endl;
  std::cout << "  Active URL: " << snapshot.active_url << std::endl;
  std::cout << "  Active Title: " << snapshot.active_tab_title << std::endl;
  std::cout << "  Timestamp: " << FormatTimestamp(snapshot.timestamp) << std::endl;
  
  std::cout << "  Entities:" << std::endl;
  for (const auto& entity : snapshot.entities) {
    std::cout << "    - " << entity.name << " (" << entity.type << "): " 
              << entity.relevance_score << std::endl;
  }
  
  std::cout << "  Topics:" << std::endl;
  for (const auto& topic : snapshot.topics) {
    std::cout << "    - " << topic.name << ": " << topic.relevance_score << std::endl;
  }
  
  std::cout << "  Active Tasks:" << std::endl;
  for (const auto& task : snapshot.active_tasks) {
    std::cout << "    - " << task.name << ": " << task.description << std::endl;
  }
  
  std::cout << std::endl;
}

// Helper function to print user tasks
void PrintUserTasks(const std::vector<browser_core::ui::ContextualManager::UserTask>& tasks) {
  std::cout << "User Tasks:" << std::endl;
  if (tasks.empty()) {
    std::cout << "  No tasks found." << std::endl;
    return;
  }
  
  for (const auto& task : tasks) {
    std::cout << "Task: " << task.name << " (ID: " << task.id << ")" << std::endl;
    std::cout << "  Description: " << task.description << std::endl;
    std::cout << "  Start Time: " << FormatTimestamp(task.start_time) << std::endl;
    std::cout << "  Last Activity: " << FormatTimestamp(task.last_activity_time) << std::endl;
    std::cout << "  Confidence: " << task.confidence_score << std::endl;
    std::cout << "  Completed: " << (task.is_completed ? "Yes" : "No") << std::endl;
    
    std::cout << "  Related Topics:" << std::endl;
    for (const auto& topic : task.related_topics) {
      std::cout << "    - " << topic.name << ": " << topic.relevance_score << std::endl;
    }
    
    std::cout << "  Related URLs:" << std::endl;
    for (const auto& url : task.related_urls) {
      std::cout << "    - " << url << std::endl;
    }
    
    std::cout << std::endl;
  }
}

// Helper function to print context suggestions
void PrintContextSuggestions(const std::vector<browser_core::ui::ContextualManager::ContextSuggestion>& suggestions) {
  std::cout << "Context Suggestions:" << std::endl;
  if (suggestions.empty()) {
    std::cout << "  No suggestions found." << std::endl;
    return;
  }
  
  for (const auto& suggestion : suggestions) {
    std::cout << "Suggestion: " << suggestion.title << " (ID: " << suggestion.id << ")" << std::endl;
    std::cout << "  Description: " << suggestion.description << std::endl;
    
    std::string type;
    switch (suggestion.type) {
      case browser_core::ui::ContextualManager::ContextSuggestion::Type::NAVIGATION:
        type = "Navigation";
        break;
      case browser_core::ui::ContextualManager::ContextSuggestion::Type::SEARCH:
        type = "Search";
        break;
      case browser_core::ui::ContextualManager::ContextSuggestion::Type::CONTENT:
        type = "Content";
        break;
      case browser_core::ui::ContextualManager::ContextSuggestion::Type::TOOL:
        type = "Tool";
        break;
      case browser_core::ui::ContextualManager::ContextSuggestion::Type::REMINDER:
        type = "Reminder";
        break;
    }
    
    std::cout << "  Type: " << type << std::endl;
    std::cout << "  Action URL: " << suggestion.action_url << std::endl;
    std::cout << "  Relevance: " << suggestion.relevance_score << std::endl;
    
    if (!suggestion.metadata.empty()) {
      std::cout << "  Metadata:" << std::endl;
      for (const auto& [key, value] : suggestion.metadata) {
        std::cout << "    - " << key << ": " << value << std::endl;
      }
    }
    
    std::cout << std::endl;
  }
}

}  // namespace

int main(int argc, char* argv[]) {
  // Initialize base
  base::AtExitManager at_exit_manager;
  base::CommandLine::Init(argc, argv);
  base::SingleThreadTaskExecutor main_task_executor(base::MessagePumpType::UI);
  
  // Create mock components
  MockBrowserEngine browser_engine;
  MockAIServiceManager ai_service_manager;
  MockContextManager context_manager;
  MockContentUnderstanding content_understanding;
  
  // Initialize components
  browser_engine.Initialize();
  ai_service_manager.Initialize();
  
  // Create and initialize contextual manager
  browser_core::ui::ContextualManager contextual_manager;
  bool init_success = contextual_manager.Initialize(
      &browser_engine, &ai_service_manager, &context_manager, &content_understanding);
  
  if (!init_success) {
    std::cerr << "Failed to initialize contextual manager" << std::endl;
    return 1;
  }
  
  std::cout << "=== Contextual Manager Example ===" << std::endl;
  std::cout << std::endl;
  
  // Simulate browsing activity
  std::cout << "Simulating browsing activity..." << std::endl;
  
  // Visit AI research page
  contextual_manager.UpdateContext(
      "https://example.com/ai-research",
      "AI Research - Latest Advances",
      "This page discusses the latest advances in artificial intelligence research, "
      "including breakthroughs in natural language processing, computer vision, and "
      "reinforcement learning. Recent developments in transformer models have "
      "significantly improved language understanding capabilities. Google's research "
      "in this area has been particularly influential.");
  
  // Wait for async operations to complete
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  
  // Visit machine learning page
  contextual_manager.UpdateContext(
      "https://example.com/machine-learning-intro",
      "Introduction to Machine Learning",
      "An introduction to machine learning concepts and techniques. This page covers "
      "supervised learning, unsupervised learning, and reinforcement learning. "
      "It also discusses popular algorithms like decision trees, neural networks, "
      "and support vector machines. Python is the most commonly used language for "
      "machine learning implementations.");
  
  // Wait for async operations to complete
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  
  // Visit shopping page
  contextual_manager.UpdateContext(
      "https://example.com/shopping/electronics",
      "Electronics - Online Store",
      "Browse our selection of the latest electronics, including smartphones, laptops, "
      "tablets, and accessories. Find great deals on top brands with fast shipping "
      "and easy returns. Our product catalog includes items from Apple, Samsung, and Google.");
  
  // Wait for async operations to complete
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  
  // Visit news page
  contextual_manager.UpdateContext(
      "https://example.com/news/tech",
      "Technology News - Latest Updates",
      "Stay up to date with the latest technology news and developments. Coverage of "
      "industry trends, product launches, and innovations from leading tech companies. "
      "Recent articles about artificial intelligence advancements and their impact on society.");
  
  // Wait for async operations to complete
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  
  // Return to AI research (current page)
  contextual_manager.UpdateContext(
      "https://example.com/deep-learning-tutorial",
      "Deep Learning Tutorial with Python",
      "A comprehensive tutorial on deep learning using Python and popular frameworks "
      "like TensorFlow and PyTorch. This guide walks through building neural networks "
      "from scratch and applying them to real-world problems. Learn about convolutional "
      "neural networks, recurrent neural networks, and transformer architectures.");
  
  // Wait for async operations to complete
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  std::cout << std::endl;
  
  // Test 1: Get context snapshot
  std::cout << "Test 1: Context Snapshot" << std::endl;
  contextual_manager.GetContextSnapshot(base::BindOnce([](
      const browser_core::ui::ContextualManager::ContextSnapshot& snapshot) {
    PrintContextSnapshot(snapshot);
  }));
  
  // Wait for async operations to complete
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  std::cout << std::endl;
  
  // Test 2: Get user tasks
  std::cout << "Test 2: User Tasks" << std::endl;
  contextual_manager.GetUserTasks(base::BindOnce([](
      const std::vector<browser_core::ui::ContextualManager::UserTask>& tasks) {
    PrintUserTasks(tasks);
  }));
  
  // Wait for async operations to complete
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  std::cout << std::endl;
  
  // Test 3: Get context suggestions
  std::cout << "Test 3: Context Suggestions" << std::endl;
  contextual_manager.GetContextSuggestions(base::BindOnce([](
      const std::vector<browser_core::ui::ContextualManager::ContextSuggestion>& suggestions) {
    PrintContextSuggestions(suggestions);
  }));
  
  // Wait for async operations to complete
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  std::cout << std::endl;
  
  // Test 4: Create user task
  std::cout << "Test 4: Create User Task" << std::endl;
  contextual_manager.CreateUserTask(
      "Learn TensorFlow",
      "Complete TensorFlow tutorials and build a machine learning model",
      base::BindOnce([](
          const std::vector<browser_core::ui::ContextualManager::UserTask>& tasks) {
        PrintUserTasks(tasks);
      }));
  
  // Wait for async operations to complete
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  std::cout << std::endl;
  
  // Test 5: Complete user task
  std::cout << "Test 5: Complete User Task" << std::endl;
  // Get tasks to find the ID of the first task
  contextual_manager.GetUserTasks(base::BindOnce([](
      browser_core::ui::ContextualManager* manager,
      const std::vector<browser_core::ui::ContextualManager::UserTask>& tasks) {
    if (!tasks.empty()) {
      std::string task_id = tasks[0].id;
      std::cout << "Completing task with ID: " << task_id << std::endl;
      
      manager->CompleteUserTask(task_id, base::BindOnce([](
          const std::vector<browser_core::ui::ContextualManager::UserTask>& updated_tasks) {
        PrintUserTasks(updated_tasks);
      }));
    } else {
      std::cout << "No tasks to complete." << std::endl;
    }
  }, &contextual_manager));
  
  // Wait for async operations to complete
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  std::cout << std::endl;
  
  std::cout << "Example completed." << std::endl;
  return 0;
}// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <chrono>
#include <iomanip>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/task/single_thread_task_executor.h"
#include "browser_core/engine/browser_engine.h"
#include "browser_core/engine/tab.h"
#include "browser_core/ui/contextual_manager.h"
#include "browser_core/ai/content_understanding.h"
#include "asol/adapters/adapter_factory.h"
#include "asol/core/ai_service_manager.h"
#include "asol/core/context_manager.h"

// Mock implementations for testing
namespace {

// Mock BrowserEngine implementation
class MockBrowserEngine : public browser_core::BrowserEngine {
 public:
  MockBrowserEngine() = default;
  ~MockBrowserEngine() override = default;

  // BrowserEngine implementation
  bool Initialize() override { return true; }
  void Shutdown() override {}
  browser_core::Tab* CreateTab() override { return nullptr; }
  bool CloseTab(int tab_id) override { return true; }
  browser_core::Tab* GetActiveTab() override { return nullptr; }
  browser_core::Tab* GetTabById(int tab_id) override { return nullptr; }
  std::vector<browser_core::Tab*> GetAllTabs() override { return {}; }
  void SetActiveTab(int tab_id) override {}
  void AddTabCreatedCallback(TabCreatedCallback callback) override {}
  void AddTabClosedCallback(TabClosedCallback callback) override {}
  void AddTabActivatedCallback(TabActivatedCallback callback) override {}
};

// Mock ContentUnderstanding implementation
class MockContentUnderstanding : public browser_core::ai::ContentUnderstanding {
 public:
  MockContentUnderstanding() = default;
  ~MockContentUnderstanding() override = default;

  void AnalyzeContent(const std::string& content, AnalysisCallback callback) override {
    AnalysisResult result;
    result.success = true;
    result.content = content;
    
    // Add mock topics based on content
    if (content.find("AI") != std::string::npos || content.find("artificial intelligence") != std::string::npos) {
      Topic ai_topic;
      ai_topic.name = "Artificial Intelligence";
      ai_topic.confidence = 0.95f;
      result.topics.push_back(ai_topic);
    }
    
    if (content.find("machine learning") != std::string::npos || content.find("ML") != std::string::npos) {
      Topic ml_topic;
      ml_topic.name = "Machine Learning";
      ml_topic.confidence = 0.9f;
      result.topics.push_back(ml_topic);
    }
    
    if (content.find("neural") != std::string::npos || content.find("deep learning") != std::string::npos) {
      Topic dl_topic;
      dl_topic.name = "Deep Learning";
      dl_topic.confidence = 0.85f;
      result.topics.push_back(dl_topic);
    }
    
    if (content.find("shopping") != std::string::npos || content.find("product") != std::string::npos) {
      Topic shopping_topic;
      shopping_topic.name = "Shopping";
      shopping_topic.confidence = 0.9f;
      result.topics.push_back(shopping_topic);
    }
    
    if (content.find("news") != std::string::npos || content.find("article") != std::string::npos) {
      Topic news_topic;
      news_topic.name = "News";
      news_topic.confidence = 0.9f;
      result.topics.push_back(news_topic);
    }
    
    // Add mock entities
    if (content.find("Google") != std::string::npos) {
      Entity entity;
      entity.name = "Google";
      entity.type = "Organization";
      entity.confidence = 0.95f;
      result.entities.push_back(entity);
    }
    
    if (content.find("Python") != std::string::npos) {
      Entity entity;
      entity.name = "Python";
      entity.type = "Programming Language";
      entity.confidence = 0.9f;
      result.entities.push_back(entity);
    }
    
    // Add mock sentiment
    result.sentiment.score = 0.7f;  // Positive sentiment
    
    std::move(callback).Run(result);
  }
};

// Mock ContextManager implementation
class MockContextManager : public asol::core::ContextManager {
 public:
  MockContextManager() = default;
  ~MockContextManager() override = default;

  void GetUserContext(GetUserContextCallback callback) override {
    UserContext context;
    context.user_id = "user123";
    context.recent_browsing_summary = "Recently browsed pages about AI technology, "
                                     "machine learning, online shopping, and news articles.";
    context.interests = {"Artificial Intelligence", "Technology", "Online Shopping"};
    context.preferences = {{"theme", "dark"}, {"language", "en"}};
    
    std::move(callback).Run(context);
  }

  void UpdateUserContext(const UserContext& context) override {}
  void ClearUserContext() override {}
};

// Mock TextAdapter implementation
class MockTextAdapter : public asol::core::TextAdapter {
 public:
  MockTextAdapter() = default;
  ~MockTextAdapter() override = default;

  void GenerateText(const std::string& prompt, GenerateTextCallback callback) override {
    GenerateTextResult result;
    result.success = true;
    
    // Simple mock response based on prompt
    if (prompt.find("task") != std::string::npos && prompt.find("detect") != std::string::npos) {
      // Task detection response
      result.text = R"({
        "tasks": [
          {
            "name": "Research AI Technologies",
            "description": "Learning about the latest developments in artificial intelligence and machine learning",
            "confidence_score": 0.95,
            "related_topics": ["Artificial Intelligence", "Machine Learning", "Deep Learning"],
            "related_urls": ["https://example.com/ai-research", "https://example.com/machine-learning-intro"]
          },
          {
            "name": "Online Shopping",
            "description": "Looking for electronics and deals on various products",
            "confidence_score": 0.85,
            "related_topics": ["Shopping", "E-commerce", "Electronics"],
            "related_urls": ["https://example.com/shopping/electronics", "https://example.com/shopping/deals"]
          },
          {
            "name": "Stay Updated on Tech News",
            "description": "Following the latest technology and science news",
            "confidence_score": 0.8,
            "related_topics": ["Technology", "News", "Science"],
            "related_urls": ["https://example.com/news/tech", "https://example.com/news/science"]
          }
        ]
      })";
    } else if (prompt.find("suggest") != std::string::npos) {
      // Context suggestions response
      result.text = R"({
        "suggestions": [
          {
            "title": "Advanced AI Techniques",
            "description": "Explore advanced techniques in artificial intelligence and machine learning",
            "type": "NAVIGATION",
            "action_url": "https://example.com/advanced-ai-techniques",
            "relevance_score": 0.95
          },
          {
            "title": "Compare ML Frameworks",
            "description": "Compare popular machine learning frameworks like TensorFlow, PyTorch, and scikit-learn",
            "type": "CONTENT",
            "action_url": "https://example.com/ml-framework-comparison",
            "relevance_score": 0.9
          },
          {
            "title": "Search for AI Courses",
            "description": "Find online courses to learn more about artificial intelligence",
            "type": "SEARCH",
            "action_url": "https://example.com/search?q=ai+courses",
            "relevance_score": 0.85
          },
          {
            "title": "Save AI Research for Later",
            "description": "Bookmark this page to continue your AI research later",
            "type": "TOOL",
            "action_url": "bookmark://current",
            "relevance_score": 0.8
          },
          {
            "title": "Continue Shopping Research",
            "description": "Return to your shopping research for electronics",
            "type": "REMINDER",
            "action_url": "https://example.com/shopping/electronics",
            "relevance_score": 0.75
          }
        ]
      })";
    } else {
      result.text = "Generated response for: " + prompt.substr(0, 50) + "...";
    }
    
    std::move(callback).Run(result);
  }
};

// Mock AIServiceManager implementation
class MockAIServiceManager : public asol::core::AIServiceManager {
 public:
  MockAIServiceManager() : text_adapter_(std::make_unique<MockTextAdapter>()) {}
  ~MockAIServiceManager() override = default;

  bool Initialize() override { return true; }
  void Shutdown() override {}
  asol::core::TextAdapter* GetTextAdapter() override { return text_adapter_.get(); }
  asol::core::ImageAdapter* GetImageAdapter() override { return nullptr; }
  asol::core::AudioAdapter* GetAudioAdapter() override { return nullptr; }
  asol::core::VideoAdapter* GetVideoAdapter() override { return nullptr; }
  void SetActiveProvider(const std::string& provider_id) override {}
  std::string GetActiveProvider() const override { return "mock_provider"; }
  std::vector<std::string> GetAvailableProviders() const override { return {"mock_provider"}; }

 private:
  std::unique_ptr<MockTextAdapter> text_adapter_;
};

// Helper function to format a timestamp
std::string FormatTimestamp(const std::chrono::system_clock::time_point& time_point) {
  std::time_t time = std::chrono::system_clock::to_time_t(time_point);
  std::stringstream ss;
  ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
  return ss.str();
}

// Helper function to print context snapshot
void PrintContextSnapshot(const browser_core::ui::ContextualManager::ContextSnapshot& snapshot) {
  std::cout << "Context Snapshot:" << std::endl;
  std::cout << "  Active URL: " << snapshot.active_url << std::endl;
  std::cout << "  Active Title: " << snapshot.active_tab_title << std::endl;
  std::cout << "  Timestamp: " << FormatTimestamp(snapshot.timestamp) << std::endl;
  
  std::cout << "  Entities:" << std::endl;
  for (const auto& entity : snapshot.entities) {
    std::cout << "    - " << entity.name << " (" << entity.type << "): " 
              << entity.relevance_score << std::endl;
  }
  
  std::cout << "  Topics:" << std::endl;
  for (const auto& topic : snapshot.topics) {
    std::cout << "    - " << topic.name << ": " << topic.relevance_score << std::endl;
  }
  
  std::cout << "  Active Tasks:" << std::endl;
  for (const auto& task : snapshot.active_tasks) {
    std::cout << "    - " << task.name << ": " << task.description << std::endl;
  }
  
  std::cout << std::endl;
}

// Helper function to print user tasks
void PrintUserTasks(const std::vector<browser_core::ui::ContextualManager::UserTask>& tasks) {
  std::cout << "User Tasks:" << std::endl;
  if (tasks.empty()) {
    std::cout << "  No tasks found." << std::endl;
    return;
  }
  
  for (const auto& task : tasks) {
    std::cout << "Task: " << task.name << " (ID: " << task.id << ")" << std::endl;
    std::cout << "  Description: " << task.description << std::endl;
    std::cout << "  Start Time: " << FormatTimestamp(task.start_time) << std::endl;
    std::cout << "  Last Activity: " << FormatTimestamp(task.last_activity_time) << std::endl;
    std::cout << "  Confidence: " << task.confidence_score << std::endl;
    std::cout << "  Completed: " << (task.is_completed ? "Yes" : "No") << std::endl;
    
    std::cout << "  Related Topics:" << std::endl;
    for (const auto& topic : task.related_topics) {
      std::cout << "    - " << topic.name << ": " << topic.relevance_score << std::endl;
    }
    
    std::cout << "  Related URLs:" << std::endl;
    for (const auto& url : task.related_urls) {
      std::cout << "    - " << url << std::endl;
    }
    
    std::cout << std::endl;
  }
}

// Helper function to print context suggestions
void PrintContextSuggestions(const std::vector<browser_core::ui::ContextualManager::ContextSuggestion>& suggestions) {
  std::cout << "Context Suggestions:" << std::endl;
  if (suggestions.empty()) {
    std::cout << "  No suggestions found." << std::endl;
    return;
  }
  
  for (const auto& suggestion : suggestions) {
    std::cout << "Suggestion: " << suggestion.title << " (ID: " << suggestion.id << ")" << std::endl;
    std::cout << "  Description: " << suggestion.description << std::endl;
    
    std::string type;
    switch (suggestion.type) {
      case browser_core::ui::ContextualManager::ContextSuggestion::Type::NAVIGATION:
        type = "Navigation";
        break;
      case browser_core::ui::ContextualManager::ContextSuggestion::Type::SEARCH:
        type = "Search";
        break;
      case browser_core::ui::ContextualManager::ContextSuggestion::Type::CONTENT:
        type = "Content";
        break;
      case browser_core::ui::ContextualManager::ContextSuggestion::Type::TOOL:
        type = "Tool";
        break;
      case browser_core::ui::ContextualManager::ContextSuggestion::Type::REMINDER:
        type = "Reminder";
        break;
    }
    
    std::cout << "  Type: " << type << std::endl;
    std::cout << "  Action URL: " << suggestion.action_url << std::endl;
    std::cout << "  Relevance: " << suggestion.relevance_score << std::endl;
    
    if (!suggestion.metadata.empty()) {
      std::cout << "  Metadata:" << std::endl;
      for (const auto& [key, value] : suggestion.metadata) {
        std::cout << "    - " << key << ": " << value << std::endl;
      }
    }
    
    std::cout << std::endl;
  }
}

}  // namespace

int main(int argc, char* argv[]) {
  // Initialize base
  base::AtExitManager at_exit_manager;
  base::CommandLine::Init(argc, argv);
  base::SingleThreadTaskExecutor main_task_executor(base::MessagePumpType::UI);
  
  // Create mock components
  MockBrowserEngine browser_engine;
  MockAIServiceManager ai_service_manager;
  MockContextManager context_manager;
  MockContentUnderstanding content_understanding;
  
  // Initialize components
  browser_engine.Initialize();
  ai_service_manager.Initialize();
  
  // Create and initialize contextual manager
  browser_core::ui::ContextualManager contextual_manager;
  bool init_success = contextual_manager.Initialize(
      &browser_engine, &ai_service_manager, &context_manager, &content_understanding);
  
  if (!init_success) {
    std::cerr << "Failed to initialize contextual manager" << std::endl;
    return 1;
  }
  
  std::cout << "=== Contextual Manager Example ===" << std::endl;
  std::cout << std::endl;
  
  // Simulate browsing activity
  std::cout << "Simulating browsing activity..." << std::endl;
  
  // Visit AI research page
  contextual_manager.UpdateContext(
      "https://example.com/ai-research",
      "AI Research - Latest Advances",
      "This page discusses the latest advances in artificial intelligence research, "
      "including breakthroughs in natural language processing, computer vision, and "
      "reinforcement learning. Recent developments in transformer models have "
      "significantly improved language understanding capabilities. Google's research "
      "in this area has been particularly influential.");
  
  // Wait for async operations to complete
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  
  // Visit machine learning page
  contextual_manager.UpdateContext(
      "https://example.com/machine-learning-intro",
      "Introduction to Machine Learning",
      "An introduction to machine learning concepts and techniques. This page covers "
      "supervised learning, unsupervised learning, and reinforcement learning. "
      "It also discusses popular algorithms like decision trees, neural networks, "
      "and support vector machines. Python is the most commonly used language for "
      "machine learning implementations.");
  
  // Wait for async operations to complete
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  
  // Visit shopping page
  contextual_manager.UpdateContext(
      "https://example.com/shopping/electronics",
      "Electronics - Online Store",
      "Browse our selection of the latest electronics, including smartphones, laptops, "
      "tablets, and accessories. Find great deals on top brands with fast shipping "
      "and easy returns. Our product catalog includes items from Apple, Samsung, and Google.");
  
  // Wait for async operations to complete
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  
  // Visit news page
  contextual_manager.UpdateContext(
      "https://example.com/news/tech",
      "Technology News - Latest Updates",
      "Stay up to date with the latest technology news and developments. Coverage of "
      "industry trends, product launches, and innovations from leading tech companies. "
      "Recent articles about artificial intelligence advancements and their impact on society.");
  
  // Wait for async operations to complete
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  
  // Return to AI research (current page)
  contextual_manager.UpdateContext(
      "https://example.com/deep-learning-tutorial",
      "Deep Learning Tutorial with Python",
      "A comprehensive tutorial on deep learning using Python and popular frameworks "
      "like TensorFlow and PyTorch. This guide walks through building neural networks "
      "from scratch and applying them to real-world problems. Learn about convolutional "
      "neural networks, recurrent neural networks, and transformer architectures.");
  
  // Wait for async operations to complete
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  std::cout << std::endl;
  
  // Test 1: Get context snapshot
  std::cout << "Test 1: Context Snapshot" << std::endl;
  contextual_manager.GetContextSnapshot(base::BindOnce([](
      const browser_core::ui::ContextualManager::ContextSnapshot& snapshot) {
    PrintContextSnapshot(snapshot);
  }));
  
  // Wait for async operations to complete
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  std::cout << std::endl;
  
  // Test 2: Get user tasks
  std::cout << "Test 2: User Tasks" << std::endl;
  contextual_manager.GetUserTasks(base::BindOnce([](
      const std::vector<browser_core::ui::ContextualManager::UserTask>& tasks) {
    PrintUserTasks(tasks);
  }));
  
  // Wait for async operations to complete
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  std::cout << std::endl;
  
  // Test 3: Get context suggestions
  std::cout << "Test 3: Context Suggestions" << std::endl;
  contextual_manager.GetContextSuggestions(base::BindOnce([](
      const std::vector<browser_core::ui::ContextualManager::ContextSuggestion>& suggestions) {
    PrintContextSuggestions(suggestions);
  }));
  
  // Wait for async operations to complete
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  std::cout << std::endl;
  
  // Test 4: Create user task
  std::cout << "Test 4: Create User Task" << std::endl;
  contextual_manager.CreateUserTask(
      "Learn TensorFlow",
      "Complete TensorFlow tutorials and build a machine learning model",
      base::BindOnce([](
          const std::vector<browser_core::ui::ContextualManager::UserTask>& tasks) {
        PrintUserTasks(tasks);
      }));
  
  // Wait for async operations to complete
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  std::cout << std::endl;
  
  // Test 5: Complete user task
  std::cout << "Test 5: Complete User Task" << std::endl;
  // Get tasks to find the ID of the first task
  contextual_manager.GetUserTasks(base::BindOnce([](
      browser_core::ui::ContextualManager* manager,
      const std::vector<browser_core::ui::ContextualManager::UserTask>& tasks) {
    if (!tasks.empty()) {
      std::string task_id = tasks[0].id;
      std::cout << "Completing task with ID: " << task_id << std::endl;
      
      manager->CompleteUserTask(task_id, base::BindOnce([](
          const std::vector<browser_core::ui::ContextualManager::UserTask>& updated_tasks) {
        PrintUserTasks(updated_tasks);
      }));
    } else {
      std::cout << "No tasks to complete." << std::endl;
    }
  }, &contextual_manager));
  
  // Wait for async operations to complete
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  std::cout << std::endl;
  
  std::cout << "Example completed." << std::endl;
  return 0;
}