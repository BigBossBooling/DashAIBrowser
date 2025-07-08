// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASOL_CORE_ENHANCED_SECURITY_MANAGER_H_
#define ASOL_CORE_ENHANCED_SECURITY_MANAGER_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "asol/core/ai_service_provider.h"

namespace asol {
namespace core {

// Enhanced security manager with AI-driven threat detection
class EnhancedSecurityManager {
 public:
  // Threat detection levels
  enum class ThreatLevel {
    NONE = 0,
    LOW = 1,
    MEDIUM = 2,
    HIGH = 3,
    CRITICAL = 4
  };

  // Security assessment result
  struct SecurityAssessment {
    ThreatLevel threat_level = ThreatLevel::NONE;
    std::vector<std::string> detected_threats;
    std::vector<std::string> mitigation_actions;
    double confidence_score = 1.0;
    bool allow_request = true;
  };

  // Behavioral anomaly detection result
  struct AnomalyDetection {
    bool anomaly_detected = false;
    std::string anomaly_type;
    double anomaly_score = 0.0;
    std::string description;
    std::vector<std::string> recommended_actions;
  };

  using SecurityCallback = base::OnceCallback<void(const SecurityAssessment&)>;
  using AnomalyCallback = base::OnceCallback<void(const AnomalyDetection&)>;

  EnhancedSecurityManager();
  ~EnhancedSecurityManager();

  // Disallow copy and assign
  EnhancedSecurityManager(const EnhancedSecurityManager&) = delete;
  EnhancedSecurityManager& operator=(const EnhancedSecurityManager&) = delete;

  // Initialize the security manager
  bool Initialize();

  // AI-driven threat detection for prompts
  void DetectPromptThreats(const std::string& prompt,
                         const std::string& user_context,
                         SecurityCallback callback);

  // Real-time malicious prompt detection
  void DetectMaliciousPrompt(const std::string& prompt,
                           SecurityCallback callback);

  // Behavioral anomaly detection for AI requests
  void DetectBehavioralAnomaly(const AIServiceProvider::AIRequestParams& params,
                             const std::string& user_id,
                             AnomalyCallback callback);

  // Content analysis for malicious scripts
  void AnalyzeContentSecurity(const std::string& content,
                            const std::string& content_type,
                            SecurityCallback callback);

  // URL scanning for threats
  void ScanURL(const std::string& url,
             SecurityCallback callback);

  // Update threat intelligence database
  void UpdateThreatIntelligence(const std::vector<std::string>& threat_indicators);

  // Get security statistics
  struct SecurityStats {
    size_t total_assessments = 0;
    size_t threats_detected = 0;
    size_t requests_blocked = 0;
    std::unordered_map<std::string, size_t> threat_types;
  };
  SecurityStats GetSecurityStats() const;

  // Enable/disable security features
  void EnableFeature(const std::string& feature_name, bool enabled);
  bool IsFeatureEnabled(const std::string& feature_name) const;

  // Get a weak pointer to this instance
  base::WeakPtr<EnhancedSecurityManager> GetWeakPtr();

 private:
  // Helper methods for threat detection
  std::vector<std::string> DetectPromptInjection(const std::string& prompt);
  std::vector<std::string> DetectDataExfiltration(const std::string& prompt);
  std::vector<std::string> DetectMaliciousInstructions(const std::string& prompt);
  
  // Behavioral analysis helpers
  bool IsAnomalousRequestPattern(const AIServiceProvider::AIRequestParams& params,
                               const std::string& user_id);
  double CalculateAnomalyScore(const AIServiceProvider::AIRequestParams& params);

  // Security configuration
  std::unordered_map<std::string, bool> enabled_features_;
  
  // Threat intelligence data
  std::vector<std::string> known_threats_;
  std::unordered_map<std::string, ThreatLevel> threat_patterns_;
  
  // Statistics
  SecurityStats security_stats_;

  // For weak pointers
  base::WeakPtrFactory<EnhancedSecurityManager> weak_ptr_factory_{this};
};

}  // namespace core
}  // namespace asol

#endif  // ASOL_CORE_ENHANCED_SECURITY_MANAGER_H_
