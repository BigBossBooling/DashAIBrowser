// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser_core/browser_content_handler.h"

#include <algorithm>
#include <string>
#include <unordered_map>

#include "base/bind.h"
#include "browser_core/features/summarization_feature.h"

namespace browser_core {

namespace {

// Maximum cache size
constexpr size_t kMaxCacheSize = 20;

}  // namespace

BrowserContentHandler::BrowserContentHandler()
    : weak_ptr_factory_(this) {}

BrowserContentHandler::~BrowserContentHandler() = default;

bool BrowserContentHandler::Initialize(BrowserFeatures* browser_features) {
  browser_features_ = browser_features;
  
  // Initialize content extractor
  content_extractor_ = std::make_unique<content::ContentExtractor>();
  if (!content_extractor_->Initialize()) {
    return false;
  }
  
  return true;
}

void BrowserContentHandler::ProcessPage(
    const std::string& page_url,
    const std::string& html_content,
    views::View* toolbar_view,
    views::Widget* browser_widget,
    ProcessingCallback callback) {
  // Check cache first
  auto cache_it = page_cache_.find(page_url);
  if (cache_it != page_cache_.end()) {
    std::move(callback).Run(cache_it->second);
    return;
  }
  
  // Extract content
  content_extractor_->ExtractContent(
      page_url,
      html_content,
      base::BindOnce(&BrowserContentHandler::OnContentExtracted,
                   weak_ptr_factory_.GetWeakPtr(),
                   page_url,
                   toolbar_view,
                   browser_widget,
                   std::move(callback)));
}

BrowserContentHandler::ProcessingResult BrowserContentHandler::ProcessPageSync(
    const std::string& page_url,
    const std::string& html_content) {
  // Check cache first
  auto cache_it = page_cache_.find(page_url);
  if (cache_it != page_cache_.end()) {
    return cache_it->second;
  }
  
  // Extract content
  content::ContentExtractor::ExtractedContent extracted_content = 
      content_extractor_->ExtractContentSync(page_url, html_content);
  
  // Create processing result
  ProcessingResult result;
  result.page_url = page_url;
  result.page_title = extracted_content.title;
  result.main_content = extracted_content.main_text;
  result.content_type = extracted_content.content_type;
  
  // Check if content is summarizable
  features::SummarizationFeature* summarization_feature = 
      browser_features_->GetSummarizationFeature();
  if (summarization_feature) {
    features::SummarizationFeature::EligibilityResult eligibility = 
        summarization_feature->IsPageEligibleForSummarization(
            page_url, extracted_content.main_text);
    result.is_summarizable = eligibility.is_eligible;
  } else {
    result.is_summarizable = false;
  }
  
  // For now, assume all content is searchable and analyzable
  result.is_searchable = true;
  result.is_analyzable = true;
  
  // Cache the result
  if (page_cache_.size() >= kMaxCacheSize) {
    // Remove a random entry (in a real implementation, we would use LRU)
    auto it = page_cache_.begin();
    page_cache_.erase(it);
  }
  page_cache_[page_url] = result;
  
  return result;
}

void BrowserContentHandler::OnPageLoaded(
    const std::string& page_url,
    const std::string& html_content,
    views::View* toolbar_view,
    views::Widget* browser_widget) {
  // Process the page
  ProcessPage(
      page_url,
      html_content,
      toolbar_view,
      browser_widget,
      base::BindOnce(
          [](base::WeakPtr<BrowserContentHandler> self,
             const std::string& page_url,
             const std::string& html_content,
             views::View* toolbar_view,
             views::Widget* browser_widget,
             const ProcessingResult& result) {
            if (!self)
              return;
            
            // Notify features of page load
            if (self->browser_features_) {
              features::SummarizationFeature* summarization_feature = 
                  self->browser_features_->GetSummarizationFeature();
              if (summarization_feature) {
                summarization_feature->OnPageLoaded(
                    page_url, result.main_content, toolbar_view, browser_widget);
              }
            }
          },
          weak_ptr_factory_.GetWeakPtr(),
          page_url,
          html_content,
          toolbar_view,
          browser_widget));
}

void BrowserContentHandler::OnPageUnloaded(const std::string& page_url) {
  // Notify features of page unload
  if (browser_features_) {
    features::SummarizationFeature* summarization_feature = 
        browser_features_->GetSummarizationFeature();
    if (summarization_feature) {
      summarization_feature->OnPageUnloaded(page_url);
    }
  }
}

void BrowserContentHandler::OnBrowserClosed() {
  // Notify features of browser close
  if (browser_features_) {
    features::SummarizationFeature* summarization_feature = 
        browser_features_->GetSummarizationFeature();
    if (summarization_feature) {
      summarization_feature->OnBrowserClosed();
    }
  }
  
  // Clear cache
  page_cache_.clear();
}

base::WeakPtr<BrowserContentHandler> BrowserContentHandler::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void BrowserContentHandler::OnContentExtracted(
    const std::string& page_url,
    views::View* toolbar_view,
    views::Widget* browser_widget,
    ProcessingCallback callback,
    const content::ContentExtractor::ExtractedContent& content) {
  // Create processing result
  ProcessingResult result;
  result.page_url = page_url;
  result.page_title = content.title;
  result.main_content = content.main_text;
  result.content_type = content.content_type;
  
  // Check if content is summarizable
  features::SummarizationFeature* summarization_feature = 
      browser_features_->GetSummarizationFeature();
  if (summarization_feature) {
    features::SummarizationFeature::EligibilityResult eligibility = 
        summarization_feature->IsPageEligibleForSummarization(
            page_url, content.main_text);
    result.is_summarizable = eligibility.is_eligible;
  } else {
    result.is_summarizable = false;
  }
  
  // For now, assume all content is searchable and analyzable
  result.is_searchable = true;
  result.is_analyzable = true;
  
  // Cache the result
  if (page_cache_.size() >= kMaxCacheSize) {
    // Remove a random entry (in a real implementation, we would use LRU)
    auto it = page_cache_.begin();
    page_cache_.erase(it);
  }
  page_cache_[page_url] = result;
  
  // Return the result
  std::move(callback).Run(result);
}

}  // namespace browser_core// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser_core/browser_content_handler.h"

