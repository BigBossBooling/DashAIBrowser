// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser_core/features/summarization_feature.h"

#include <algorithm>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"

namespace browser_core {
namespace features {

namespace {

// Minimum content length for auto-summarization (in characters)
constexpr size_t kMinAutoSummarizeLength = 3000;

// Minimum reading time for auto-summarization (in seconds)
constexpr int kMinReadingTimeSeconds = 120;  // 2 minutes

// Average reading speed (words per minute)
constexpr int kAverageReadingSpeedWPM = 250;

// Helper function to estimate reading time
base::TimeDelta EstimateReadingTime(const std::string& content) {
  // Count words
  std::vector<std::string> words = base::SplitString(
      content, " ", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  
  // Calculate reading time
  double minutes = static_cast<double>(words.size()) / kAverageReadingSpeedWPM;
  return base::Seconds(minutes * 60);
}

// Helper function to check if a URL is eligible for summarization
bool IsURLEligible(const std::string& url) {
  // Skip empty URLs
  if (url.empty())
    return false;
  
  // Skip about: URLs
  if (base::StartsWith(url, "about:", base::CompareCase::INSENSITIVE_ASCII))
    return false;
  
  // Skip chrome: URLs
  if (base::StartsWith(url, "chrome:", base::CompareCase::INSENSITIVE_ASCII))
    return false;
  
  // Skip file: URLs
  if (base::StartsWith(url, "file:", base::CompareCase::INSENSITIVE_ASCII))
    return false;
  
  // Skip data: URLs
  if (base::StartsWith(url, "data:", base::CompareCase::INSENSITIVE_ASCII))
    return false;
  
  // Skip javascript: URLs
  if (base::StartsWith(url, "javascript:", base::CompareCase::INSENSITIVE_ASCII))
    return false;
  
  return true;
}

}  // namespace

SummarizationFeature::SummarizationFeature()
    : weak_ptr_factory_(this) {}

SummarizationFeature::~SummarizationFeature() = default;

bool SummarizationFeature::Initialize(
    asol::core::AIServiceManager* ai_service_manager,
    asol::core::PrivacyProxy* privacy_proxy) {
  // Initialize summarization service
  summarization_service_ = std::make_unique<ai::SummarizationService>();
  if (!summarization_service_->Initialize(ai_service_manager, privacy_proxy))
    return false;
  
  // Initialize UI
  summarization_ui_ = std::make_unique<ui::SummarizationUI>();
  if (!summarization_ui_->Initialize(summarization_service_.get()))
    return false;
  
  // Set up UI event handling
  summarization_ui_->SetEventCallback(
      base::BindRepeating(&SummarizationFeature::OnUIEvent,
                        base::Unretained(this)));
  
  return true;
}

SummarizationFeature::EligibilityResult 
SummarizationFeature::IsPageEligibleForSummarization(
    const std::string& page_url,
    const std::string& page_content) {
  EligibilityResult result;
  
  // Check URL
  if (!IsURLEligible(page_url)) {
    result.is_eligible = false;
    result.reason = "URL not eligible for summarization";
    result.confidence_score = 0.0f;
    return result;
  }
  
  // Check if content is summarizable
  if (!summarization_service_->IsContentSummarizable(page_content)) {
    result.is_eligible = false;
    result.reason = "Content not suitable for summarization";
    result.confidence_score = 0.0f;
    return result;
  }
  
  // Content is eligible
  result.is_eligible = true;
  result.reason = "Content is eligible for summarization";
  
  // Calculate confidence score based on content length and estimated reading time
  float length_score = std::min(1.0f, page_content.length() / 10000.0f);
  float time_score = std::min(1.0f, 
      EstimateReadingTime(page_content).InSecondsF() / 300.0f);  // Max 5 minutes
  
  result.confidence_score = (length_score + time_score) / 2.0f;
  
  return result;
}

void SummarizationFeature::SetFeatureMode(FeatureMode mode) {
  feature_mode_ = mode;
}

SummarizationFeature::FeatureMode SummarizationFeature::GetFeatureMode() const {
  return feature_mode_;
}

void SummarizationFeature::SetPreferredSummaryFormat(
    ai::SummarizationService::SummaryFormat format) {
  preferred_format_ = format;
  summarization_ui_->SetSummaryFormat(format);
}

void SummarizationFeature::SetPreferredSummaryLength(
    ai::SummarizationService::SummaryLength length) {
  preferred_length_ = length;
  summarization_ui_->SetSummaryLength(length);
}

void SummarizationFeature::OnPageLoaded(
    const std::string& page_url,
    const std::string& page_content,
    views::View* toolbar_view,
    views::Widget* browser_widget) {
  // Store current page info
  current_page_url_ = page_url;
  current_page_content_ = page_content;
  current_toolbar_view_ = toolbar_view;
  current_browser_widget_ = browser_widget;
  
  // Check if page is eligible for summarization
  EligibilityResult eligibility = 
      IsPageEligibleForSummarization(page_url, page_content);
  
  if (eligibility.is_eligible) {
    // Set content for summarization
    summarization_ui_->SetContent(page_content, page_url);
    
    // Show the Synapse button
    summarization_ui_->ShowSynapseButton(toolbar_view);
    
    // Set UI state to available
    summarization_ui_->SetUIState(ui::SummarizationUI::UIState::AVAILABLE);
    
    // Check if we should auto-summarize
    if ((feature_mode_ == FeatureMode::AUTOMATIC || 
         feature_mode_ == FeatureMode::HYBRID) &&
        ShouldAutoSummarize(page_url, page_content)) {
      HandleAutoSummarization(page_url, page_content, browser_widget);
    }
  } else {
    // Hide the Synapse button
    summarization_ui_->HideSynapseButton();
    
    // Set UI state to inactive
    summarization_ui_->SetUIState(ui::SummarizationUI::UIState::INACTIVE);
  }
}

void SummarizationFeature::OnPageUnloaded(const std::string& page_url) {
  if (current_page_url_ == page_url) {
    // Hide the Synapse button
    summarization_ui_->HideSynapseButton();
    
    // Hide the summary sidebar
    summarization_ui_->HideSummarySidebar();
    
    // Clear current page info
    current_page_url_.clear();
    current_page_content_.clear();
    current_toolbar_view_ = nullptr;
    current_browser_widget_ = nullptr;
  }
}

void SummarizationFeature::OnBrowserClosed() {
  // Hide the Synapse button
  summarization_ui_->HideSynapseButton();
  
  // Hide the summary sidebar
  summarization_ui_->HideSummarySidebar();
  
  // Clear current page info
  current_page_url_.clear();
  current_page_content_.clear();
  current_toolbar_view_ = nullptr;
  current_browser_widget_ = nullptr;
}

base::WeakPtr<SummarizationFeature> SummarizationFeature::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

bool SummarizationFeature::ShouldAutoSummarize(
    const std::string& page_url,
    const std::string& page_content) {
  // Skip if feature mode is manual
  if (feature_mode_ == FeatureMode::MANUAL)
    return false;
  
  // Check content length
  if (page_content.length() < kMinAutoSummarizeLength)
    return false;
  
  // Check estimated reading time
  base::TimeDelta reading_time = EstimateReadingTime(page_content);
  if (reading_time.InSeconds() < kMinReadingTimeSeconds)
    return false;
  
  // In a real implementation, we would use more sophisticated heuristics
  // such as content type analysis, user preferences, browsing history, etc.
  
  return true;
}

void SummarizationFeature::HandleAutoSummarization(
    const std::string& page_url,
    const std::string& page_content,
    views::Widget* browser_widget) {
  // Set UI state to loading
  summarization_ui_->SetUIState(ui::SummarizationUI::UIState::LOADING);
  
  // Show the summary sidebar
  summarization_ui_->ShowSummarySidebar(browser_widget);
  
  // Trigger summarization
  summarization_service_->SummarizeContent(
      page_content,
      page_url,
      preferred_format_,
      preferred_length_,
      base::BindOnce(
          [](base::WeakPtr<SummarizationFeature> self,
             const ai::SummarizationService::SummaryResult& result) {
            if (!self)
              return;
            
            if (result.success) {
              self->summarization_ui_->SetUIState(
                  ui::SummarizationUI::UIState::ACTIVE);
            } else {
              self->summarization_ui_->SetUIState(
                  ui::SummarizationUI::UIState::ERROR);
            }
          },
          weak_ptr_factory_.GetWeakPtr()));
}

void SummarizationFeature::OnUIEvent(
    const std::string& event_type,
    const std::string& event_data) {
  if (event_type == "synapse_button_clicked") {
    // Toggle the summary sidebar
    summarization_ui_->ToggleSummarySidebar(current_browser_widget_);
  } else if (event_type == "sidebar_closed") {
    // Update UI state
    summarization_ui_->SetUIState(ui::SummarizationUI::UIState::AVAILABLE);
  } else if (event_type == "format_changed") {
    // Parse format from event data
    int format_value = std::stoi(event_data);
    preferred_format_ = static_cast<ai::SummarizationService::SummaryFormat>(
        format_value);
  } else if (event_type == "length_changed") {
    // Parse length from event data
    int length_value = std::stoi(event_data);
    preferred_length_ = static_cast<ai::SummarizationService::SummaryLength>(
        length_value);
  }
}

}  // namespace features
}  // namespace browser_core// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser_core/features/summarization_feature.h"

#include <algorithm>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"

namespace browser_core {
namespace features {

namespace {

// Minimum content length for auto-summarization (in characters)
constexpr size_t kMinAutoSummarizeLength = 3000;

// Minimum reading time for auto-summarization (in seconds)
constexpr int kMinReadingTimeSeconds = 120;  // 2 minutes

// Average reading speed (words per minute)
constexpr int kAverageReadingSpeedWPM = 250;

// Helper function to estimate reading time
base::TimeDelta EstimateReadingTime(const std::string& content) {
  // Count words
  std::vector<std::string> words = base::SplitString(
      content, " ", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  
  // Calculate reading time
  double minutes = static_cast<double>(words.size()) / kAverageReadingSpeedWPM;
  return base::Seconds(minutes * 60);
}

// Helper function to check if a URL is eligible for summarization
bool IsURLEligible(const std::string& url) {
  // Skip empty URLs
  if (url.empty())
    return false;
  
  // Skip about: URLs
  if (base::StartsWith(url, "about:", base::CompareCase::INSENSITIVE_ASCII))
    return false;
  
  // Skip chrome: URLs
  if (base::StartsWith(url, "chrome:", base::CompareCase::INSENSITIVE_ASCII))
    return false;
  
  // Skip file: URLs
  if (base::StartsWith(url, "file:", base::CompareCase::INSENSITIVE_ASCII))
    return false;
  
  // Skip data: URLs
  if (base::StartsWith(url, "data:", base::CompareCase::INSENSITIVE_ASCII))
    return false;
  
  // Skip javascript: URLs
  if (base::StartsWith(url, "javascript:", base::CompareCase::INSENSITIVE_ASCII))
    return false;
  
  return true;
}

}  // namespace

SummarizationFeature::SummarizationFeature()
    : weak_ptr_factory_(this) {}

SummarizationFeature::~SummarizationFeature() = default;

bool SummarizationFeature::Initialize(
    asol::core::AIServiceManager* ai_service_manager,
    asol::core::PrivacyProxy* privacy_proxy) {
  // Initialize summarization service
  summarization_service_ = std::make_unique<ai::SummarizationService>();
  if (!summarization_service_->Initialize(ai_service_manager, privacy_proxy))
    return false;
  
  // Initialize UI
  summarization_ui_ = std::make_unique<ui::SummarizationUI>();
  if (!summarization_ui_->Initialize(summarization_service_.get()))
    return false;
  
  // Set up UI event handling
  summarization_ui_->SetEventCallback(
      base::BindRepeating(&SummarizationFeature::OnUIEvent,
                        base::Unretained(this)));
  
  return true;
}

SummarizationFeature::EligibilityResult 
SummarizationFeature::IsPageEligibleForSummarization(
    const std::string& page_url,
    const std::string& page_content) {
  EligibilityResult result;
  
  // Check URL
  if (!IsURLEligible(page_url)) {
    result.is_eligible = false;
    result.reason = "URL not eligible for summarization";
    result.confidence_score = 0.0f;
    return result;
  }
  
  // Check if content is summarizable
  if (!summarization_service_->IsContentSummarizable(page_content)) {
    result.is_eligible = false;
    result.reason = "Content not suitable for summarization";
    result.confidence_score = 0.0f;
    return result;
  }
  
  // Content is eligible
  result.is_eligible = true;
  result.reason = "Content is eligible for summarization";
  
  // Calculate confidence score based on content length and estimated reading time
  float length_score = std::min(1.0f, page_content.length() / 10000.0f);
  float time_score = std::min(1.0f, 
      EstimateReadingTime(page_content).InSecondsF() / 300.0f);  // Max 5 minutes
  
  result.confidence_score = (length_score + time_score) / 2.0f;
  
  return result;
}

void SummarizationFeature::SetFeatureMode(FeatureMode mode) {
  feature_mode_ = mode;
}

SummarizationFeature::FeatureMode SummarizationFeature::GetFeatureMode() const {
  return feature_mode_;
}

void SummarizationFeature::SetPreferredSummaryFormat(
    ai::SummarizationService::SummaryFormat format) {
  preferred_format_ = format;
  summarization_ui_->SetSummaryFormat(format);
}

void SummarizationFeature::SetPreferredSummaryLength(
    ai::SummarizationService::SummaryLength length) {
  preferred_length_ = length;
  summarization_ui_->SetSummaryLength(length);
}

void SummarizationFeature::OnPageLoaded(
    const std::string& page_url,
    const std::string& page_content,
    views::View* toolbar_view,
    views::Widget* browser_widget) {
  // Store current page info
  current_page_url_ = page_url;
  current_page_content_ = page_content;
  current_toolbar_view_ = toolbar_view;
  current_browser_widget_ = browser_widget;
  
  // Check if page is eligible for summarization
  EligibilityResult eligibility = 
      IsPageEligibleForSummarization(page_url, page_content);
  
  if (eligibility.is_eligible) {
    // Set content for summarization
    summarization_ui_->SetContent(page_content, page_url);
    
    // Show the Synapse button
    summarization_ui_->ShowSynapseButton(toolbar_view);
    
    // Set UI state to available
    summarization_ui_->SetUIState(ui::SummarizationUI::UIState::AVAILABLE);
    
    // Check if we should auto-summarize
    if ((feature_mode_ == FeatureMode::AUTOMATIC || 
         feature_mode_ == FeatureMode::HYBRID) &&
        ShouldAutoSummarize(page_url, page_content)) {
      HandleAutoSummarization(page_url, page_content, browser_widget);
    }
  } else {
    // Hide the Synapse button
    summarization_ui_->HideSynapseButton();
    
    // Set UI state to inactive
    summarization_ui_->SetUIState(ui::SummarizationUI::UIState::INACTIVE);
  }
}

void SummarizationFeature::OnPageUnloaded(const std::string& page_url) {
  if (current_page_url_ == page_url) {
    // Hide the Synapse button
    summarization_ui_->HideSynapseButton();
    
    // Hide the summary sidebar
    summarization_ui_->HideSummarySidebar();
    
    // Clear current page info
    current_page_url_.clear();
    current_page_content_.clear();
    current_toolbar_view_ = nullptr;
    current_browser_widget_ = nullptr;
  }
}

void SummarizationFeature::OnBrowserClosed() {
  // Hide the Synapse button
  summarization_ui_->HideSynapseButton();
  
  // Hide the summary sidebar
  summarization_ui_->HideSummarySidebar();
  
  // Clear current page info
  current_page_url_.clear();
  current_page_content_.clear();
  current_toolbar_view_ = nullptr;
  current_browser_widget_ = nullptr;
}

base::WeakPtr<SummarizationFeature> SummarizationFeature::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

bool SummarizationFeature::ShouldAutoSummarize(
    const std::string& page_url,
    const std::string& page_content) {
  // Skip if feature mode is manual
  if (feature_mode_ == FeatureMode::MANUAL)
    return false;
  
  // Check content length
  if (page_content.length() < kMinAutoSummarizeLength)
    return false;
  
  // Check estimated reading time
  base::TimeDelta reading_time = EstimateReadingTime(page_content);
  if (reading_time.InSeconds() < kMinReadingTimeSeconds)
    return false;
  
  // In a real implementation, we would use more sophisticated heuristics
  // such as content type analysis, user preferences, browsing history, etc.
  
  return true;
}

void SummarizationFeature::HandleAutoSummarization(
    const std::string& page_url,
    const std::string& page_content,
    views::Widget* browser_widget) {
  // Set UI state to loading
  summarization_ui_->SetUIState(ui::SummarizationUI::UIState::LOADING);
  
  // Show the summary sidebar
  summarization_ui_->ShowSummarySidebar(browser_widget);
  
  // Trigger summarization
  summarization_service_->SummarizeContent(
      page_content,
      page_url,
      preferred_format_,
      preferred_length_,
      base::BindOnce(
          [](base::WeakPtr<SummarizationFeature> self,
             const ai::SummarizationService::SummaryResult& result) {
            if (!self)
              return;
            
            if (result.success) {
              self->summarization_ui_->SetUIState(
                  ui::SummarizationUI::UIState::ACTIVE);
            } else {
              self->summarization_ui_->SetUIState(
                  ui::SummarizationUI::UIState::ERROR);
            }
          },
          weak_ptr_factory_.GetWeakPtr()));
}

void SummarizationFeature::OnUIEvent(
    const std::string& event_type,
    const std::string& event_data) {
  if (event_type == "synapse_button_clicked") {
    // Toggle the summary sidebar
    summarization_ui_->ToggleSummarySidebar(current_browser_widget_);
  } else if (event_type == "sidebar_closed") {
    // Update UI state
    summarization_ui_->SetUIState(ui::SummarizationUI::UIState::AVAILABLE);
  } else if (event_type == "format_changed") {
    // Parse format from event data
    int format_value = std::stoi(event_data);
    preferred_format_ = static_cast<ai::SummarizationService::SummaryFormat>(
        format_value);
  } else if (event_type == "length_changed") {
    // Parse length from event data
    int length_value = std::stoi(event_data);
    preferred_length_ = static_cast<ai::SummarizationService::SummaryLength>(
        length_value);
  }
}

}  // namespace features
}  // namespace browser_core