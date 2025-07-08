// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser_core/ui/memory_palace.h"

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
constexpr char kMemoryClusteringPrompt[] = 
    "Analyze the following browsing history and suggest logical clusters based on content similarity, "
    "temporal proximity, and semantic relationships. For each suggested cluster, provide a name, "
    "description, main topics, and relevance score (0.0-1.0).\n\n"
    "Browsing history:\n{memory_items}\n\n"
    "Format response as JSON with an array of cluster objects, each containing: "
    "name, description, item_indices (array of integers), topics (array of strings), and relevance_score.";

constexpr char kMemorySearchPrompt[] =
    "Search through the following browsing history for items related to the query: \"{query}\". "
    "Rank results by relevance to the query, considering semantic meaning, not just keyword matching.\n\n"
    "Browsing history:\n{memory_items}\n\n"
    "Format response as JSON with an array of result objects, each containing: "
    "index (integer), relevance_score (float 0.0-1.0), and match_reason (string).";

constexpr char kMemoryJourneyPrompt[] =
    "Create a memory journey through the user's browsing history that helps achieve the goal: \"{goal}\". "
    "A memory journey is a curated sequence of browsing history items that tell a coherent story "
    "or help accomplish a specific task.\n\n"
    "Browsing history:\n{memory_items}\n\n"
    "Format response as JSON with fields: name (string), description (string), "
    "item_indices (array of integers in sequence order), and goal (string).";

// Helper function to format a timestamp
std::string FormatTimestamp(const std::chrono::system_clock::time_point& time_point) {
  std::time_t time = std::chrono::system_clock::to_time_t(time_point);
  std::stringstream ss;
  ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
  return ss.str();
}

// Helper function to generate a unique ID
std::string GenerateUniqueId(const std::string& prefix) {
  static int counter = 0;
  std::stringstream ss;
  ss << prefix << "_" << std::time(nullptr) << "_" << counter++;
  return ss.str();
}

}  // namespace

MemoryPalace::MemoryPalace() = default;
MemoryPalace::~MemoryPalace() = default;

bool MemoryPalace::Initialize(
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
  
  return true;
}

void MemoryPalace::RecordPageVisit(
    const std::string& url, 
    const std::string& title,
    const std::string& content) {
  if (!is_enabled_) {
    return;
  }

  // Check if this URL is already in memory
  auto it = std::find_if(memory_items_.begin(), memory_items_.end(),
                       [&url](const MemoryItem& item) { return item.url == url; });
  
  if (it != memory_items_.end()) {
    // Update existing item
    it->title = title;
    it->timestamp = std::chrono::system_clock::now();
    
    // Re-analyze content if it might have changed
    AnalyzePageContent(url, title, content);
  } else {
    // Create new memory item
    MemoryItem item;
    item.url = url;
    item.title = title;
    item.timestamp = std::chrono::system_clock::now();
    item.importance_score = 0.5f;  // Default importance
    item.is_bookmarked = false;
    
    // Add to memory
    memory_items_.push_back(item);
    
    // Analyze content
    AnalyzePageContent(url, title, content);
  }
  
  // Update memory clusters periodically
  // In a real implementation, this would be throttled and possibly done in the background
  if (memory_items_.size() % 10 == 0) {
    UpdateMemoryClusters();
  }
}

void MemoryPalace::SearchMemory(const std::string& query, MemorySearchCallback callback) {
  if (!is_enabled_) {
    MemorySearchResult empty_result;
    empty_result.success = false;
    empty_result.error_message = "Memory Palace is disabled";
    std::move(callback).Run(empty_result);
    return;
  }

  SearchMemoryInternal(query, nullptr, nullptr, nullptr, std::move(callback));
}

