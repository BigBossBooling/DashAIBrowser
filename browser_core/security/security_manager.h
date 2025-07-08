// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_CORE_SECURITY_SECURITY_MANAGER_H_
#define BROWSER_CORE_SECURITY_SECURITY_MANAGER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"

namespace browser_core {
namespace security {

// SecurityManager handles all security-related features in the browser.
class SecurityManager {
 public:
  // Security check result
  struct SecurityCheckResult {
    bool is_safe = true;
    std::string threat_type;
    std::string description;
    int risk_level = 0;  // 0-10, where 10 is highest risk
  };

  // Phishing detection result
  struct PhishingDetectionResult {
    bool is_phishing = false;
    float confidence = 0.0f;
    std::string target_brand;
    std::vector<std::string> suspicious_elements;
  };

  // Malware scan result
  struct MalwareScanResult {
    bool contains_malware = false;
    std::string malware_type;
    std::string detection_method;
    float confidence = 0.0f;
  };

  // Certificate info
  struct CertificateInfo {
    std::string subject;
    std::string issuer;
    std::string valid_from;
    std::string valid_until;
    bool is_valid = false;
    bool is_trusted = false;
    std::string signature_algorithm;
    std::vector<std::string> san_domains;
    bool has_ct_info = false;
  };

  // Callback types
  using SecurityCheckCallback = 
      base::OnceCallback<void(const SecurityCheckResult&)>;
  using PhishingDetectionCallback = 
      base::OnceCallback<void(const PhishingDetectionResult&)>;
  using MalwareScanCallback = 
      base::OnceCallback<void(const MalwareScanResult&)>;

  SecurityManager();
  ~SecurityManager();

  // Disallow copy and assign
  SecurityManager(const SecurityManager&) = delete;
  SecurityManager& operator=(const SecurityManager&) = delete;

  // Initialize the security manager
  bool Initialize();

  // URL security checks
  void CheckURL(const std::string& url, SecurityCheckCallback callback);
  bool IsURLSafe(const std::string& url);
  bool IsURLInSafeBrowsingList(const std::string& url);

  // Phishing detection
  void DetectPhishing(const std::string& url, 
                    const std::string& page_content,
                    PhishingDetectionCallback callback);

  // Malware scanning
  void ScanForMalware(const std::string& content, 
                    MalwareScanCallback callback);
  void ScanDownloadedFile(const std::string& file_path, 
                        MalwareScanCallback callback);

  // Certificate validation
  CertificateInfo ValidateCertificate(const std::string& certificate_data);
  bool IsCertificateValid(const std::string& certificate_data);

  // Privacy protection
  void EnableTrackingPrevention(bool enable);
  bool IsTrackingPreventionEnabled() const;
  void SetTrackingPreventionLevel(int level);
  int GetTrackingPreventionLevel() const;

  // Cookie management
  void BlockThirdPartyCookies(bool block);
  bool AreThirdPartyCookiesBlocked() const;
  void ClearCookies();
  void ClearCookiesForDomain(const std::string& domain);

  // Content filtering
  void EnableContentFiltering(bool enable);
  bool IsContentFilteringEnabled() const;
  void SetContentFilteringLevel(int level);
  int GetContentFilteringLevel() const;
  void AddContentFilteringException(const std::string& domain);
  void RemoveContentFilteringException(const std::string& domain);
  std::vector<std::string> GetContentFilteringExceptions() const;

  // Get a weak pointer to this instance
  base::WeakPtr<SecurityManager> GetWeakPtr();

 private:
  // Private implementation
  class Impl;
  std::unique_ptr<Impl> impl_;

  // For weak pointers
  base::WeakPtrFactory<SecurityManager> weak_ptr_factory_{this};
};

}  // namespace security
}  // namespace browser_core

#endif  // BROWSER_CORE_SECURITY_SECURITY_MANAGER_H_