// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASOL_BROWSER_BROWSER_FEATURES_H_
#define ASOL_BROWSER_BROWSER_FEATURES_H_

#include <string>
#include <vector>

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace asol {
namespace browser {

// Features defined here should be used in implementation files like this:
//   if (base::FeatureList::IsEnabled(features::kAsolSidePanelIntegration)) {
//     // Feature-specific code here
//   }

// This feature enables the ASOL side panel integration with the browser
extern const base::Feature kAsolSidePanelIntegration;

// This feature enables the ASOL toolbar button
extern const base::Feature kAsolToolbarButton;

// This feature enables the ASOL context menu integration
extern const base::Feature kAsolContextMenuIntegration;

// This feature enables the ASOL keyboard shortcuts
extern const base::Feature kAsolKeyboardShortcuts;

// This feature enables the ASOL omnibox integration
extern const base::Feature kAsolOmniboxIntegration;

// This feature enables the ASOL page context extraction
extern const base::Feature kAsolPageContextExtraction;

// This feature enables the ASOL multi-modal input support
extern const base::Feature kAsolMultiModalInput;

// This feature enables the ASOL response caching
extern const base::Feature kAsolResponseCaching;

// This feature enables the ASOL performance tracking
extern const base::Feature kAsolPerformanceTracking;

// This feature enables the ASOL telemetry
extern const base::Feature kAsolTelemetry;

// This feature enables the ASOL research mode
extern const base::Feature kAsolResearchMode;

// This feature enables the ASOL developer mode
extern const base::Feature kAsolDeveloperMode;

// This feature enables the ASOL work mode
extern const base::Feature kAsolWorkMode;

// This feature enables the ASOL gaming mode
extern const base::Feature kAsolGamingMode;

// Feature parameters

// Maximum number of responses to cache
extern const base::FeatureParam<int> kAsolResponseCacheSizeParam;

// Cache expiration time in seconds
extern const base::FeatureParam<int> kAsolResponseCacheExpirationParam;

// Maximum context length to extract from the page
extern const base::FeatureParam<int> kAsolMaxPageContextLengthParam;

// Whether to use streaming by default
extern const base::FeatureParam<bool> kAsolUseStreamingByDefaultParam;

// Default adapter to use
extern const base::FeatureParam<std::string> kAsolDefaultAdapterParam;

// Default capability to use
extern const base::FeatureParam<std::string> kAsolDefaultCapabilityParam;

// Whether to show the ASOL panel on startup
extern const base::FeatureParam<bool> kAsolShowPanelOnStartupParam;

// Whether to enable telemetry by default
extern const base::FeatureParam<bool> kAsolEnableTelemetryByDefaultParam;

// Whether to automatically add pages to research sessions
extern const base::FeatureParam<bool> kAsolAutoAddPagesToResearchParam;

// Whether to automatically generate key points for research pages
extern const base::FeatureParam<bool> kAsolAutoGenerateKeyPointsParam;

// Maximum number of pages per research session
extern const base::FeatureParam<int> kAsolMaxPagesPerSessionParam;

// Whether to automatically detect code on pages in developer mode
extern const base::FeatureParam<bool> kAsolAutoDetectCodeParam;

// Maximum number of code snippets to store in developer mode
extern const base::FeatureParam<int> kAsolMaxCodeSnippetsParam;

// Whether to automatically save documents in work mode
extern const base::FeatureParam<bool> kAsolAutoSaveDocumentsParam;

// Maximum number of documents to store in work mode
extern const base::FeatureParam<int> kAsolMaxDocumentsParam;

// Whether to automatically detect games in gaming mode
extern const base::FeatureParam<bool> kAsolAutoDetectGamesParam;

// Maximum number of game info entries to store in gaming mode
extern const base::FeatureParam<int> kAsolMaxGameInfoParam;

}  // namespace browser
}  // namespace asol

#endif  // ASOL_BROWSER_BROWSER_FEATURES_H_