void MemoryPalace::SearchMemoryByTimeRange(
    const std::chrono::system_clock::time_point& start_time,
    const std::chrono::system_clock::time_point& end_time,
    MemorySearchCallback callback) {
  if (!is_enabled_) {
    MemorySearchResult empty_result;
    empty_result.success = false;
    empty_result.error_message = "Memory Palace is disabled";
    std::move(callback).Run(empty_result);
    return;
  }

  SearchMemoryInternal("", &start_time, &end_time, nullptr, std::move(callback));
}

void MemoryPalace::SearchMemoryByTopic(
    const std::string& topic, MemorySearchCallback callback) {
  if (!is_enabled_) {
    MemorySearchResult empty_result;
    empty_result.success = false;
    empty_result.error_message = "Memory Palace is disabled";
    std::move(callback).Run(empty_result);
    return;
  }

  SearchMemoryInternal("", nullptr, nullptr, &topic, std::move(callback));
}

void MemoryPalace::GetMemoryClusters(MemoryClustersCallback callback) {
  if (!is_enabled_) {
    std::move(callback).Run({});
    return;
  }

  if (memory_clusters_.empty()) {
    GenerateMemoryClusters(std::move(callback));
  } else {
    std::move(callback).Run(memory_clusters_);
  }
}

void MemoryPalace::CreateMemoryJourney(
    const std::string& goal, MemoryJourneyCallback callback) {
  if (!is_enabled_ || memory_items_.empty()) {
    MemoryJourney empty_journey;
    std::move(callback).Run(false, empty_journey);
    return;
  }

  // Build memory items list for the prompt
  std::stringstream memory_items_stream;
  for (size_t i = 0; i < memory_items_.size(); ++i) {
    const auto& item = memory_items_[i];
    memory_items_stream << "Index: " << i 
                       << ", Title: \"" << item.title 
                       << "\", URL: " << item.url
                       << ", Time: " << FormatTimestamp(item.timestamp);
    
    if (!item.summary.empty()) {
      memory_items_stream << ", Summary: \"" << item.summary << "\"";
    }
    
    if (!item.topics.empty()) {
      memory_items_stream << ", Topics: ";
      for (size_t j = 0; j < item.topics.size(); ++j) {
        if (j > 0) memory_items_stream << ", ";
        memory_items_stream << item.topics[j];
      }
    }
    
    memory_items_stream << "\n";
  }
  std::string memory_items_str = memory_items_stream.str();

  // Prepare AI prompt
  std::string prompt = kMemoryJourneyPrompt;
  base::ReplaceSubstringsAfterOffset(&prompt, 0, "{goal}", goal);
  base::ReplaceSubstringsAfterOffset(&prompt, 0, "{memory_items}", memory_items_str);

  // Request AI journey creation
  ai_service_manager_->GetTextAdapter()->GenerateText(
      prompt,
      base::BindOnce([](
          MemoryPalace* self,
          std::string goal,
          MemoryJourneyCallback callback,
          const asol::core::TextAdapter::GenerateTextResult& text_result) {
        if (!text_result.success) {
          MemoryJourney empty_journey;
          std::move(callback).Run(false, empty_journey);
          return;
        }

        // Parse AI response
        absl::optional<base::Value> json = base::JSONReader::Read(text_result.text);
        if (!json || !json->is_dict()) {
          MemoryJourney empty_journey;
          std::move(callback).Run(false, empty_journey);
          return;
        }

        // Create journey
        MemoryJourney journey;
        journey.id = GenerateUniqueId("journey");
        journey.goal = goal;
        
        const base::Value::Dict& dict = json->GetDict();
        journey.name = dict.FindString("name").value_or("Unnamed Journey");
        journey.description = dict.FindString("description").value_or("");
        
        // Set time range based on included items
        bool has_items = false;
        
        // Extract item indices
        const base::Value::List* indices_list = dict.FindList("item_indices");
        if (indices_list) {
          for (const auto& index_value : *indices_list) {
            if (index_value.is_int()) {
              int index = index_value.GetInt();
              if (index >= 0 && index < static_cast<int>(self->memory_items_.size())) {
                journey.items.push_back(self->memory_items_[index]);
                
                // Update time range
                if (!has_items || journey.items.back().timestamp < journey.start_time) {
                  journey.start_time = journey.items.back().timestamp;
                }
                if (!has_items || journey.items.back().timestamp > journey.end_time) {
                  journey.end_time = journey.items.back().timestamp;
                }
                has_items = true;
              }
            }
          }
        }
        
        // Store the journey
        self->memory_journeys_[journey.id] = journey;
        
        std::move(callback).Run(true, journey);
      }, this, goal, std::move(callback)));
}

