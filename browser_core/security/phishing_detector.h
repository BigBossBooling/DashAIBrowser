// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_CORE_SECURITY_PHISHING_DETECTOR_H_
#define BROWSER_CORE_SECURITY_PHISHING_DETECTOR_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "browser_core/security/security_manager.h"

namespace browser_core {
namespace security {

// PhishingDetector detects phishing attempts in web pages.
class PhishingDetector {
 public:
  // Phishing detection result (same as in SecurityManager)
  using PhishingDetectionResult = SecurityManager::PhishingDetectionResult;

  // Callback for phishing detection results
  using PhishingDetectionCallback = 
      base::OnceCallback<void(const PhishingDetectionResult&)>;

  PhishingDetector();
  ~PhishingDetector();

  // Disallow copy and assign
  PhishingDetector(const PhishingDetector&) = delete;
  PhishingDetector& operator=(const PhishingDetector&) = delete;

  // Initialize the phishing detector
  bool Initialize();

  // Detect phishing in a URL and page content
  void DetectPhishing(const std::string& url, 
                    const std::string& page_content,
                    PhishingDetectionCallback callback);

  // Check if a URL is similar to a known brand
  bool IsSimilarToBrand(const std::string& url, std::string* brand_name);

  // Get the list of known phishing domains
  std::vector<std::string> GetKnownPhishingDomains() const;

  // Add a domain to the phishing list
  void AddPhishingDomain(const std::string& domain);

  // Remove a domain from the phishing list
  void RemovePhishingDomain(const std::string& domain);

  // Clear the phishing list
  void ClearPhishingList();

  // Enable/disable AI-based phishing detection
  void EnableAIDetection(bool enable);
  bool IsAIDetectionEnabled() const;

 private:
  // Private implementation
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace security
}  // namespace browser_core

#endif  // BROWSER_CORE_SECURITY_PHISHING_DETECTOR_H_