#include <algorithm>
#include <string>
#include <unordered_map>

#include "base/bind.h"
#include "browser_core/features/summarization_feature.h"

namespace browser_core {

namespace {

// Maximum cache size
constexpr size_t kMaxCacheSize = 20;

}  // namespace

BrowserContentHandler::BrowserContentHandler()
    : weak_ptr_factory_(this) {}

BrowserContentHandler::~BrowserContentHandler() = default;

bool BrowserContentHandler::Initialize(BrowserFeatures* browser_features) {
  browser_features_ = browser_features;
  
  // Initialize content extractor
  content_extractor_ = std::make_unique<content::ContentExtractor>();
  if (!content_extractor_->Initialize()) {
    return false;
  }
  
  return true;
}

void BrowserContentHandler::ProcessPage(
    const std::string& page_url,
    const std::string& html_content,
    views::View* toolbar_view,
    views::Widget* browser_widget,
    ProcessingCallback callback) {
  // Check cache first
  auto cache_it = page_cache_.find(page_url);
  if (cache_it != page_cache_.end()) {
    std::move(callback).Run(cache_it->second);
    return;
  }
  
  // Extract content
  content_extractor_->ExtractContent(
      page_url,
      html_content,
      base::BindOnce(&BrowserContentHandler::OnContentExtracted,
                   weak_ptr_factory_.GetWeakPtr(),
                   page_url,
                   toolbar_view,
                   browser_widget,
                   std::move(callback)));
}

BrowserContentHandler::ProcessingResult BrowserContentHandler::ProcessPageSync(
    const std::string& page_url,
    const std::string& html_content) {
  // Check cache first
  auto cache_it = page_cache_.find(page_url);
  if (cache_it != page_cache_.end()) {
    return cache_it->second;
  }
  
  // Extract content
  content::ContentExtractor::ExtractedContent extracted_content = 
      content_extractor_->ExtractContentSync(page_url, html_content);
  
  // Create processing result
  ProcessingResult result;
  result.page_url = page_url;
  result.page_title = extracted_content.title;
  result.main_content = extracted_content.main_text;
  result.content_type = extracted_content.content_type;
  
  // Check if content is summarizable
  features::SummarizationFeature* summarization_feature = 
      browser_features_->GetSummarizationFeature();
  if (summarization_feature) {
    features::SummarizationFeature::EligibilityResult eligibility = 
        summarization_feature->IsPageEligibleForSummarization(
            page_url, extracted_content.main_text);
    result.is_summarizable = eligibility.is_eligible;
  } else {
    result.is_summarizable = false;
  }
  
  // For now, assume all content is searchable and analyzable
  result.is_searchable = true;
  result.is_analyzable = true;
  
  // Cache the result
  if (page_cache_.size() >= kMaxCacheSize) {
    // Remove a random entry (in a real implementation, we would use LRU)
    auto it = page_cache_.begin();
    page_cache_.erase(it);
  }
  page_cache_[page_url] = result;
  
  return result;
}

void BrowserContentHandler::OnPageLoaded(
    const std::string& page_url,
    const std::string& html_content,
    views::View* toolbar_view,
    views::Widget* browser_widget) {
  // Process the page
  ProcessPage(
      page_url,
      html_content,
      toolbar_view,
      browser_widget,
      base::BindOnce(
          [](base::WeakPtr<BrowserContentHandler> self,
             const std::string& page_url,
             const std::string& html_content,
             views::View* toolbar_view,
             views::Widget* browser_widget,
             const ProcessingResult& result) {
            if (!self)
              return;
            
            // Notify features of page load
            if (self->browser_features_) {
              features::SummarizationFeature* summarization_feature = 
                  self->browser_features_->GetSummarizationFeature();
              if (summarization_feature) {
                summarization_feature->OnPageLoaded(
                    page_url, result.main_content, toolbar_view, browser_widget);
              }
            }
          },
          weak_ptr_factory_.GetWeakPtr(),
          page_url,
          html_content,
          toolbar_view,
          browser_widget));
}

void BrowserContentHandler::OnPageUnloaded(const std::string& page_url) {
  // Notify features of page unload
  if (browser_features_) {
    features::SummarizationFeature* summarization_feature = 
        browser_features_->GetSummarizationFeature();
    if (summarization_feature) {
      summarization_feature->OnPageUnloaded(page_url);
    }
  }
}

void BrowserContentHandler::OnBrowserClosed() {
  // Notify features of browser close
  if (browser_features_) {
    features::SummarizationFeature* summarization_feature = 
        browser_features_->GetSummarizationFeature();
    if (summarization_feature) {
      summarization_feature->OnBrowserClosed();
    }
  }
  
  // Clear cache
  page_cache_.clear();
}

base::WeakPtr<BrowserContentHandler> BrowserContentHandler::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void BrowserContentHandler::OnContentExtracted(
    const std::string& page_url,
    views::View* toolbar_view,
    views::Widget* browser_widget,
    ProcessingCallback callback,
    const content::ContentExtractor::ExtractedContent& content) {
  // Create processing result
  ProcessingResult result;
  result.page_url = page_url;
  result.page_title = content.title;
  result.main_content = content.main_text;
  result.content_type = content.content_type;
  
  // Check if content is summarizable
  features::SummarizationFeature* summarization_feature = 
      browser_features_->GetSummarizationFeature();
  if (summarization_feature) {
    features::SummarizationFeature::EligibilityResult eligibility = 
        summarization_feature->IsPageEligibleForSummarization(
            page_url, content.main_text);
    result.is_summarizable = eligibility.is_eligible;
  } else {
    result.is_summarizable = false;
  }
  
  // For now, assume all content is searchable and analyzable
  result.is_searchable = true;
  result.is_analyzable = true;
  
  // Cache the result
  if (page_cache_.size() >= kMaxCacheSize) {
    // Remove a random entry (in a real implementation, we would use LRU)
    auto it = page_cache_.begin();
    page_cache_.erase(it);
  }
  page_cache_[page_url] = result;
  
  // Return the result
  std::move(callback).Run(result);
}

}  // namespace browser_core