void MemoryPalace::GetMemoryJourney(
    const std::string& journey_id, MemoryJourneyCallback callback) {
  if (!is_enabled_) {
    MemoryJourney empty_journey;
    std::move(callback).Run(false, empty_journey);
    return;
  }

  auto it = memory_journeys_.find(journey_id);
  if (it != memory_journeys_.end()) {
    std::move(callback).Run(true, it->second);
  } else {
    MemoryJourney empty_journey;
    std::move(callback).Run(false, empty_journey);
  }
}

void MemoryPalace::Enable(bool enable) {
  is_enabled_ = enable;
}

bool MemoryPalace::IsEnabled() const {
  return is_enabled_;
}

base::WeakPtr<MemoryPalace> MemoryPalace::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void MemoryPalace::AnalyzePageContent(
    const std::string& url,
    const std::string& title,
    const std::string& content) {
  // Find the memory item
  auto it = std::find_if(memory_items_.begin(), memory_items_.end(),
                       [&url](const MemoryItem& item) { return item.url == url; });
  
  if (it == memory_items_.end()) {
    return;
  }
  
  // Use content understanding to analyze the page
  content_understanding_->AnalyzeContent(
      content,
      base::BindOnce([](
          MemoryPalace* self,
          std::string url,
          const ai::ContentUnderstanding::AnalysisResult& result) {
        if (!result.success) {
          return;
        }
        
        // Find the memory item
        auto it = std::find_if(self->memory_items_.begin(), self->memory_items_.end(),
                             [&url](const MemoryItem& item) { return item.url == url; });
        
        if (it == self->memory_items_.end()) {
          return;
        }
        
        // Update memory item with analysis results
        it->topics.clear();
        for (const auto& topic : result.topics) {
          it->topics.push_back(topic.name);
        }
        
        it->entities.clear();
        for (const auto& entity : result.entities) {
          it->entities.push_back(entity.name);
        }
        
        // Generate a summary if needed
        if (it->summary.empty()) {
          self->ai_service_manager_->GetTextAdapter()->GenerateText(
              "Summarize the following content in 1-2 sentences:\n\n" + result.content,
              base::BindOnce([](
                  MemoryPalace* self,
                  std::string url,
                  const asol::core::TextAdapter::GenerateTextResult& text_result) {
                if (!text_result.success) {
                  return;
                }
                
                // Find the memory item
                auto it = std::find_if(self->memory_items_.begin(), self->memory_items_.end(),
                                     [&url](const MemoryItem& item) { return item.url == url; });
                
                if (it == self->memory_items_.end()) {
                  return;
                }
                
                // Update summary
                it->summary = text_result.text;
              }, self, url));
        }
        
        // Calculate importance score based on content analysis
        float importance = 0.5f;  // Default importance
        
        // Adjust importance based on topics
        if (!result.topics.empty()) {
          // Get user interests from context manager
          self->context_manager_->GetUserContext(
              base::BindOnce([](
                  MemoryPalace* self,
                  std::string url,
                  const std::vector<ai::ContentUnderstanding::Topic>& topics,
                  const asol::core::ContextManager::UserContext& user_context) {
                // Find the memory item
                auto it = std::find_if(self->memory_items_.begin(), self->memory_items_.end(),
                                     [&url](const MemoryItem& item) { return item.url == url; });
                
                if (it == self->memory_items_.end()) {
                  return;
                }
                
                // Calculate importance based on match with user interests
                float importance = 0.5f;
                
                for (const auto& topic : topics) {
                  // Check if topic matches user interests
                  for (const auto& interest : user_context.interests) {
                    if (base::ToLowerASCII(topic.name) == base::ToLowerASCII(interest)) {
                      importance += 0.1f * topic.confidence;
                      break;
                    }
                  }
                }
                
                // Cap importance at 1.0
                importance = std::min(importance, 1.0f);
                
                // Update importance score
                it->importance_score = importance;
              }, self, url, result.topics));
        }
      }, this, url));
}

