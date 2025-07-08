// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser_core/browser_features.h"

#include "browser_core/features/summarization_feature.h"

namespace browser_core {

BrowserFeatures::BrowserFeatures()
    : weak_ptr_factory_(this) {}

BrowserFeatures::~BrowserFeatures() = default;

bool BrowserFeatures::Initialize(
    asol::core::AIServiceManager* ai_service_manager,
    asol::core::PrivacyProxy* privacy_proxy) {
  // Initialize summarization feature
  summarization_feature_ = std::make_unique<features::SummarizationFeature>();
  if (!summarization_feature_->Initialize(ai_service_manager, privacy_proxy)) {
    return false;
  }

  return true;
}

features::SummarizationFeature* BrowserFeatures::GetSummarizationFeature() {
  return summarization_feature_.get();
}

base::WeakPtr<BrowserFeatures> BrowserFeatures::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

}  // namespace browser_core