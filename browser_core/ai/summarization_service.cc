// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser_core/ai/summarization_service.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "asol/core/ai_service_manager.h"
#include "asol/core/privacy_proxy.h"

namespace browser_core {
namespace ai {

namespace {

// Minimum content length for summarization (in characters)
constexpr size_t kMinContentLength = 1000;

// Maximum content length for summarization (in characters)
constexpr size_t kMaxContentLength = 100000;

// Minimum paragraph count for summarization
constexpr int kMinParagraphCount = 3;

// Maximum cache size
constexpr size_t kMaxCacheSize = 50;

// Cache expiration time (24 hours)
constexpr base::TimeDelta kCacheExpirationTime = base::Hours(24);

// Helper function to count paragraphs in content
int CountParagraphs(const std::string& content) {
  std::vector<std::string> paragraphs = base::SplitString(
      content, "\n\n", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  return paragraphs.size();
}

// Helper function to extract paragraphs from content
std::vector<std::string> ExtractParagraphs(const std::string& content) {
  return base::SplitString(
      content, "\n\n", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
}

// Helper function to extract sentences from a paragraph
std::vector<std::string> ExtractSentences(const std::string& paragraph) {
  // Simple sentence splitting (can be improved)
  std::vector<std::string> sentences = base::SplitString(
      paragraph, ". ", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  
  // Add back the period that was removed during splitting
  for (size_t i = 0; i < sentences.size() - 1; ++i) {
    sentences[i] += ".";
  }
  
  // Check if the last sentence already ends with a period
  if (!sentences.empty() && !sentences.back().empty() && 
      sentences.back().back() != '.') {
    sentences.back() += ".";
  }
  
  return sentences;
}

// Helper function to get summary format string
std::string GetSummaryFormatString(
    SummarizationService::SummaryFormat format) {
  switch (format) {
    case SummarizationService::SummaryFormat::EXECUTIVE_SUMMARY:
      return "executive summary";
    case SummarizationService::SummaryFormat::BULLET_POINTS:
      return "bullet points";
    case SummarizationService::SummaryFormat::QA_FORMAT:
      return "question and answer format";
    case SummarizationService::SummaryFormat::TECHNICAL_BRIEF:
      return "technical brief for experts";
    case SummarizationService::SummaryFormat::SIMPLIFIED:
      return "simplified explanation for beginners";
  }
  return "executive summary";  // Default
}

// Helper function to get summary length string
std::string GetSummaryLengthString(
    SummarizationService::SummaryLength length) {
  switch (length) {
    case SummarizationService::SummaryLength::VERY_SHORT:
      return "1-2 sentences";
    case SummarizationService::SummaryLength::SHORT:
      return "1 paragraph";
    case SummarizationService::SummaryLength::MEDIUM:
      return "2-3 paragraphs";
    case SummarizationService::SummaryLength::LONG:
      return "4-5 paragraphs";
  }
  return "2-3 paragraphs";  // Default
}

}  // namespace

SummarizationService::SummarizationService()
    : weak_ptr_factory_(this) {}

SummarizationService::~SummarizationService() = default;

bool SummarizationService::Initialize(
    asol::core::AIServiceManager* ai_service_manager,
    asol::core::PrivacyProxy* privacy_proxy) {
  ai_service_manager_ = ai_service_manager;
  privacy_proxy_ = privacy_proxy;
  return (ai_service_manager_ != nullptr && privacy_proxy_ != nullptr);
}

void SummarizationService::SummarizeContent(
    const std::string& content,
    const std::string& page_url,
    SummaryFormat format,
    SummaryLength length,
    SummarizationCallback callback) {
  // Check if content is summarizable
  if (!IsContentSummarizable(content)) {
    SummaryResult result;
    result.success = false;
    result.error_message = "Content is not suitable for summarization";
    result.timestamp = base::Time::Now();
    std::move(callback).Run(result);
    return;
  }

  // Check cache first
  std::string cache_key = page_url + "_" + 
                        std::to_string(static_cast<int>(format)) + "_" +
                        std::to_string(static_cast<int>(length));
  
  auto cache_it = summary_cache_.find(cache_key);
  if (cache_it != summary_cache_.end()) {
    // Check if cache entry is still valid
    if (base::Time::Now() - cache_it->second.timestamp < kCacheExpirationTime) {
      std::move(callback).Run(cache_it->second);
      return;
    }
    // Cache entry expired, remove it
    summary_cache_.erase(cache_it);
  }

  // Process content through privacy proxy first
  ProcessWithPrivacyProxy(content, page_url, format, length, std::move(callback));
}

void SummarizationService::SummarizeContent(
    const std::string& content,
    const std::string& page_url,
    SummarizationCallback callback) {
  // Use default format and length
  SummarizeContent(content, page_url, SummaryFormat::EXECUTIVE_SUMMARY,
                 SummaryLength::MEDIUM, std::move(callback));
}

bool SummarizationService::IsContentSummarizable(const std::string& content) const {
  // Check content length
  if (content.length() < kMinContentLength || content.length() > kMaxContentLength) {
    return false;
  }

  // Check paragraph count
  if (CountParagraphs(content) < kMinParagraphCount) {
    return false;
  }

  return true;
}

base::WeakPtr<SummarizationService> SummarizationService::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void SummarizationService::ProcessWithPrivacyProxy(
    const std::string& content,
    const std::string& page_url,
    SummaryFormat format,
    SummaryLength length,
    SummarizationCallback callback) {
  // Use privacy proxy to redact any PII before sending to AI service
  privacy_proxy_->ProcessText(
      content,
      base::BindOnce(
          [](base::WeakPtr<SummarizationService> self,
             const std::string& original_content,
             const std::string& page_url,
             SummaryFormat format,
             SummaryLength length,
             SummarizationCallback callback,
             const asol::core::PrivacyProxy::ProcessingResult& privacy_result) {
            if (!self)
              return;
            
            self->ProcessWithAIService(
                privacy_result.processed_text,
                page_url,
                format,
                length,
                std::move(callback));
          },
          weak_ptr_factory_.GetWeakPtr(),
          content,  // Keep original content for source link generation
          page_url,
          format,
          length,
          std::move(callback)));
}

void SummarizationService::ProcessWithAIService(
    const std::string& processed_content,
    const std::string& page_url,
    SummaryFormat format,
    SummaryLength length,
    SummarizationCallback callback) {
  // Format the prompt for the AI service
  std::string prompt = FormatSummaryPrompt(processed_content, format, length);

  // Prepare request parameters
  asol::core::AIServiceManager::AIRequestParams params;
  params.task_type = asol::core::AIServiceManager::TaskType::TEXT_SUMMARIZATION;
  params.input_text = prompt;
  
  // Add metadata about the summary request
  params.custom_params["summary_format"] = GetSummaryFormatString(format);
  params.custom_params["summary_length"] = GetSummaryLengthString(length);
  params.custom_params["page_url"] = page_url;

  // Send request to AI service manager
  ai_service_manager_->ProcessRequest(
      params,
      base::BindOnce(
          [](base::WeakPtr<SummarizationService> self,
             const std::string& original_content,
             const std::string& page_url,
             SummaryFormat format,
             SummaryLength length,
             SummarizationCallback callback,
             bool success,
             const std::string& response) {
            if (!self)
              return;
            
            self->HandleAIResponse(
                original_content,
                page_url,
                format,
                length,
                std::move(callback),
                success,
                response);
          },
          weak_ptr_factory_.GetWeakPtr(),
          processed_content,  // We'll use this as the "original" content
          page_url,
          format,
          length,
          std::move(callback)));
}

void SummarizationService::HandleAIResponse(
    const std::string& original_content,
    const std::string& page_url,
    SummaryFormat format,
    SummaryLength length,
    SummarizationCallback callback,
    bool success,
    const std::string& response) {
  SummaryResult result;
  result.format = format;
  result.length = length;
  result.success = success;
  result.timestamp = base::Time::Now();
  
  if (success) {
    result.summary_text = response;
    
    // Generate source links
    result.source_links = GenerateSourceLinks(original_content, response, page_url);
    
    // Add metadata
    result.metadata["page_url"] = page_url;
    result.metadata["timestamp"] = std::to_string(result.timestamp.ToDoubleT());
    result.metadata["format"] = GetSummaryFormatString(format);
    result.metadata["length"] = GetSummaryLengthString(length);
    
    // Cache the result
    std::string cache_key = page_url + "_" + 
                          std::to_string(static_cast<int>(format)) + "_" +
                          std::to_string(static_cast<int>(length));
    
    // Limit cache size
    if (summary_cache_.size() >= kMaxCacheSize) {
      // Find oldest entry
      auto oldest_it = summary_cache_.begin();
      for (auto it = summary_cache_.begin(); it != summary_cache_.end(); ++it) {
        if (it->second.timestamp < oldest_it->second.timestamp) {
          oldest_it = it;
        }
      }
      summary_cache_.erase(oldest_it);
    }
    
    summary_cache_[cache_key] = result;
  } else {
    result.error_message = "Failed to generate summary: " + response;
  }
  
  std::move(callback).Run(result);
}

std::vector<SummarizationService::SourceLink> SummarizationService::GenerateSourceLinks(
    const std::string& original_content,
    const std::string& summary,
    const std::string& page_url) {
  std::vector<SourceLink> source_links;
  
  // Extract paragraphs from original content
  std::vector<std::string> paragraphs = ExtractParagraphs(original_content);
  
  // Extract sentences from summary
  std::vector<std::string> summary_sentences;
  std::vector<std::string> summary_paragraphs = ExtractParagraphs(summary);
  for (const auto& paragraph : summary_paragraphs) {
    std::vector<std::string> sentences = ExtractSentences(paragraph);
    summary_sentences.insert(summary_sentences.end(), sentences.begin(), sentences.end());
  }
  
  // For each summary sentence, find the most similar sentence in the original content
  for (const auto& summary_sentence : summary_sentences) {
    SourceLink link;
    link.anchor_text = summary_sentence;
    
    // Find the most similar paragraph and sentence
    int best_paragraph_index = 0;
    int best_sentence_index = 0;
    double best_similarity = 0.0;
    
    for (size_t p = 0; p < paragraphs.size(); ++p) {
      std::vector<std::string> sentences = ExtractSentences(paragraphs[p]);
      
      for (size_t s = 0; s < sentences.size(); ++s) {
        // Simple similarity measure (can be improved)
        // In a real implementation, we would use a more sophisticated
        // similarity measure like cosine similarity or semantic similarity
        
        // For now, just use a simple word overlap measure
        std::vector<std::string> summary_words = base::SplitString(
            base::ToLowerASCII(summary_sentence), " ", base::TRIM_WHITESPACE, 
            base::SPLIT_WANT_NONEMPTY);
        
        std::vector<std::string> sentence_words = base::SplitString(
            base::ToLowerASCII(sentences[s]), " ", base::TRIM_WHITESPACE, 
            base::SPLIT_WANT_NONEMPTY);
        
        // Count common words
        int common_words = 0;
        for (const auto& word : summary_words) {
          if (std::find(sentence_words.begin(), sentence_words.end(), word) != 
              sentence_words.end()) {
            common_words++;
          }
        }
        
        // Calculate similarity as proportion of common words
        double similarity = 0.0;
        if (!summary_words.empty() && !sentence_words.empty()) {
          similarity = static_cast<double>(common_words) / 
                     std::min(summary_words.size(), sentence_words.size());
        }
        
        if (similarity > best_similarity) {
          best_similarity = similarity;
          best_paragraph_index = p;
          best_sentence_index = s;
          link.text_snippet = sentences[s];
        }
      }
    }
    
    // Only add links with reasonable similarity
    if (best_similarity > 0.3) {
      // Create URL fragment (e.g., #p5s2 for paragraph 5, sentence 2)
      link.url_fragment = page_url + "#p" + std::to_string(best_paragraph_index + 1) +
                        "s" + std::to_string(best_sentence_index + 1);
      link.paragraph_index = best_paragraph_index;
      link.sentence_index = best_sentence_index;
      
      source_links.push_back(link);
    }
  }
  
  return source_links;
}

std::string SummarizationService::FormatSummaryPrompt(
    const std::string& content,
    SummaryFormat format,
    SummaryLength length) {
  std::stringstream prompt;
  
  // Base prompt
  prompt << "Please summarize the following content ";
  
  // Add format specification
  prompt << "in " << GetSummaryFormatString(format) << " format ";
  
  // Add length specification
  prompt << "with a length of " << GetSummaryLengthString(length) << ".\n\n";
  
  // Add special instructions based on format
  switch (format) {
    case SummaryFormat::EXECUTIVE_SUMMARY:
      prompt << "Focus on the most important points and key takeaways. ";
      prompt << "The summary should be concise and informative.\n\n";
      break;
    case SummaryFormat::BULLET_POINTS:
      prompt << "Present the main points as bullet points. ";
      prompt << "Each bullet point should be clear and self-contained.\n\n";
      break;
    case SummaryFormat::QA_FORMAT:
      prompt << "Structure the summary as questions and answers. ";
      prompt << "Identify the key questions addressed in the content ";
      prompt << "and provide concise answers.\n\n";
      break;
    case SummaryFormat::TECHNICAL_BRIEF:
      prompt << "This summary is for experts in the field. ";
      prompt << "Use appropriate technical terminology and focus on ";
      prompt << "advanced concepts and details.\n\n";
      break;
    case SummaryFormat::SIMPLIFIED:
      prompt << "This summary is for beginners. ";
      prompt << "Explain concepts in simple terms, avoid jargon, ";
      prompt << "and provide context for technical terms.\n\n";
      break;
  }
  
  // Add the content to summarize
  prompt << "Content to summarize:\n\n" << content;
  
  return prompt.str();
}

}  // namespace ai
}  // namespace browser_core// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser_core/ai/summarization_service.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "asol/core/ai_service_manager.h"
#include "asol/core/privacy_proxy.h"

namespace browser_core {
namespace ai {

namespace {

// Minimum content length for summarization (in characters)
constexpr size_t kMinContentLength = 1000;

// Maximum content length for summarization (in characters)
constexpr size_t kMaxContentLength = 100000;

// Minimum paragraph count for summarization
constexpr int kMinParagraphCount = 3;

// Maximum cache size
constexpr size_t kMaxCacheSize = 50;

// Cache expiration time (24 hours)
constexpr base::TimeDelta kCacheExpirationTime = base::Hours(24);

// Helper function to count paragraphs in content
int CountParagraphs(const std::string& content) {
  std::vector<std::string> paragraphs = base::SplitString(
      content, "\n\n", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  return paragraphs.size();
}

// Helper function to extract paragraphs from content
std::vector<std::string> ExtractParagraphs(const std::string& content) {
  return base::SplitString(
      content, "\n\n", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
}

// Helper function to extract sentences from a paragraph
std::vector<std::string> ExtractSentences(const std::string& paragraph) {
  // Simple sentence splitting (can be improved)
  std::vector<std::string> sentences = base::SplitString(
      paragraph, ". ", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  
  // Add back the period that was removed during splitting
  for (size_t i = 0; i < sentences.size() - 1; ++i) {
    sentences[i] += ".";
  }
  
  // Check if the last sentence already ends with a period
  if (!sentences.empty() && !sentences.back().empty() && 
      sentences.back().back() != '.') {
    sentences.back() += ".";
  }
  
  return sentences;
}

// Helper function to get summary format string
std::string GetSummaryFormatString(
    SummarizationService::SummaryFormat format) {
  switch (format) {
    case SummarizationService::SummaryFormat::EXECUTIVE_SUMMARY:
      return "executive summary";
    case SummarizationService::SummaryFormat::BULLET_POINTS:
      return "bullet points";
    case SummarizationService::SummaryFormat::QA_FORMAT:
      return "question and answer format";
    case SummarizationService::SummaryFormat::TECHNICAL_BRIEF:
      return "technical brief for experts";
    case SummarizationService::SummaryFormat::SIMPLIFIED:
      return "simplified explanation for beginners";
  }
  return "executive summary";  // Default
}

// Helper function to get summary length string
std::string GetSummaryLengthString(
    SummarizationService::SummaryLength length) {
  switch (length) {
    case SummarizationService::SummaryLength::VERY_SHORT:
      return "1-2 sentences";
    case SummarizationService::SummaryLength::SHORT:
      return "1 paragraph";
    case SummarizationService::SummaryLength::MEDIUM:
      return "2-3 paragraphs";
    case SummarizationService::SummaryLength::LONG:
      return "4-5 paragraphs";
  }
  return "2-3 paragraphs";  // Default
}

}  // namespace

SummarizationService::SummarizationService()
    : weak_ptr_factory_(this) {}

SummarizationService::~SummarizationService() = default;

bool SummarizationService::Initialize(
    asol::core::AIServiceManager* ai_service_manager,
    asol::core::PrivacyProxy* privacy_proxy) {
  ai_service_manager_ = ai_service_manager;
  privacy_proxy_ = privacy_proxy;
  return (ai_service_manager_ != nullptr && privacy_proxy_ != nullptr);
}

void SummarizationService::SummarizeContent(
    const std::string& content,
    const std::string& page_url,
    SummaryFormat format,
    SummaryLength length,
    SummarizationCallback callback) {
  // Check if content is summarizable
  if (!IsContentSummarizable(content)) {
    SummaryResult result;
    result.success = false;
    result.error_message = "Content is not suitable for summarization";
    result.timestamp = base::Time::Now();
    std::move(callback).Run(result);
    return;
  }

  // Check cache first
  std::string cache_key = page_url + "_" + 
                        std::to_string(static_cast<int>(format)) + "_" +
                        std::to_string(static_cast<int>(length));
  
  auto cache_it = summary_cache_.find(cache_key);
  if (cache_it != summary_cache_.end()) {
    // Check if cache entry is still valid
    if (base::Time::Now() - cache_it->second.timestamp < kCacheExpirationTime) {
      std::move(callback).Run(cache_it->second);
      return;
    }
    // Cache entry expired, remove it
    summary_cache_.erase(cache_it);
  }

  // Process content through privacy proxy first
  ProcessWithPrivacyProxy(content, page_url, format, length, std::move(callback));
}

void SummarizationService::SummarizeContent(
    const std::string& content,
    const std::string& page_url,
    SummarizationCallback callback) {
  // Use default format and length
  SummarizeContent(content, page_url, SummaryFormat::EXECUTIVE_SUMMARY,
                 SummaryLength::MEDIUM, std::move(callback));
}

bool SummarizationService::IsContentSummarizable(const std::string& content) const {
  // Check content length
  if (content.length() < kMinContentLength || content.length() > kMaxContentLength) {
    return false;
  }

  // Check paragraph count
  if (CountParagraphs(content) < kMinParagraphCount) {
    return false;
  }

  return true;
}

base::WeakPtr<SummarizationService> SummarizationService::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void SummarizationService::ProcessWithPrivacyProxy(
    const std::string& content,
    const std::string& page_url,
    SummaryFormat format,
    SummaryLength length,
    SummarizationCallback callback) {
  // Use privacy proxy to redact any PII before sending to AI service
  privacy_proxy_->ProcessText(
      content,
      base::BindOnce(
          [](base::WeakPtr<SummarizationService> self,
             const std::string& original_content,
             const std::string& page_url,
             SummaryFormat format,
             SummaryLength length,
             SummarizationCallback callback,
             const asol::core::PrivacyProxy::ProcessingResult& privacy_result) {
            if (!self)
              return;
            
            self->ProcessWithAIService(
                privacy_result.processed_text,
                page_url,
                format,
                length,
                std::move(callback));
          },
          weak_ptr_factory_.GetWeakPtr(),
          content,  // Keep original content for source link generation
          page_url,
          format,
          length,
          std::move(callback)));
}

void SummarizationService::ProcessWithAIService(
    const std::string& processed_content,
    const std::string& page_url,
    SummaryFormat format,
    SummaryLength length,
    SummarizationCallback callback) {
  // Format the prompt for the AI service
  std::string prompt = FormatSummaryPrompt(processed_content, format, length);

  // Prepare request parameters
  asol::core::AIServiceManager::AIRequestParams params;
  params.task_type = asol::core::AIServiceManager::TaskType::TEXT_SUMMARIZATION;
  params.input_text = prompt;
  
  // Add metadata about the summary request
  params.custom_params["summary_format"] = GetSummaryFormatString(format);
  params.custom_params["summary_length"] = GetSummaryLengthString(length);
  params.custom_params["page_url"] = page_url;

  // Send request to AI service manager
  ai_service_manager_->ProcessRequest(
      params,
      base::BindOnce(
          [](base::WeakPtr<SummarizationService> self,
             const std::string& original_content,
             const std::string& page_url,
             SummaryFormat format,
             SummaryLength length,
             SummarizationCallback callback,
             bool success,
             const std::string& response) {
            if (!self)
              return;
            
            self->HandleAIResponse(
                original_content,
                page_url,
                format,
                length,
                std::move(callback),
                success,
                response);
          },
          weak_ptr_factory_.GetWeakPtr(),
          processed_content,  // We'll use this as the "original" content
          page_url,
          format,
          length,
          std::move(callback)));
}

void SummarizationService::HandleAIResponse(
    const std::string& original_content,
    const std::string& page_url,
    SummaryFormat format,
    SummaryLength length,
    SummarizationCallback callback,
    bool success,
    const std::string& response) {
  SummaryResult result;
  result.format = format;
  result.length = length;
  result.success = success;
  result.timestamp = base::Time::Now();
  
  if (success) {
    result.summary_text = response;
    
    // Generate source links
    result.source_links = GenerateSourceLinks(original_content, response, page_url);
    
    // Add metadata
    result.metadata["page_url"] = page_url;
    result.metadata["timestamp"] = std::to_string(result.timestamp.ToDoubleT());
    result.metadata["format"] = GetSummaryFormatString(format);
    result.metadata["length"] = GetSummaryLengthString(length);
    
    // Cache the result
    std::string cache_key = page_url + "_" + 
                          std::to_string(static_cast<int>(format)) + "_" +
                          std::to_string(static_cast<int>(length));
    
    // Limit cache size
    if (summary_cache_.size() >= kMaxCacheSize) {
      // Find oldest entry
      auto oldest_it = summary_cache_.begin();
      for (auto it = summary_cache_.begin(); it != summary_cache_.end(); ++it) {
        if (it->second.timestamp < oldest_it->second.timestamp) {
          oldest_it = it;
        }
      }
      summary_cache_.erase(oldest_it);
    }
    
    summary_cache_[cache_key] = result;
  } else {
    result.error_message = "Failed to generate summary: " + response;
  }
  
  std::move(callback).Run(result);
}

std::vector<SummarizationService::SourceLink> SummarizationService::GenerateSourceLinks(
    const std::string& original_content,
    const std::string& summary,
    const std::string& page_url) {
  std::vector<SourceLink> source_links;
  
  // Extract paragraphs from original content
  std::vector<std::string> paragraphs = ExtractParagraphs(original_content);
  
  // Extract sentences from summary
  std::vector<std::string> summary_sentences;
  std::vector<std::string> summary_paragraphs = ExtractParagraphs(summary);
  for (const auto& paragraph : summary_paragraphs) {
    std::vector<std::string> sentences = ExtractSentences(paragraph);
    summary_sentences.insert(summary_sentences.end(), sentences.begin(), sentences.end());
  }
  
  // For each summary sentence, find the most similar sentence in the original content
  for (const auto& summary_sentence : summary_sentences) {
    SourceLink link;
    link.anchor_text = summary_sentence;
    
    // Find the most similar paragraph and sentence
    int best_paragraph_index = 0;
    int best_sentence_index = 0;
    double best_similarity = 0.0;
    
    for (size_t p = 0; p < paragraphs.size(); ++p) {
      std::vector<std::string> sentences = ExtractSentences(paragraphs[p]);
      
      for (size_t s = 0; s < sentences.size(); ++s) {
        // Simple similarity measure (can be improved)
        // In a real implementation, we would use a more sophisticated
        // similarity measure like cosine similarity or semantic similarity
        
        // For now, just use a simple word overlap measure
        std::vector<std::string> summary_words = base::SplitString(
            base::ToLowerASCII(summary_sentence), " ", base::TRIM_WHITESPACE, 
            base::SPLIT_WANT_NONEMPTY);
        
        std::vector<std::string> sentence_words = base::SplitString(
            base::ToLowerASCII(sentences[s]), " ", base::TRIM_WHITESPACE, 
            base::SPLIT_WANT_NONEMPTY);
        
        // Count common words
        int common_words = 0;
        for (const auto& word : summary_words) {
          if (std::find(sentence_words.begin(), sentence_words.end(), word) != 
              sentence_words.end()) {
            common_words++;
          }
        }
        
        // Calculate similarity as proportion of common words
        double similarity = 0.0;
        if (!summary_words.empty() && !sentence_words.empty()) {
          similarity = static_cast<double>(common_words) / 
                     std::min(summary_words.size(), sentence_words.size());
        }
        
        if (similarity > best_similarity) {
          best_similarity = similarity;
          best_paragraph_index = p;
          best_sentence_index = s;
          link.text_snippet = sentences[s];
        }
      }
    }
    
    // Only add links with reasonable similarity
    if (best_similarity > 0.3) {
      // Create URL fragment (e.g., #p5s2 for paragraph 5, sentence 2)
      link.url_fragment = page_url + "#p" + std::to_string(best_paragraph_index + 1) +
                        "s" + std::to_string(best_sentence_index + 1);
      link.paragraph_index = best_paragraph_index;
      link.sentence_index = best_sentence_index;
      
      source_links.push_back(link);
    }
  }
  
  return source_links;
}

std::string SummarizationService::FormatSummaryPrompt(
    const std::string& content,
    SummaryFormat format,
    SummaryLength length) {
  std::stringstream prompt;
  
  // Base prompt
  prompt << "Please summarize the following content ";
  
  // Add format specification
  prompt << "in " << GetSummaryFormatString(format) << " format ";
  
  // Add length specification
  prompt << "with a length of " << GetSummaryLengthString(length) << ".\n\n";
  
  // Add special instructions based on format
  switch (format) {
    case SummaryFormat::EXECUTIVE_SUMMARY:
      prompt << "Focus on the most important points and key takeaways. ";
      prompt << "The summary should be concise and informative.\n\n";
      break;
    case SummaryFormat::BULLET_POINTS:
      prompt << "Present the main points as bullet points. ";
      prompt << "Each bullet point should be clear and self-contained.\n\n";
      break;
    case SummaryFormat::QA_FORMAT:
      prompt << "Structure the summary as questions and answers. ";
      prompt << "Identify the key questions addressed in the content ";
      prompt << "and provide concise answers.\n\n";
      break;
    case SummaryFormat::TECHNICAL_BRIEF:
      prompt << "This summary is for experts in the field. ";
      prompt << "Use appropriate technical terminology and focus on ";
      prompt << "advanced concepts and details.\n\n";
      break;
    case SummaryFormat::SIMPLIFIED:
      prompt << "This summary is for beginners. ";
      prompt << "Explain concepts in simple terms, avoid jargon, ";
      prompt << "and provide context for technical terms.\n\n";
      break;
  }
  
  // Add the content to summarize
  prompt << "Content to summarize:\n\n" << content;
  
  return prompt.str();
}

}  // namespace ai
}  // namespace browser_core