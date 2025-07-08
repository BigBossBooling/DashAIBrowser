// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASOL_CORE_PRIVACY_PROXY_H_
#define ASOL_CORE_PRIVACY_PROXY_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"

namespace asol {
namespace core {

// PrivacyProxy implements the Data Minimization & Privacy-Preserving Proxy (DMP)
// component of the ASOL. It filters, anonymizes, or redacts Personally Identifiable
// Information (PII) based on user consent settings before data is sent to external AI services.
class PrivacyProxy {
 public:
  // Privacy level for data processing
  enum class PrivacyLevel {
    MINIMAL,     // Basic PII filtering (names, emails, phone numbers)
    STANDARD,    // Standard filtering (includes locations, dates, financial info)
    STRICT,      // Strict filtering (includes device info, browsing patterns)
    MAXIMUM      // Maximum privacy (aggressive filtering, may impact quality)
  };

  // Data category for consent management
  enum class DataCategory {
    PERSONAL_INFO,       // Names, addresses, contact info
    LOCATION_DATA,       // Geographic locations, travel history
    FINANCIAL_DATA,      // Financial information, transaction history
    HEALTH_DATA,         // Health-related information
    BROWSING_HISTORY,    // Web browsing history
    DEVICE_INFO,         // Device identifiers, hardware info
    SOCIAL_CONNECTIONS,  // Social network, contacts
    CUSTOM_CATEGORY      // User-defined category
  };

  // Consent settings for a data category
  struct ConsentSetting {
    DataCategory category;
    bool allowed;
    std::string custom_category_name;  // Only used if category is CUSTOM_CATEGORY
  };

  // Result of privacy processing
  struct ProcessingResult {
    std::string processed_text;
    bool was_modified;
    int num_redactions;
    std::unordered_map<std::string, int> redaction_categories;
  };

  // Callback for privacy processing
  using ProcessingCallback = 
      base::OnceCallback<void(const ProcessingResult& result)>;

  PrivacyProxy();
  ~PrivacyProxy();

  // Disallow copy and assign
  PrivacyProxy(const PrivacyProxy&) = delete;
  PrivacyProxy& operator=(const PrivacyProxy&) = delete;

  // Initialize the privacy proxy
  bool Initialize();

  // Process text to remove/redact PII based on privacy settings
  void ProcessText(const std::string& input_text, ProcessingCallback callback);

  // Process text synchronously (for simpler use cases)
  ProcessingResult ProcessTextSync(const std::string& input_text);

  // Set the privacy level
  void SetPrivacyLevel(PrivacyLevel level);
  PrivacyLevel GetPrivacyLevel() const;

  // Manage consent settings
  void SetConsentSetting(const ConsentSetting& setting);
  void SetConsentSettings(const std::vector<ConsentSetting>& settings);
  ConsentSetting GetConsentSetting(DataCategory category) const;
  std::vector<ConsentSetting> GetAllConsentSettings() const;

  // Add custom PII patterns to detect and redact
  void AddCustomPattern(const std::string& pattern_name, 
                       const std::string& regex_pattern);
  void RemoveCustomPattern(const std::string& pattern_name);

  // Get a weak pointer to this instance
  base::WeakPtr<PrivacyProxy> GetWeakPtr();

 private:
  // Helper methods for PII detection and redaction
  bool ShouldRedactCategory(DataCategory category) const;
  std::string RedactPII(const std::string& text, 
                      std::unordered_map<std::string, int>* redaction_categories);
  
  // Apply specific redaction techniques
  std::string RedactNames(const std::string& text, int* count);
  std::string RedactEmailAddresses(const std::string& text, int* count);
  std::string RedactPhoneNumbers(const std::string& text, int* count);
  std::string RedactAddresses(const std::string& text, int* count);
  std::string RedactFinancialInfo(const std::string& text, int* count);
  std::string RedactHealthInfo(const std::string& text, int* count);
  std::string RedactLocationData(const std::string& text, int* count);
  std::string RedactDeviceInfo(const std::string& text, int* count);
  std::string RedactCustomPatterns(const std::string& text, 
                                 std::unordered_map<std::string, int>* pattern_counts);

  // Current privacy level
  PrivacyLevel privacy_level_ = PrivacyLevel::STANDARD;

  // Consent settings for different data categories
  std::unordered_map<DataCategory, ConsentSetting> consent_settings_;

  // Custom patterns for PII detection
  std::unordered_map<std::string, std::string> custom_patterns_;

  // For weak pointers
  base::WeakPtrFactory<PrivacyProxy> weak_ptr_factory_{this};
};

}  // namespace core
}  // namespace asol

#endif  // ASOL_CORE_PRIVACY_PROXY_H_