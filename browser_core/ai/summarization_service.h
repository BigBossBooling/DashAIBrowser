// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_CORE_AI_SUMMARIZATION_SERVICE_H_
#define BROWSER_CORE_AI_SUMMARIZATION_SERVICE_H_

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"

namespace asol {
namespace core {
class AIServiceManager;
class PrivacyProxy;
}  // namespace core
}  // namespace asol

namespace browser_core {
namespace ai {

// SummarizationService provides AI-powered summarization capabilities for web content.
// It integrates with the ASOL layer to leverage the best AI model for summarization
// while ensuring privacy protection.
class SummarizationService {
 public:
  // Summary format options
  enum class SummaryFormat {
    EXECUTIVE_SUMMARY,  // Concise overview of key points
    BULLET_POINTS,      // Key points in bullet format
    QA_FORMAT,          // Question and answer format
    TECHNICAL_BRIEF,    // Technical summary for experts
    SIMPLIFIED          // Simplified explanation for beginners
  };

  // Summary length options
  enum class SummaryLength {
    VERY_SHORT,         // 1-2 sentences
    SHORT,              // 1 paragraph
    MEDIUM,             // 2-3 paragraphs
    LONG                // 4-5 paragraphs
  };

  // Source link information
  struct SourceLink {
    std::string text_snippet;
    std::string anchor_text;
    std::string url_fragment;
    int paragraph_index;
    int sentence_index;
  };

  // Summary result
  struct SummaryResult {
    std::string summary_text;
    SummaryFormat format;
    SummaryLength length;
    std::vector<SourceLink> source_links;
    std::unordered_map<std::string, std::string> metadata;
    bool success;
    std::string error_message;
    base::Time timestamp;
  };

  // Callback for summarization requests
  using SummarizationCallback = 
      base::OnceCallback<void(const SummaryResult& result)>;

  SummarizationService();
  ~SummarizationService();

  // Disallow copy and assign
  SummarizationService(const SummarizationService&) = delete;
  SummarizationService& operator=(const SummarizationService&) = delete;

  // Initialize the service
  bool Initialize(asol::core::AIServiceManager* ai_service_manager,
                asol::core::PrivacyProxy* privacy_proxy);

  // Summarize content with specified format and length
  void SummarizeContent(const std::string& content,
                      const std::string& page_url,
                      SummaryFormat format,
                      SummaryLength length,
                      SummarizationCallback callback);

  // Summarize content with default settings
  void SummarizeContent(const std::string& content,
                      const std::string& page_url,
                      SummarizationCallback callback);

  // Check if content is summarizable
  bool IsContentSummarizable(const std::string& content) const;

  // Get a weak pointer to this instance
  base::WeakPtr<SummarizationService> GetWeakPtr();

 private:
  // Helper methods
  void ProcessWithPrivacyProxy(const std::string& content,
                             const std::string& page_url,
                             SummaryFormat format,
                             SummaryLength length,
                             SummarizationCallback callback);

  void ProcessWithAIService(const std::string& processed_content,
                          const std::string& page_url,
                          SummaryFormat format,
                          SummaryLength length,
                          SummarizationCallback callback);

  void HandleAIResponse(const std::string& original_content,
                      const std::string& page_url,
                      SummaryFormat format,
                      SummaryLength length,
                      SummarizationCallback callback,
                      bool success,
                      const std::string& response);

  // Generate source links from original content and summary
  std::vector<SourceLink> GenerateSourceLinks(
      const std::string& original_content,
      const std::string& summary,
      const std::string& page_url);

  // Format the AI prompt based on summary format and length
  std::string FormatSummaryPrompt(const std::string& content,
                                SummaryFormat format,
                                SummaryLength length);

  // Components
  asol::core::AIServiceManager* ai_service_manager_ = nullptr;
  asol::core::PrivacyProxy* privacy_proxy_ = nullptr;

  // Cache of recent summaries
  std::unordered_map<std::string, SummaryResult> summary_cache_;

  // For weak pointers
  base::WeakPtrFactory<SummarizationService> weak_ptr_factory_{this};
};

}  // namespace ai
}  // namespace browser_core