void MemoryPalace::UpdateMemoryClusters() {
  // This would be called periodically to update memory clusters
  // For simplicity, we'll just regenerate clusters
  GenerateMemoryClusters(base::BindOnce([](
      MemoryPalace* self, const std::vector<MemoryCluster>& clusters) {
    self->memory_clusters_ = clusters;
  }, this));
}

void MemoryPalace::GenerateMemoryClusters(MemoryClustersCallback callback) {
  if (memory_items_.empty()) {
    std::move(callback).Run({});
    return;
  }

  // Build memory items list for the prompt
  std::stringstream memory_items_stream;
  for (size_t i = 0; i < memory_items_.size(); ++i) {
    const auto& item = memory_items_[i];
    memory_items_stream << "Index: " << i 
                       << ", Title: \"" << item.title 
                       << "\", URL: " << item.url
                       << ", Time: " << FormatTimestamp(item.timestamp);
    
    if (!item.summary.empty()) {
      memory_items_stream << ", Summary: \"" << item.summary << "\"";
    }
    
    if (!item.topics.empty()) {
      memory_items_stream << ", Topics: ";
      for (size_t j = 0; j < item.topics.size(); ++j) {
        if (j > 0) memory_items_stream << ", ";
        memory_items_stream << item.topics[j];
      }
    }
    
    memory_items_stream << "\n";
  }
  std::string memory_items_str = memory_items_stream.str();

  // Prepare AI prompt
  std::string prompt = kMemoryClusteringPrompt;
  base::ReplaceSubstringsAfterOffset(&prompt, 0, "{memory_items}", memory_items_str);

  // Request AI clustering
  ai_service_manager_->GetTextAdapter()->GenerateText(
      prompt,
      base::BindOnce([](
          MemoryPalace* self,
          MemoryClustersCallback callback,
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

        // Extract clusters
        const base::Value::List* clusters_list = json->GetDict().FindList("clusters");
        if (!clusters_list) {
          std::move(callback).Run({});
          return;
        }

        // Convert to memory clusters
        std::vector<MemoryCluster> memory_clusters;
        
        for (const auto& cluster_value : *clusters_list) {
          if (!cluster_value.is_dict()) continue;
          
          const base::Value::Dict& cluster_dict = cluster_value.GetDict();
          
          std::string name = cluster_dict.FindString("name").value_or("Unnamed Cluster");
          std::string description = cluster_dict.FindString("description").value_or("");
          float relevance_score = cluster_dict.FindDouble("relevance_score").value_or(0.5);
          
          // Create cluster
          MemoryCluster cluster;
          cluster.id = GenerateUniqueId("cluster");
          cluster.name = name;
          cluster.description = description;
          cluster.relevance_score = relevance_score;
          
          // Extract topics
          const base::Value::List* topics_list = cluster_dict.FindList("topics");
          if (topics_list) {
            for (const auto& topic_value : *topics_list) {
              if (topic_value.is_string()) {
                cluster.topics.push_back(topic_value.GetString());
              }
            }
          }
          
          // Extract item indices
          const base::Value::List* indices_list = cluster_dict.FindList("item_indices");
          if (indices_list) {
            bool has_items = false;
            
            for (const auto& index_value : *indices_list) {
              if (index_value.is_int()) {
                int index = index_value.GetInt();
                if (index >= 0 && index < static_cast<int>(self->memory_items_.size())) {
                  cluster.items.push_back(self->memory_items_[index]);
                  
                  // Update time range
                  if (!has_items || cluster.items.back().timestamp < cluster.start_time) {
                    cluster.start_time = cluster.items.back().timestamp;
                  }
                  if (!has_items || cluster.items.back().timestamp > cluster.end_time) {
                    cluster.end_time = cluster.items.back().timestamp;
                  }
                  has_items = true;
                }
              }
            }
          }
          
          // Skip empty clusters
          if (!cluster.items.empty()) {
            memory_clusters.push_back(cluster);
          }
        }
        
        // Sort clusters by relevance score (descending)
        std::sort(memory_clusters.begin(), memory_clusters.end(),
                [](const MemoryCluster& a, const MemoryCluster& b) {
                  return a.relevance_score > b.relevance_score;
                });
        
        std::move(callback).Run(memory_clusters);
      }, this, std::move(callback)));
}

