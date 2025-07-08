// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_CORE_FEATURES_SUMMARIZATION_FEATURE_H_
#define BROWSER_CORE_FEATURES_SUMMARIZATION_FEATURE_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "browser_core/ai/summarization_service.h"
#include "browser_core/ui/summarization_ui.h"
#include "asol/core/ai_service_manager.h"
#include "asol/core/privacy_proxy.h"

namespace browser_core {
namespace features {

// SummarizationFeature integrates the AI-Summarization & Content Digest feature
// into the browser. It coordinates between the content extraction, summarization
// service, and UI components, and handles browser events.
class SummarizationFeature {
 public:
  // Feature mode
  enum class FeatureMode {
    MANUAL,       // User manually triggers summarization
    AUTOMATIC,    // Browser automatically triggers summarization
    HYBRID        // Combination of manual and automatic
  };

  // Summarization eligibility result
  struct EligibilityResult {
    bool is_eligible;
    std::string reason;
    float confidence_score;
  };

  SummarizationFeature();
  ~SummarizationFeature();

  // Disallow copy and assign
  SummarizationFeature(const SummarizationFeature&) = delete;
  SummarizationFeature& operator=(const SummarizationFeature&) = delete;

  // Initialize the feature
  bool Initialize(asol::core::AIServiceManager* ai_service_manager,
                asol::core::PrivacyProxy* privacy_proxy);

  // Check if a page is eligible for summarization
  EligibilityResult IsPageEligibleForSummarization(const std::string& page_url,
                                                 const std::string& page_content);

  // Set the feature mode
  void SetFeatureMode(FeatureMode mode);
  FeatureMode GetFeatureMode() const;

  // Set user preferences for summarization
  void SetPreferredSummaryFormat(ai::SummarizationService::SummaryFormat format);
  void SetPreferredSummaryLength(ai::SummarizationService::SummaryLength length);

  // Browser event handlers
  void OnPageLoaded(const std::string& page_url, 
                  const std::string& page_content,
                  views::View* toolbar_view,
                  views::Widget* browser_widget);
  void OnPageUnloaded(const std::string& page_url);
  void OnBrowserClosed();

  // Get a weak pointer to this instance
  base::WeakPtr<SummarizationFeature> GetWeakPtr();

 private:
  // Predict if a page should be automatically summarized
  bool ShouldAutoSummarize(const std::string& page_url,
                         const std::string& page_content);

  // Handle automatic summarization
  void HandleAutoSummarization(const std::string& page_url,
                             const std::string& page_content,
                             views::Widget* browser_widget);

  // Handle UI events
  void OnUIEvent(const std::string& event_type, const std::string& event_data);

  // Components
  std::unique_ptr<ai::SummarizationService> summarization_service_;
  std::unique_ptr<ui::SummarizationUI> summarization_ui_;

  // Configuration
  FeatureMode feature_mode_ = FeatureMode::HYBRID;
  ai::SummarizationService::SummaryFormat preferred_format_ = 
      ai::SummarizationService::SummaryFormat::EXECUTIVE_SUMMARY;
  ai::SummarizationService::SummaryLength preferred_length_ = 
      ai::SummarizationService::SummaryLength::MEDIUM;

  // Current page info
  std::string current_page_url_;
  std::string current_page_content_;
  views::View* current_toolbar_view_ = nullptr;
  views::Widget* current_browser_widget_ = nullptr;

  // For weak pointers
  base::WeakPtrFactory<SummarizationFeature> weak_ptr_factory_{this};
};

}  // namespace features
}  // namespace browser_core

#endif  // BROWSER_CORE_FEATURES_SUMMARIZATION_FEATURE_H_// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_CORE_FEATURES_SUMMARIZATION_FEATURE_H_
#define BROWSER_CORE_FEATURES_SUMMARIZATION_FEATURE_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "browser_core/ai/summarization_service.h"
#include "browser_core/ui/summarization_ui.h"
#include "asol/core/ai_service_manager.h"
#include "asol/core/privacy_proxy.h"

namespace browser_core {
namespace features {

// SummarizationFeature integrates the AI-Summarization & Content Digest feature
// into the browser. It coordinates between the content extraction, summarization
// service, and UI components, and handles browser events.
class SummarizationFeature {
 public:
  // Feature mode
  enum class FeatureMode {
    MANUAL,       // User manually triggers summarization
    AUTOMATIC,    // Browser automatically triggers summarization
    HYBRID        // Combination of manual and automatic
  };

  // Summarization eligibility result
  struct EligibilityResult {
    bool is_eligible;
    std::string reason;
    float confidence_score;
  };

  SummarizationFeature();
  ~SummarizationFeature();

  // Disallow copy and assign
  SummarizationFeature(const SummarizationFeature&) = delete;
  SummarizationFeature& operator=(const SummarizationFeature&) = delete;

  // Initialize the feature
  bool Initialize(asol::core::AIServiceManager* ai_service_manager,
                asol::core::PrivacyProxy* privacy_proxy);

  // Check if a page is eligible for summarization
  EligibilityResult IsPageEligibleForSummarization(const std::string& page_url,
                                                 const std::string& page_content);

  // Set the feature mode
  void SetFeatureMode(FeatureMode mode);
  FeatureMode GetFeatureMode() const;

  // Set user preferences for summarization
  void SetPreferredSummaryFormat(ai::SummarizationService::SummaryFormat format);
  void SetPreferredSummaryLength(ai::SummarizationService::SummaryLength length);

  // Browser event handlers
  void OnPageLoaded(const std::string& page_url, 
                  const std::string& page_content,
                  views::View* toolbar_view,
                  views::Widget* browser_widget);
  void OnPageUnloaded(const std::string& page_url);
  void OnBrowserClosed();

  // Get a weak pointer to this instance
  base::WeakPtr<SummarizationFeature> GetWeakPtr();

 private:
  // Predict if a page should be automatically summarized
  bool ShouldAutoSummarize(const std::string& page_url,
                         const std::string& page_content);

  // Handle automatic summarization
  void HandleAutoSummarization(const std::string& page_url,
                             const std::string& page_content,
                             views::Widget* browser_widget);

  // Handle UI events
  void OnUIEvent(const std::string& event_type, const std::string& event_data);

  // Components
  std::unique_ptr<ai::SummarizationService> summarization_service_;
  std::unique_ptr<ui::SummarizationUI> summarization_ui_;

  // Configuration
  FeatureMode feature_mode_ = FeatureMode::HYBRID;
  ai::SummarizationService::SummaryFormat preferred_format_ = 
      ai::SummarizationService::SummaryFormat::EXECUTIVE_SUMMARY;
  ai::SummarizationService::SummaryLength preferred_length_ = 
      ai::SummarizationService::SummaryLength::MEDIUM;

  // Current page info
  std::string current_page_url_;
  std::string current_page_content_;
  views::View* current_toolbar_view_ = nullptr;
  views::Widget* current_browser_widget_ = nullptr;

  // For weak pointers
  base::WeakPtrFactory<SummarizationFeature> weak_ptr_factory_{this};
};

}  // namespace features
}  // namespace browser_core

#endif  // BROWSER_CORE_FEATURES_SUMMARIZATION_FEATURE_H_