// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_CORE_SECURITY_AI_PHISHING_DETECTOR_H_
#define BROWSER_CORE_SECURITY_AI_PHISHING_DETECTOR_H_

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "asol/core/ai_service_manager.h"
#include "browser_core/security/phishing_detector.h"

namespace browser_core {
namespace security {

// AIPhishingDetector uses AI to detect sophisticated phishing attempts.
class AIPhishingDetector {
 public:
  // Phishing detection result (extended from PhishingDetector)
  struct AIPhishingDetectionResult : public PhishingDetector::PhishingDetectionResult {
    // Additional AI-specific fields
    std::vector<std::string> reasoning;
    std::unordered_map<std::string, float> feature_scores;
    std::string visual_similarity_target;
    float visual_similarity_score = 0.0f;
    std::vector<std::string> deceptive_techniques;
    std::string recommended_action;
  };

  // Callback for phishing detection results
  using AIPhishingDetectionCallback = 
      base::OnceCallback<void(const AIPhishingDetectionResult&)>;

  AIPhishingDetector();
  ~AIPhishingDetector();

  // Disallow copy and assign
  AIPhishingDetector(const AIPhishingDetector&) = delete;
  AIPhishingDetector& operator=(const AIPhishingDetector&) = delete;

  // Initialize with AI service manager
  bool Initialize(asol::core::AIServiceManager* ai_service_manager);

  // Detect phishing using AI
  void DetectPhishing(const std::string& url, 
                    const std::string& page_content,
                    const std::string& page_screenshot,
                    AIPhishingDetectionCallback callback);

  // Analyze URL for phishing indicators
  void AnalyzeURL(const std::string& url,
                base::OnceCallback<void(float score, 
                                      const std::vector<std::string>& reasons)> callback);

  // Analyze content for phishing indicators
  void AnalyzeContent(const std::string& content,
                    base::OnceCallback<void(float score, 
                                          const std::vector<std::string>& reasons)> callback);

  // Analyze visual similarity to known brands
  void AnalyzeVisualSimilarity(const std::string& screenshot,
                             base::OnceCallback<void(const std::string& target_brand, 
                                                   float similarity_score)> callback);

  // Check for known phishing patterns
  void CheckPhishingPatterns(const std::string& content,
                           base::OnceCallback<void(const std::vector<std::string>& patterns)> callback);

  // Enable/disable AI phishing detection
  void Enable(bool enable);
  bool IsEnabled() const;

  // Set detection sensitivity (0.0 to 1.0)
  void SetSensitivity(float sensitivity);
  float GetSensitivity() const;

  // Get a weak pointer to this instance
  base::WeakPtr<AIPhishingDetector> GetWeakPtr();

 private:
  // Helper methods
  void CombineDetectionResults(float url_score,
                             const std::vector<std::string>& url_reasons,
                             float content_score,
                             const std::vector<std::string>& content_reasons,
                             const std::string& visual_target,
                             float visual_score,
                             const std::vector<std::string>& patterns,
                             AIPhishingDetectionCallback callback);
  
  std::string GenerateRecommendedAction(bool is_phishing,
                                      float confidence,
                                      const std::string& target_brand);

  // AI service manager
  asol::core::AIServiceManager* ai_service_manager_ = nullptr;

  // State
  bool is_enabled_ = true;
  float sensitivity_ = 0.7f;

  // For weak pointers
  base::WeakPtrFactory<AIPhishingDetector> weak_ptr_factory_{this};
};

}  // namespace security
}  // namespace browser_core

#endif  // BROWSER_CORE_SECURITY_AI_PHISHING_DETECTOR_H_