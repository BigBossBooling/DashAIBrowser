// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASOL_CORE_PERFORMANCE_TRACKER_H_
#define ASOL_CORE_PERFORMANCE_TRACKER_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"

namespace asol {
namespace core {

// Performance tracker for AI service providers and adapters
class PerformanceTracker {
 public:
  // Performance metrics for a provider
  struct ProviderMetrics {
    std::string provider_id;
    double average_response_time_ms = 0.0;
    double success_rate = 1.0;
    size_t total_requests = 0;
    size_t successful_requests = 0;
    size_t failed_requests = 0;
    double average_cost_per_request = 0.0;
    double total_cost = 0.0;
    base::Time last_request_time;
    base::Time first_request_time;
  };

  // Temporal intelligence for predictive analytics
  struct TemporalPattern {
    std::string pattern_id;
    std::vector<base::Time> request_times;
    double predicted_next_request_time = 0.0;
    std::string usage_pattern_type;
    double confidence_score = 0.0;
  };

  PerformanceTracker();
  ~PerformanceTracker();

  // Disallow copy and assign
  PerformanceTracker(const PerformanceTracker&) = delete;
  PerformanceTracker& operator=(const PerformanceTracker&) = delete;

  // Initialize the performance tracker
  bool Initialize();

  // Record request metrics
  void RecordRequest(const std::string& provider_id,
                   base::TimeDelta response_time,
                   bool success,
                   double cost = 0.0);

  // Get metrics for a specific provider
  ProviderMetrics GetProviderMetrics(const std::string& provider_id) const;

  // Get metrics for all providers
  std::unordered_map<std::string, ProviderMetrics> GetAllProviderMetrics() const;

  // Get the fastest provider from a list
  std::string GetFastestProvider(const std::vector<std::string>& provider_ids) const;

  // Get the most cost-effective provider
  std::string GetCheapestProvider(const std::vector<std::string>& provider_ids) const;

  // Get the most reliable provider (highest success rate)
  std::string GetMostReliableProvider(const std::vector<std::string>& provider_ids) const;

  // Temporal intelligence features
  void AnalyzeUsagePatterns(const std::string& user_id);
  std::vector<TemporalPattern> GetTemporalPatterns(const std::string& user_id) const;
  double PredictNextRequestTime(const std::string& user_id, const std::string& provider_id) const;

  // Performance optimization recommendations
  struct OptimizationRecommendation {
    std::string recommendation_type;
    std::string description;
    std::vector<std::string> affected_providers;
    double potential_improvement = 0.0;
  };
  std::vector<OptimizationRecommendation> GetOptimizationRecommendations() const;

  // Reset all metrics
  void ResetMetrics();

  // Get a weak pointer to this instance
  base::WeakPtr<PerformanceTracker> GetWeakPtr();

 private:
  // Helper methods
  void UpdateProviderMetrics(const std::string& provider_id,
                           base::TimeDelta response_time,
                           bool success,
                           double cost);

  double CalculateAverageResponseTime(const std::string& provider_id) const;
  double CalculateSuccessRate(const std::string& provider_id) const;

  // Storage for provider metrics
  std::unordered_map<std::string, ProviderMetrics> provider_metrics_;

  // Temporal patterns storage
  std::unordered_map<std::string, std::vector<TemporalPattern>> user_patterns_;

  // For weak pointers
  base::WeakPtrFactory<PerformanceTracker> weak_ptr_factory_{this};
};

}  // namespace core
}  // namespace asol

#endif  // ASOL_CORE_PERFORMANCE_TRACKER_H_
