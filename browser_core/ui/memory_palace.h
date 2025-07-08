// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_CORE_UI_MEMORY_PALACE_H_
#define BROWSER_CORE_UI_MEMORY_PALACE_H_

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

// MemoryPalace provides an intelligent browsing history organization system
// that helps users recall and revisit content based on semantic understanding.
class MemoryPalace {
 public:
  // Memory item representing a visited page
  struct MemoryItem {
    std::string url;
    std::string title;
    std::string summary;
    std::vector<std::string> topics;
    std::vector<std::string> entities;
    std::chrono::system_clock::time_point timestamp;
    float importance_score;
    bool is_bookmarked;
  };

  // Memory cluster representing a group of related memory items
  struct MemoryCluster {
    std::string id;
    std::string name;
    std::string description;
    std::vector<MemoryItem> items;
    std::vector<std::string> topics;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    float relevance_score;
  };

  // Memory journey representing a sequence of related memory items
  struct MemoryJourney {
    std::string id;
    std::string name;
    std::string description;
    std::vector<MemoryItem> items;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    std::string goal;
  };

  // Memory search result
  struct MemorySearchResult {
    bool success;
    std::vector<MemoryItem> items;
    std::vector<MemoryCluster> clusters;
    std::string error_message;
  };

  // Callback for memory search
  using MemorySearchCallback = 
      base::OnceCallback<void(const MemorySearchResult&)>;

  // Callback for memory journey
  using MemoryJourneyCallback = 
      base::OnceCallback<void(bool success, const MemoryJourney&)>;

  // Callback for memory clusters
  using MemoryClustersCallback = 
      base::OnceCallback<void(const std::vector<MemoryCluster>&)>;

  MemoryPalace();
  ~MemoryPalace();

  // Disallow copy and assign
  MemoryPalace(const MemoryPalace&) = delete;
  MemoryPalace& operator=(const MemoryPalace&) = delete;

  // Initialize with browser engine, AI service manager, and content understanding
  bool Initialize(BrowserEngine* browser_engine, 
                asol::core::AIServiceManager* ai_service_manager,
                asol::core::ContextManager* context_manager,
                ai::ContentUnderstanding* content_understanding);

  // Record a page visit
  void RecordPageVisit(const std::string& url, 
                     const std::string& title,
                     const std::string& content);

  // Search memory by query
  void SearchMemory(const std::string& query, MemorySearchCallback callback);

  // Search memory by time range
  void SearchMemoryByTimeRange(
      const std::chrono::system_clock::time_point& start_time,
      const std::chrono::system_clock::time_point& end_time,
      MemorySearchCallback callback);

  // Search memory by topic
  void SearchMemoryByTopic(const std::string& topic, MemorySearchCallback callback);

  // Get memory clusters
  void GetMemoryClusters(MemoryClustersCallback callback);

  // Create a memory journey based on a goal
  void CreateMemoryJourney(const std::string& goal, MemoryJourneyCallback callback);

  // Get a memory journey by ID
  void GetMemoryJourney(const std::string& journey_id, MemoryJourneyCallback callback);

  // Enable/disable memory palace
  void Enable(bool enable);
  bool IsEnabled() const;

  // Get a weak pointer to this instance
  base::WeakPtr<MemoryPalace> GetWeakPtr();

 private:
  // Helper methods
  void AnalyzePageContent(const std::string& url,
                        const std::string& title,
                        const std::string& content);
  
  void UpdateMemoryClusters();
  
  void GenerateMemoryClusters(MemoryClustersCallback callback);
  
  void SearchMemoryInternal(const std::string& query,
                          const std::chrono::system_clock::time_point* start_time,
                          const std::chrono::system_clock::time_point* end_time,
                          const std::string* topic,
                          MemorySearchCallback callback);

  // Components
  BrowserEngine* browser_engine_ = nullptr;
  asol::core::AIServiceManager* ai_service_manager_ = nullptr;
  asol::core::ContextManager* context_manager_ = nullptr;
  ai::ContentUnderstanding* content_understanding_ = nullptr;

  // State
  bool is_enabled_ = true;
  std::vector<MemoryItem> memory_items_;
  std::vector<MemoryCluster> memory_clusters_;
  std::map<std::string, MemoryJourney> memory_journeys_;

  // For weak pointers
  base::WeakPtrFactory<MemoryPalace> weak_ptr_factory_{this};
};

}  // namespace ui
}  // namespace browser_core

#endif  // BROWSER_CORE_UI_MEMORY_PALACE_H_