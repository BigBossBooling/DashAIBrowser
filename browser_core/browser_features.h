// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_CORE_BROWSER_FEATURES_H_
#define BROWSER_CORE_BROWSER_FEATURES_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "asol/core/ai_service_manager.h"
#include "asol/core/privacy_proxy.h"

namespace browser_core {
namespace features {
class SummarizationFeature;
}  // namespace features

// BrowserFeatures manages all AI-powered features in the browser.
// It initializes and coordinates features, and provides a central point
// for browser components to access them.
class BrowserFeatures {
 public:
  BrowserFeatures();
  ~BrowserFeatures();

  // Disallow copy and assign
  BrowserFeatures(const BrowserFeatures&) = delete;
  BrowserFeatures& operator=(const BrowserFeatures&) = delete;

  // Initialize all features
  bool Initialize(asol::core::AIServiceManager* ai_service_manager,
                asol::core::PrivacyProxy* privacy_proxy);

  // Get the summarization feature
  features::SummarizationFeature* GetSummarizationFeature();

  // Get a weak pointer to this instance
  base::WeakPtr<BrowserFeatures> GetWeakPtr();

 private:
  // Features
  std::unique_ptr<features::SummarizationFeature> summarization_feature_;

  // For weak pointers
  base::WeakPtrFactory<BrowserFeatures> weak_ptr_factory_{this};
};

}  // namespace browser_core

#endif  // BROWSER_CORE_BROWSER_FEATURES_H_