#endif  // BROWSER_CORE_AI_SUMMARIZATION_SERVICE_H_// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_CORE_AI_SUMMARIZATION_SERVICE_H_
#define BROWSER_CORE_AI_SUMMARIZATION_SERVICE_H_

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"

namespace asol {
namespace core {
class AIServiceManager;
class PrivacyProxy;
}  // namespace core
}  // namespace asol

namespace browser_core {
namespace ai {

// SummarizationService provides AI-powered summarization capabilities for web content.
// It integrates with the ASOL layer to leverage the best AI model for summarization
// while ensuring privacy protection.
class SummarizationService {
 public:
  // Summary format options
  enum class SummaryFormat {
    EXECUTIVE_SUMMARY,  // Concise overview of key points
    BULLET_POINTS,      // Key points in bullet format
    QA_FORMAT,          // Question and answer format
    TECHNICAL_BRIEF,    // Technical summary for experts
    SIMPLIFIED          // Simplified explanation for beginners
  };

  // Summary length options
  enum class SummaryLength {
    VERY_SHORT,         // 1-2 sentences
    SHORT,              // 1 paragraph
    MEDIUM,             // 2-3 paragraphs
    LONG                // 4-5 paragraphs
  };

  // Source link information
  struct SourceLink {
    std::string text_snippet;
    std::string anchor_text;
    std::string url_fragment;
    int paragraph_index;
    int sentence_index;
  };

  // Summary result
  struct SummaryResult {
    std::string summary_text;
    SummaryFormat format;
    SummaryLength length;
    std::vector<SourceLink> source_links;
    std::unordered_map<std::string, std::string> metadata;
    bool success;
    std::string error_message;
    base::Time timestamp;
  };

  // Callback for summarization requests
  using SummarizationCallback = 
      base::OnceCallback<void(const SummaryResult& result)>;

  SummarizationService();
  ~SummarizationService();

  // Disallow copy and assign
  SummarizationService(const SummarizationService&) = delete;
  SummarizationService& operator=(const SummarizationService&) = delete;

  // Initialize the service
  bool Initialize(asol::core::AIServiceManager* ai_service_manager,
                asol::core::PrivacyProxy* privacy_proxy);

  // Summarize content with specified format and length
  void SummarizeContent(const std::string& content,
                      const std::string& page_url,
                      SummaryFormat format,
                      SummaryLength length,
                      SummarizationCallback callback);

  // Summarize content with default settings
  void SummarizeContent(const std::string& content,
                      const std::string& page_url,
                      SummarizationCallback callback);

  // Check if content is summarizable
  bool IsContentSummarizable(const std::string& content) const;

  // Get a weak pointer to this instance
  base::WeakPtr<SummarizationService> GetWeakPtr();

 private:
  // Helper methods
  void ProcessWithPrivacyProxy(const std::string& content,
                             const std::string& page_url,
                             SummaryFormat format,
                             SummaryLength length,
                             SummarizationCallback callback);

  void ProcessWithAIService(const std::string& processed_content,
                          const std::string& page_url,
                          SummaryFormat format,
                          SummaryLength length,
                          SummarizationCallback callback);

  void HandleAIResponse(const std::string& original_content,
                      const std::string& page_url,
                      SummaryFormat format,
                      SummaryLength length,
                      SummarizationCallback callback,
                      bool success,
                      const std::string& response);

  // Generate source links from original content and summary
  std::vector<SourceLink> GenerateSourceLinks(
      const std::string& original_content,
      const std::string& summary,
      const std::string& page_url);

  // Format the AI prompt based on summary format and length
  std::string FormatSummaryPrompt(const std::string& content,
                                SummaryFormat format,
                                SummaryLength length);

  // Components
  asol::core::AIServiceManager* ai_service_manager_ = nullptr;
  asol::core::PrivacyProxy* privacy_proxy_ = nullptr;

  // Cache of recent summaries
  std::unordered_map<std::string, SummaryResult> summary_cache_;

  // For weak pointers
  base::WeakPtrFactory<SummarizationService> weak_ptr_factory_{this};
};

}  // namespace ai
}  // namespace browser_core

#endif  // BROWSER_CORE_AI_SUMMARIZATION_SERVICE_H_