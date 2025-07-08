// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_CORE_SECURITY_PRIVACY_MANAGER_H_
#define BROWSER_CORE_SECURITY_PRIVACY_MANAGER_H_

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace browser_core {
namespace security {

// PrivacyManager handles privacy-related features in the browser.
class PrivacyManager {
 public:
  // Privacy protection levels
  enum class ProtectionLevel {
    STANDARD,  // Blocks known trackers
    STRICT,    // Blocks all third-party trackers
    CUSTOM     // Custom settings
  };

  // Tracker types
  enum class TrackerType {
    ANALYTICS,
    ADVERTISING,
    SOCIAL,
    CONTENT,
    CRYPTOMINING,
    FINGERPRINTING,
    OTHER
  };

  // Tracker info
  struct TrackerInfo {
    std::string domain;
    TrackerType type;
    std::string company;
    std::string description;
    bool is_blocked;
  };

  PrivacyManager();
  ~PrivacyManager();

  // Disallow copy and assign
  PrivacyManager(const PrivacyManager&) = delete;
  PrivacyManager& operator=(const PrivacyManager&) = delete;

  // Initialize the privacy manager
  bool Initialize();

  // Tracking prevention
  void SetProtectionLevel(ProtectionLevel level);
  ProtectionLevel GetProtectionLevel() const;
  void EnableTrackingPrevention(bool enable);
  bool IsTrackingPreventionEnabled() const;

  // Cookie management
  void BlockThirdPartyCookies(bool block);
  bool AreThirdPartyCookiesBlocked() const;
  void ClearCookies();
  void ClearCookiesForDomain(const std::string& domain);
  void SetCookiePolicy(const std::string& domain, bool allow);
  bool IsCookieAllowed(const std::string& domain) const;

  // Fingerprinting protection
  void EnableFingerprintingProtection(bool enable);
  bool IsFingerprintingProtectionEnabled() const;
  void SetFingerprintingProtectionLevel(int level);
  int GetFingerprintingProtectionLevel() const;

  // Tracker blocking
  void BlockTracker(const std::string& domain, bool block);
  bool IsTrackerBlocked(const std::string& domain) const;
  void BlockTrackerType(TrackerType type, bool block);
  bool IsTrackerTypeBlocked(TrackerType type) const;
  std::vector<TrackerInfo> GetBlockedTrackers() const;

  // Exceptions
  void AddException(const std::string& domain);
  void RemoveException(const std::string& domain);
  bool IsException(const std::string& domain) const;
  std::vector<std::string> GetExceptions() const;

  // Do Not Track
  void EnableDoNotTrack(bool enable);
  bool IsDoNotTrackEnabled() const;

  // Local storage
  void ClearLocalStorage();
  void ClearLocalStorageForDomain(const std::string& domain);

 private:
  // Private implementation
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace security
}  // namespace browser_core

#endif  // BROWSER_CORE_SECURITY_PRIVACY_MANAGER_H_