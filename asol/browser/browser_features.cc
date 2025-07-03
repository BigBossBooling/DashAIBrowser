// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "asol/browser/browser_features.h"

namespace asol {
namespace browser {

// Feature definitions
const base::Feature kAsolSidePanelIntegration{"AsolSidePanelIntegration",
                                             base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kAsolToolbarButton{"AsolToolbarButton",
                                      base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kAsolContextMenuIntegration{"AsolContextMenuIntegration",
                                              base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kAsolKeyboardShortcuts{"AsolKeyboardShortcuts",
                                         base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kAsolOmniboxIntegration{"AsolOmniboxIntegration",
                                          base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kAsolPageContextExtraction{"AsolPageContextExtraction",
                                             base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kAsolMultiModalInput{"AsolMultiModalInput",
                                        base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kAsolResponseCaching{"AsolResponseCaching",
                                        base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kAsolPerformanceTracking{"AsolPerformanceTracking",
                                           base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kAsolTelemetry{"AsolTelemetry",
                                  base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kAsolResearchMode{"AsolResearchMode",
                                     base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kAsolDeveloperMode{"AsolDeveloperMode",
                                      base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kAsolWorkMode{"AsolWorkMode",
                                 base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kAsolGamingMode{"AsolGamingMode",
                                   base::FEATURE_ENABLED_BY_DEFAULT};

// Feature parameters
const base::FeatureParam<int> kAsolResponseCacheSizeParam{
    &kAsolResponseCaching, "cache_size", 100};

const base::FeatureParam<int> kAsolResponseCacheExpirationParam{
    &kAsolResponseCaching, "cache_expiration_seconds", 3600};  // 1 hour

const base::FeatureParam<int> kAsolMaxPageContextLengthParam{
    &kAsolPageContextExtraction, "max_context_length", 5000};

const base::FeatureParam<bool> kAsolUseStreamingByDefaultParam{
    &kAsolSidePanelIntegration, "use_streaming_by_default", true};

const base::FeatureParam<std::string> kAsolDefaultAdapterParam{
    &kAsolSidePanelIntegration, "default_adapter", "gemini"};

const base::FeatureParam<std::string> kAsolDefaultCapabilityParam{
    &kAsolSidePanelIntegration, "default_capability", "text-generation"};

const base::FeatureParam<bool> kAsolShowPanelOnStartupParam{
    &kAsolSidePanelIntegration, "show_panel_on_startup", false};

const base::FeatureParam<bool> kAsolEnableTelemetryByDefaultParam{
    &kAsolTelemetry, "enable_by_default", false};

const base::FeatureParam<bool> kAsolAutoAddPagesToResearchParam{
    &kAsolResearchMode, "auto_add_pages", false};

const base::FeatureParam<bool> kAsolAutoGenerateKeyPointsParam{
    &kAsolResearchMode, "auto_generate_key_points", false};

const base::FeatureParam<int> kAsolMaxPagesPerSessionParam{
    &kAsolResearchMode, "max_pages_per_session", 100};

const base::FeatureParam<bool> kAsolAutoDetectCodeParam{
    &kAsolDeveloperMode, "auto_detect_code", true};

const base::FeatureParam<int> kAsolMaxCodeSnippetsParam{
    &kAsolDeveloperMode, "max_code_snippets", 100};

const base::FeatureParam<bool> kAsolAutoSaveDocumentsParam{
    &kAsolWorkMode, "auto_save_documents", true};

const base::FeatureParam<int> kAsolMaxDocumentsParam{
    &kAsolWorkMode, "max_documents", 50};

const base::FeatureParam<bool> kAsolAutoDetectGamesParam{
    &kAsolGamingMode, "auto_detect_games", true};

const base::FeatureParam<int> kAsolMaxGameInfoParam{
    &kAsolGamingMode, "max_game_info", 50};

}  // namespace browser
}  // namespace asol