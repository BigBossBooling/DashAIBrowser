// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "asol/core/enhanced_security_manager.h"

#include <algorithm>
#include <utility>

#include "base/logging.h"
#include "base/strings/string_util.h"

namespace asol {
namespace core {

EnhancedSecurityManager::EnhancedSecurityManager() {
  // Initialize default enabled features
  enabled_features_["prompt_threat_detection"] = true;
  enabled_features_["malicious_prompt_detection"] = true;
  enabled_features_["behavioral_anomaly_detection"] = true;
  enabled_features_["content_security_analysis"] = true;
  enabled_features_["url_scanning"] = true;
  
  // Initialize known threat patterns
  threat_patterns_["prompt_injection"] = ThreatLevel::HIGH;
  threat_patterns_["data_exfiltration"] = ThreatLevel::CRITICAL;
  threat_patterns_["malicious_instructions"] = ThreatLevel::HIGH;
  threat_patterns_["social_engineering"] = ThreatLevel::MEDIUM;
}

EnhancedSecurityManager::~EnhancedSecurityManager() = default;

bool EnhancedSecurityManager::Initialize() {
  LOG(INFO) << "Initializing Enhanced Security Manager";
  
  // Load threat intelligence database
  known_threats_ = {
    "ignore previous instructions",
    "forget everything above",
    "system prompt override",
    "jailbreak attempt",
    "extract training data",
    "reveal system information"
  };
  
  return true;
}

void EnhancedSecurityManager::DetectPromptThreats(const std::string& prompt,
                                                const std::string& user_context,
                                                SecurityCallback callback) {
  if (!IsFeatureEnabled("prompt_threat_detection")) {
    SecurityAssessment assessment;
    assessment.allow_request = true;
    std::move(callback).Run(assessment);
    return;
  }

  SecurityAssessment assessment;
  
  // Detect various threat types
  auto injection_threats = DetectPromptInjection(prompt);
  auto exfiltration_threats = DetectDataExfiltration(prompt);
  auto malicious_threats = DetectMaliciousInstructions(prompt);
  
  // Combine all detected threats
  assessment.detected_threats.insert(assessment.detected_threats.end(),
                                   injection_threats.begin(), injection_threats.end());
  assessment.detected_threats.insert(assessment.detected_threats.end(),
                                   exfiltration_threats.begin(), exfiltration_threats.end());
  assessment.detected_threats.insert(assessment.detected_threats.end(),
                                   malicious_threats.begin(), malicious_threats.end());
  
  // Determine threat level
  if (!assessment.detected_threats.empty()) {
    if (!exfiltration_threats.empty()) {
      assessment.threat_level = ThreatLevel::CRITICAL;
    } else if (!injection_threats.empty() || !malicious_threats.empty()) {
      assessment.threat_level = ThreatLevel::HIGH;
    } else {
      assessment.threat_level = ThreatLevel::MEDIUM;
    }
    
    assessment.allow_request = (assessment.threat_level < ThreatLevel::HIGH);
    
    if (!assessment.allow_request) {
      assessment.mitigation_actions.push_back("Block request");
      assessment.mitigation_actions.push_back("Log security incident");
      assessment.mitigation_actions.push_back("Notify user of potential threat");
    }
  }
  
  assessment.confidence_score = 0.85;
  security_stats_.total_assessments++;
  if (!assessment.detected_threats.empty()) {
    security_stats_.threats_detected++;
    if (!assessment.allow_request) {
      security_stats_.requests_blocked++;
    }
  }
  
  std::move(callback).Run(assessment);
}

void EnhancedSecurityManager::DetectMaliciousPrompt(const std::string& prompt,
                                                  SecurityCallback callback) {
  if (!IsFeatureEnabled("malicious_prompt_detection")) {
    SecurityAssessment assessment;
    assessment.allow_request = true;
    std::move(callback).Run(assessment);
    return;
  }

  SecurityAssessment assessment;
  
  // Check against known malicious patterns
  std::string lower_prompt = base::ToLowerASCII(prompt);
  
  for (const auto& threat : known_threats_) {
    if (lower_prompt.find(base::ToLowerASCII(threat)) != std::string::npos) {
      assessment.detected_threats.push_back("Malicious pattern: " + threat);
      assessment.threat_level = ThreatLevel::HIGH;
    }
  }
  
  // Check for suspicious keywords
  std::vector<std::string> suspicious_keywords = {
    "bypass", "override", "hack", "exploit", "vulnerability",
    "admin", "root", "system", "debug", "internal"
  };
  
  for (const auto& keyword : suspicious_keywords) {
    if (lower_prompt.find(keyword) != std::string::npos) {
      assessment.detected_threats.push_back("Suspicious keyword: " + keyword);
      if (assessment.threat_level < ThreatLevel::MEDIUM) {
        assessment.threat_level = ThreatLevel::MEDIUM;
      }
    }
  }
  
  assessment.allow_request = (assessment.threat_level < ThreatLevel::HIGH);
  assessment.confidence_score = 0.75;
  
  std::move(callback).Run(assessment);
}

void EnhancedSecurityManager::DetectBehavioralAnomaly(
    const AIServiceProvider::AIRequestParams& params,
    const std::string& user_id,
    AnomalyCallback callback) {
  
  if (!IsFeatureEnabled("behavioral_anomaly_detection")) {
    AnomalyDetection detection;
    std::move(callback).Run(detection);
    return;
  }

  AnomalyDetection detection;
  
  // Check for anomalous request patterns
  if (IsAnomalousRequestPattern(params, user_id)) {
    detection.anomaly_detected = true;
    detection.anomaly_type = "unusual_request_pattern";
    detection.anomaly_score = CalculateAnomalyScore(params);
    detection.description = "Detected unusual request pattern for user";
    detection.recommended_actions.push_back("Monitor user activity");
    detection.recommended_actions.push_back("Apply additional security checks");
  }
  
  // Check for rapid-fire requests (potential automation)
  if (params.input_text.length() > 10000) {
    detection.anomaly_detected = true;
    detection.anomaly_type = "oversized_request";
    detection.anomaly_score = 0.8;
    detection.description = "Request size exceeds normal parameters";
    detection.recommended_actions.push_back("Limit request size");
  }
  
  std::move(callback).Run(detection);
}

void EnhancedSecurityManager::AnalyzeContentSecurity(const std::string& content,
                                                   const std::string& content_type,
                                                   SecurityCallback callback) {
  if (!IsFeatureEnabled("content_security_analysis")) {
    SecurityAssessment assessment;
    assessment.allow_request = true;
    std::move(callback).Run(assessment);
    return;
  }

  SecurityAssessment assessment;
  
  // Analyze based on content type
  if (content_type == "javascript" || content_type == "script") {
    if (content.find("eval(") != std::string::npos ||
        content.find("document.cookie") != std::string::npos ||
        content.find("localStorage") != std::string::npos) {
      assessment.detected_threats.push_back("Potentially malicious script content");
      assessment.threat_level = ThreatLevel::HIGH;
      assessment.allow_request = false;
    }
  }
  
  assessment.confidence_score = 0.7;
  std::move(callback).Run(assessment);
}

void EnhancedSecurityManager::ScanURL(const std::string& url,
                                    SecurityCallback callback) {
  if (!IsFeatureEnabled("url_scanning")) {
    SecurityAssessment assessment;
    assessment.allow_request = true;
    std::move(callback).Run(assessment);
    return;
  }

  SecurityAssessment assessment;
  
  // Basic URL security checks
  std::vector<std::string> suspicious_domains = {
    "malware.example.com",
    "phishing.test.com",
    "suspicious.domain.net"
  };
  
  for (const auto& domain : suspicious_domains) {
    if (url.find(domain) != std::string::npos) {
      assessment.detected_threats.push_back("Suspicious domain: " + domain);
      assessment.threat_level = ThreatLevel::HIGH;
      assessment.allow_request = false;
    }
  }
  
  assessment.confidence_score = 0.9;
  std::move(callback).Run(assessment);
}

void EnhancedSecurityManager::UpdateThreatIntelligence(
    const std::vector<std::string>& threat_indicators) {
  known_threats_.insert(known_threats_.end(), 
                       threat_indicators.begin(), threat_indicators.end());
  LOG(INFO) << "Updated threat intelligence with " << threat_indicators.size() 
            << " new indicators";
}

EnhancedSecurityManager::SecurityStats EnhancedSecurityManager::GetSecurityStats() const {
  return security_stats_;
}

void EnhancedSecurityManager::EnableFeature(const std::string& feature_name, bool enabled) {
  enabled_features_[feature_name] = enabled;
  LOG(INFO) << "Security feature " << feature_name << " " 
            << (enabled ? "enabled" : "disabled");
}

bool EnhancedSecurityManager::IsFeatureEnabled(const std::string& feature_name) const {
  auto it = enabled_features_.find(feature_name);
  return it != enabled_features_.end() && it->second;
}

base::WeakPtr<EnhancedSecurityManager> EnhancedSecurityManager::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

std::vector<std::string> EnhancedSecurityManager::DetectPromptInjection(const std::string& prompt) {
  std::vector<std::string> threats;
  std::string lower_prompt = base::ToLowerASCII(prompt);
  
  std::vector<std::string> injection_patterns = {
    "ignore previous instructions",
    "forget everything above",
    "new instructions:",
    "system override",
    "admin mode"
  };
  
  for (const auto& pattern : injection_patterns) {
    if (lower_prompt.find(pattern) != std::string::npos) {
      threats.push_back("Prompt injection detected: " + pattern);
    }
  }
  
  return threats;
}

std::vector<std::string> EnhancedSecurityManager::DetectDataExfiltration(const std::string& prompt) {
  std::vector<std::string> threats;
  std::string lower_prompt = base::ToLowerASCII(prompt);
  
  std::vector<std::string> exfiltration_patterns = {
    "show me your training data",
    "reveal system prompt",
    "extract internal information",
    "dump configuration",
    "show source code"
  };
  
  for (const auto& pattern : exfiltration_patterns) {
    if (lower_prompt.find(pattern) != std::string::npos) {
      threats.push_back("Data exfiltration attempt: " + pattern);
    }
  }
  
  return threats;
}

std::vector<std::string> EnhancedSecurityManager::DetectMaliciousInstructions(const std::string& prompt) {
  std::vector<std::string> threats;
  std::string lower_prompt = base::ToLowerASCII(prompt);
  
  std::vector<std::string> malicious_patterns = {
    "generate harmful content",
    "create malware",
    "bypass security",
    "exploit vulnerability",
    "social engineering"
  };
  
  for (const auto& pattern : malicious_patterns) {
    if (lower_prompt.find(pattern) != std::string::npos) {
      threats.push_back("Malicious instruction detected: " + pattern);
    }
  }
  
  return threats;
}

bool EnhancedSecurityManager::IsAnomalousRequestPattern(
    const AIServiceProvider::AIRequestParams& params,
    const std::string& user_id) {
  // Simple heuristics for anomaly detection
  return params.input_text.length() > 5000 || 
         params.input_text.find("repeat") != std::string::npos;
}

double EnhancedSecurityManager::CalculateAnomalyScore(
    const AIServiceProvider::AIRequestParams& params) {
  double score = 0.0;
  
  // Length-based scoring
  if (params.input_text.length() > 1000) score += 0.3;
  if (params.input_text.length() > 5000) score += 0.4;
  
  // Pattern-based scoring
  if (params.input_text.find("repeat") != std::string::npos) score += 0.2;
  if (params.input_text.find("system") != std::string::npos) score += 0.1;
  
  return std::min(score, 1.0);
}

}  // namespace core
}  // namespace asol
