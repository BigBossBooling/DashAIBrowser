// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_CORE_AI_CONTENT_UNDERSTANDING_H_
#define BROWSER_CORE_AI_CONTENT_UNDERSTANDING_H_

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "asol/core/ai_service_manager.h"

namespace browser_core {
namespace ai {

// ContentUnderstanding provides advanced content analysis capabilities.
class ContentUnderstanding {
 public:
  // Entity types
  enum class EntityType {
    PERSON,
    ORGANIZATION,
    LOCATION,
    DATE,
    EVENT,
    PRODUCT,
    CONCEPT,
    OTHER
  };

  // Entity information
  struct Entity {
    std::string name;
    EntityType type;
    std::string description;
    float confidence;
    std::vector<std::pair<int, int>> positions; // Start and end positions in text
  };

  // Topic information
  struct Topic {
    std::string name;
    float relevance;
    std::vector<std::string> related_topics;
  };

  // Sentiment analysis result
  struct SentimentAnalysis {
    enum class Sentiment {
      VERY_NEGATIVE,
      NEGATIVE,
      NEUTRAL,
      POSITIVE,
      VERY_POSITIVE
    };

    Sentiment overall_sentiment;
    float score; // -1.0 to 1.0
    std::unordered_map<std::string, float> aspect_sentiments;
  };

  // Content summary
  struct ContentSummary {
    std::string brief_summary; // 1-2 sentences
    std::string detailed_summary; // Paragraph
    std::vector<std::string> key_points;
    std::string title;
    std::string author;
    std::string published_date;
  };

  // Content analysis result
  struct ContentAnalysisResult {
    bool success;
    ContentSummary summary;
    std::vector<Entity> entities;
    std::vector<Topic> topics;
    SentimentAnalysis sentiment;
    std::string language;
    std::string content_type; // Article, product page, news, etc.
    std::unordered_map<std::string, std::string> metadata;
    std::string error_message;
  };

  // Callback for content analysis
  using ContentAnalysisCallback = 
      base::OnceCallback<void(const ContentAnalysisResult&)>;

  ContentUnderstanding();
  ~ContentUnderstanding();

  // Disallow copy and assign
  ContentUnderstanding(const ContentUnderstanding&) = delete;
  ContentUnderstanding& operator=(const ContentUnderstanding&) = delete;

  // Initialize with AI service manager
  bool Initialize(asol::core::AIServiceManager* ai_service_manager);

  // Analyze content
  void AnalyzeContent(const std::string& content, 
                    ContentAnalysisCallback callback);

  // Extract entities
  void ExtractEntities(const std::string& content,
                     base::OnceCallback<void(const std::vector<Entity>&)> callback);

  // Analyze sentiment
  void AnalyzeSentiment(const std::string& content,
                      base::OnceCallback<void(const SentimentAnalysis&)> callback);

  // Identify topics
  void IdentifyTopics(const std::string& content,
                    base::OnceCallback<void(const std::vector<Topic>&)> callback);

  // Summarize content
  void SummarizeContent(const std::string& content,
                      base::OnceCallback<void(const ContentSummary&)> callback);

  // Detect language
  void DetectLanguage(const std::string& content,
                    base::OnceCallback<void(const std::string&)> callback);

  // Get a weak pointer to this instance
  base::WeakPtr<ContentUnderstanding> GetWeakPtr();

 private:
  // Helper methods
  void ParseEntityResponse(const std::string& response, 
                         std::vector<Entity>* entities);
  
  void ParseSentimentResponse(const std::string& response, 
                            SentimentAnalysis* sentiment);
  
  void ParseTopicsResponse(const std::string& response, 
                         std::vector<Topic>* topics);
  
  void ParseSummaryResponse(const std::string& response, 
                          ContentSummary* summary);

  // AI service manager
  asol::core::AIServiceManager* ai_service_manager_ = nullptr;

  // For weak pointers
  base::WeakPtrFactory<ContentUnderstanding> weak_ptr_factory_{this};
};

}  // namespace ai
}  // namespace browser_core

#endif  // BROWSER_CORE_AI_CONTENT_UNDERSTANDING_H_