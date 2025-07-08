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
#include "browser_core/ui/memory_palace.h"
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
    if (prompt.find("Summarize") != std::string::npos) {
      // Summary response
      result.text = "This page discusses the latest advances in artificial intelligence research, including breakthroughs in natural language processing and machine learning.";
    } else if (prompt.find("cluster") != std::string::npos) {
      // Clustering response
      result.text = R"({
        "clusters": [
          {
            "name": "AI Research",
            "description": "Pages about artificial intelligence research and advancements",
            "item_indices": [0, 1, 2],
            "topics": ["Artificial Intelligence", "Machine Learning", "Deep Learning"],
            "relevance_score": 0.95
          },
          {
            "name": "Online Shopping",
            "description": "E-commerce and product pages",
            "item_indices": [3, 4],
            "topics": ["Shopping", "E-commerce", "Products"],
            "relevance_score": 0.9
          },
          {
            "name": "Tech News",
            "description": "Technology news articles",
            "item_indices": [5, 6],
            "topics": ["News", "Technology", "Current Events"],
            "relevance_score": 0.85
          }
        ]
      })";
    } else if (prompt.find("journey") != std::string::npos) {
      // Journey response
      result.text = R"({
        "name": "AI Learning Path",
        "description": "A journey through AI concepts from basic to advanced",
        "item_indices": [0, 1, 2],
        "goal": "Learn about artificial intelligence"
      })";
    } else if (prompt.find("search") != std::string::npos) {
      // Search response
      result.text = R"({
        "results": [
          {
            "index": 0,
            "relevance_score": 0.95,
            "match_reason": "Direct topic match with query"
          },
          {
            "index": 1,
            "relevance_score": 0.85,
            "match_reason": "Related to query topic"
          },
          {
            "index": 2,
            "relevance_score": 0.75,
            "match_reason": "Partial match with query"
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

// Helper function to print memory items
void PrintMemoryItems(const std::vector<browser_core::ui::MemoryPalace::MemoryItem>& items) {
  std::cout << "Memory Items:" << std::endl;
  if (items.empty()) {
    std::cout << "  No items found." << std::endl;
    return;
  }
  
  for (const auto& item : items) {
    std::cout << "Title: " << item.title << std::endl;
    std::cout << "  URL: " << item.url << std::endl;
    std::cout << "  Time: " << FormatTimestamp(item.timestamp) << std::endl;
    std::cout << "  Summary: " << item.summary << std::endl;
    
    std::cout << "  Topics: ";
    for (const auto& topic : item.topics) {
      std::cout << topic << ", ";
    }
    std::cout << std::endl;
    
    std::cout << "  Importance: " << item.importance_score << std::endl;
    std::cout << "  Bookmarked: " << (item.is_bookmarked ? "Yes" : "No") << std::endl;
    std::cout << std::endl;
  }
}

// Helper function to print memory clusters
void PrintMemoryClusters(const std::vector<browser_core::ui::MemoryPalace::MemoryCluster>& clusters) {
  std::cout << "Memory Clusters:" << std::endl;
  if (clusters.empty()) {
    std::cout << "  No clusters found." << std::endl;
    return;
  }
  
  for (const auto& cluster : clusters) {
    std::cout << "Cluster: " << cluster.name << " (ID: " << cluster.id << ")" << std::endl;
    std::cout << "  Description: " << cluster.description << std::endl;
    std::cout << "  Time Range: " << FormatTimestamp(cluster.start_time) << " to " 
              << FormatTimestamp(cluster.end_time) << std::endl;
    
    std::cout << "  Topics: ";
    for (const auto& topic : cluster.topics) {
      std::cout << topic << ", ";
    }
    std::cout << std::endl;
    
    std::cout << "  Relevance: " << cluster.relevance_score << std::endl;
    std::cout << "  Items: " << cluster.items.size() << std::endl;
    std::cout << std::endl;
  }
}

// Helper function to print memory journey
void PrintMemoryJourney(bool success, const browser_core::ui::MemoryPalace::MemoryJourney& journey) {
  std::cout << "Memory Journey (success: " << (success ? "true" : "false") << "):" << std::endl;
  if (!success) {
    std::cout << "  Failed to create journey." << std::endl;
    return;
  }
  
  std::cout << "Journey: " << journey.name << " (ID: " << journey.id << ")" << std::endl;
  std::cout << "  Description: " << journey.description << std::endl;
  std::cout << "  Goal: " << journey.goal << std::endl;
  std::cout << "  Time Range: " << FormatTimestamp(journey.start_time) << " to " 
            << FormatTimestamp(journey.end_time) << std::endl;
  std::cout << "  Items: " << journey.items.size() << std::endl;
  
  for (size_t i = 0; i < journey.items.size(); ++i) {
    const auto& item = journey.items[i];
    std::cout << "    " << (i + 1) << ". " << item.title << std::endl;
  }
  
  std::cout << std::endl;
}

// Helper function to print search results
void PrintSearchResults(const browser_core::ui::MemoryPalace::MemorySearchResult& result) {
  std::cout << "Search Results (success: " << (result.success ? "true" : "false") << "):" << std::endl;
  if (!result.success) {
    std::cout << "  Error: " << result.error_message << std::endl;
    return;
  }
  
  std::cout << "  Found " << result.items.size() << " items and " 
            << result.clusters.size() << " clusters." << std::endl;
  
  if (!result.items.empty()) {
    std::cout << "  Top Items:" << std::endl;
    for (size_t i = 0; i < std::min(result.items.size(), size_t(3)); ++i) {
      const auto& item = result.items[i];
      std::cout << "    " << (i + 1) << ". " << item.title << std::endl;
      std::cout << "       URL: " << item.url << std::endl;
      std::cout << "       Summary: " << item.summary << std::endl;
    }
  }
  
  if (!result.clusters.empty()) {
    std::cout << "  Related Clusters:" << std::endl;
    for (size_t i = 0; i < result.clusters.size(); ++i) {
      const auto& cluster = result.clusters[i];
      std::cout << "    " << (i + 1) << ". " << cluster.name << std::endl;
      std::cout << "       Description: " << cluster.description << std::endl;
    }
  }
  
  std::cout << std::endl;
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
  
  // Create and initialize memory palace
  browser_core::ui::MemoryPalace memory_palace;
  bool init_success = memory_palace.Initialize(
      &browser_engine, &ai_service_manager, &context_manager, &content_understanding);
  
  if (!init_success) {
    std::cerr << "Failed to initialize memory palace" << std::endl;
    return 1;
  }
  
  std::cout << "=== Memory Palace Example ===" << std::endl;
  std::cout << std::endl;
  
  // Add some sample browsing history
  std::cout << "Adding sample browsing history..." << std::endl;
  
  // Create timestamps with increasing times
  auto now = std::chrono::system_clock::now();
  auto one_hour_ago = now - std::chrono::hours(1);
  auto two_hours_ago = now - std::chrono::hours(2);
  auto three_hours_ago = now - std::chrono::hours(3);
  auto four_hours_ago = now - std::chrono::hours(4);
  auto five_hours_ago = now - std::chrono::hours(5);
  auto six_hours_ago = now - std::chrono::hours(6);
  
  // Add sample pages
  memory_palace.RecordPageVisit(
      "https://example.com/ai-research",
      "AI Research - Latest Advances",
      "This page discusses the latest advances in artificial intelligence research, "
      "including breakthroughs in natural language processing, computer vision, and "
      "reinforcement learning. Recent developments in transformer models have "
      "significantly improved language understanding capabilities.");
  
  memory_palace.RecordPageVisit(
      "https://example.com/machine-learning-intro",
      "Introduction to Machine Learning",
      "An introduction to machine learning concepts and techniques. This page covers "
      "supervised learning, unsupervised learning, and reinforcement learning. "
      "It also discusses popular algorithms like decision trees, neural networks, "
      "and support vector machines.");
  
  memory_palace.RecordPageVisit(
      "https://example.com/deep-learning-tutorial",
      "Deep Learning Tutorial with Python",
      "A comprehensive tutorial on deep learning using Python and popular frameworks "
      "like TensorFlow and PyTorch. This guide walks through building neural networks "
      "from scratch and applying them to real-world problems.");
  
  memory_palace.RecordPageVisit(
      "https://example.com/shopping/electronics",
      "Electronics - Online Store",
      "Browse our selection of the latest electronics, including smartphones, laptops, "
      "tablets, and accessories. Find great deals on top brands with fast shipping "
      "and easy returns.");
  
  memory_palace.RecordPageVisit(
      "https://example.com/shopping/deals",
      "Today's Best Deals - Limited Time Offers",
      "Check out today's best deals across all product categories. Limited-time offers "
      "with significant discounts on popular items. New deals added daily.");
  
  memory_palace.RecordPageVisit(
      "https://example.com/news/tech",
      "Technology News - Latest Updates",
      "Stay up to date with the latest technology news and developments. Coverage of "
      "industry trends, product launches, and innovations from leading tech companies.");
  
  memory_palace.RecordPageVisit(
      "https://example.com/news/science",
      "Science News - Recent Discoveries",
      "Recent scientific discoveries and breakthroughs across various fields including "
      "physics, biology, chemistry, and astronomy. In-depth coverage of research "
      "findings and their implications.");
  
  // Wait for async operations to complete
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  std::cout << std::endl;
  
  // Test 1: Get memory clusters
  std::cout << "Test 1: Memory Clusters" << std::endl;
  memory_palace.GetMemoryClusters(base::BindOnce([](
      const std::vector<browser_core::ui::MemoryPalace::MemoryCluster>& clusters) {
    PrintMemoryClusters(clusters);
  }));
  
  // Wait for async operations to complete
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  std::cout << std::endl;
  
  // Test 2: Search memory
  std::cout << "Test 2: Memory Search" << std::endl;
  memory_palace.SearchMemory("artificial intelligence", base::BindOnce([](
      const browser_core::ui::MemoryPalace::MemorySearchResult& result) {
    PrintSearchResults(result);
  }));
  
  // Wait for async operations to complete
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  std::cout << std::endl;
  
  // Test 3: Create memory journey
  std::cout << "Test 3: Memory Journey" << std::endl;
  memory_palace.CreateMemoryJourney("Learn about artificial intelligence", base::BindOnce([](
      bool success, const browser_core::ui::MemoryPalace::MemoryJourney& journey) {
    PrintMemoryJourney(success, journey);
  }));
  
  // Wait for async operations to complete
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  std::cout << std::endl;
  
  // Test 4: Search by topic
  std::cout << "Test 4: Search by Topic" << std::endl;
  memory_palace.SearchMemoryByTopic("Shopping", base::BindOnce([](
      const browser_core::ui::MemoryPalace::MemorySearchResult& result) {
    PrintSearchResults(result);
  }));
  
  // Wait for async operations to complete
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  std::cout << std::endl;
  
  std::cout << "Example completed." << std::endl;
  return 0;
}