void MemoryPalace::SearchMemoryInternal(
    const std::string& query,
    const std::chrono::system_clock::time_point* start_time,
    const std::chrono::system_clock::time_point* end_time,
    const std::string* topic,
    MemorySearchCallback callback) {
  if (memory_items_.empty()) {
    MemorySearchResult empty_result;
    empty_result.success = true;
    std::move(callback).Run(empty_result);
    return;
  }

  // First, filter by time range and topic if specified
  std::vector<MemoryItem> filtered_items;
  std::vector<size_t> filtered_indices;
  
  for (size_t i = 0; i < memory_items_.size(); ++i) {
    const auto& item = memory_items_[i];
    bool include = true;
    
    // Filter by time range
    if (start_time && item.timestamp < *start_time) {
      include = false;
    }
    if (end_time && item.timestamp > *end_time) {
      include = false;
    }
    
    // Filter by topic
    if (topic && include) {
      bool has_topic = false;
      for (const auto& item_topic : item.topics) {
        if (base::ToLowerASCII(item_topic).find(base::ToLowerASCII(*topic)) != std::string::npos) {
          has_topic = true;
          break;
        }
      }
      if (!has_topic) {
        include = false;
      }
    }
    
    if (include) {
      filtered_items.push_back(item);
      filtered_indices.push_back(i);
    }
  }
  
  // If no query, return filtered items
  if (query.empty()) {
    MemorySearchResult result;
    result.success = true;
    result.items = filtered_items;
    
    // Add relevant clusters
    for (const auto& cluster : memory_clusters_) {
      bool include = false;
      
      // Check if cluster matches time range
      if ((!start_time || cluster.end_time >= *start_time) &&
          (!end_time || cluster.start_time <= *end_time)) {
        // Check if cluster matches topic
        if (!topic) {
          include = true;
        } else {
          for (const auto& cluster_topic : cluster.topics) {
            if (base::ToLowerASCII(cluster_topic).find(base::ToLowerASCII(*topic)) != std::string::npos) {
              include = true;
              break;
            }
          }
        }
      }
      
      if (include) {
        result.clusters.push_back(cluster);
      }
    }
    
    std::move(callback).Run(result);
    return;
  }
  
  // If filtered items is empty, return empty result
  if (filtered_items.empty()) {
    MemorySearchResult empty_result;
    empty_result.success = true;
    std::move(callback).Run(empty_result);
    return;
  }

  // Build memory items list for the prompt
  std::stringstream memory_items_stream;
  for (size_t i = 0; i < filtered_items.size(); ++i) {
    const auto& item = filtered_items[i];
    memory_items_stream << "Index: " << i 
                       << ", Title: \"" << item.title 
                       << "\", URL: " << item.url
                       << ", Time: " << FormatTimestamp(item.timestamp);
    
    if (!item.summary.empty()) {
      memory_items_stream << ", Summary: \"" << item.summary << "\"";
    }
    
    if (!item.topics.empty()) {
      memory_items_stream << ", Topics: ";
      for (size_t j = 0; j < item.topics.size(); ++j) {
        if (j > 0) memory_items_stream << ", ";
        memory_items_stream << item.topics[j];
      }
    }
    
    memory_items_stream << "\n";
  }
  std::string memory_items_str = memory_items_stream.str();

  // Prepare AI prompt
  std::string prompt = kMemorySearchPrompt;
  base::ReplaceSubstringsAfterOffset(&prompt, 0, "{query}", query);
  base::ReplaceSubstringsAfterOffset(&prompt, 0, "{memory_items}", memory_items_str);

  // Request AI search
  ai_service_manager_->GetTextAdapter()->GenerateText(
      prompt,
      base::BindOnce([](
          MemoryPalace* self,
          std::vector<MemoryItem> filtered_items,
          std::vector<size_t> filtered_indices,
          std::string query,
          MemorySearchCallback callback,
          const asol::core::TextAdapter::GenerateTextResult& text_result) {
        MemorySearchResult result;
        result.success = text_result.success;
        
        if (!text_result.success) {
          result.error_message = "Failed to search memory";
          std::move(callback).Run(result);
          return;
        }

        // Parse AI response
        absl::optional<base::Value> json = base::JSONReader::Read(text_result.text);
        if (!json || !json->is_dict()) {
          result.error_message = "Failed to parse search results";
          std::move(callback).Run(result);
          return;
        }

        // Extract results
        const base::Value::List* results_list = json->GetDict().FindList("results");
        if (!results_list) {
          result.error_message = "No results found";
          std::move(callback).Run(result);
          return;
        }

        // Process results
        std::vector<std::pair<MemoryItem, float>> scored_items;
        
        for (const auto& result_value : *results_list) {
          if (!result_value.is_dict()) continue;
          
          const base::Value::Dict& result_dict = result_value.GetDict();
          
          int index = result_dict.FindInt("index").value_or(-1);
          float relevance_score = result_dict.FindDouble("relevance_score").value_or(0.0);
          
          if (index >= 0 && index < static_cast<int>(filtered_items.size()) && relevance_score > 0.0) {
            scored_items.emplace_back(filtered_items[index], relevance_score);
          }
        }
        
        // Sort by relevance score (descending)
        std::sort(scored_items.begin(), scored_items.end(),
                [](const auto& a, const auto& b) {
                  return a.second > b.second;
                });
        
        // Extract items
        for (const auto& scored_item : scored_items) {
          result.items.push_back(scored_item.first);
        }
        
        // Find relevant clusters
        std::set<size_t> result_indices;
        for (size_t i = 0; i < filtered_indices.size(); ++i) {
          for (const auto& scored_item : scored_items) {
            if (filtered_items[i].url == scored_item.first.url) {
              result_indices.insert(filtered_indices[i]);
              break;
            }
          }
        }
        
        // Add clusters that contain result items
        for (const auto& cluster : self->memory_clusters_) {
          bool include = false;
          
          for (const auto& item : cluster.items) {
            auto it = std::find_if(self->memory_items_.begin(), self->memory_items_.end(),
                                 [&item](const MemoryItem& memory_item) {
                                   return memory_item.url == item.url;
                                 });
            
            if (it != self->memory_items_.end()) {
              size_t index = std::distance(self->memory_items_.begin(), it);
              if (result_indices.find(index) != result_indices.end()) {
                include = true;
                break;
              }
            }
          }
          
          if (include) {
            result.clusters.push_back(cluster);
          }
        }
        
        std::move(callback).Run(result);
      }, this, filtered_items, filtered_indices, query, std::move(callback)));
}

}  // namespace ui
}  // namespace browser_core