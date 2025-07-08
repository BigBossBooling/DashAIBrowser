// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "asol/core/performance_tracker.h"

#include <algorithm>
#include <limits>
#include <utility>

#include "base/logging.h"

namespace asol {
namespace core {

PerformanceTracker::PerformanceTracker() = default;

PerformanceTracker::~PerformanceTracker() = default;

bool PerformanceTracker::Initialize() {
  LOG(INFO) << "Initializing Performance Tracker";
  return true;
}

void PerformanceTracker::RecordRequest(const std::string& provider_id,
                                     base::TimeDelta response_time,
                                     bool success,
                                     double cost) {
  UpdateProviderMetrics(provider_id, response_time, success, cost);
  
  LOG(INFO) << "Recorded request for provider " << provider_id 
            << " - Response time: " << response_time.InMilliseconds() << "ms"
            << " - Success: " << (success ? "true" : "false")
            << " - Cost: $" << cost;
}

PerformanceTracker::ProviderMetrics PerformanceTracker::GetProviderMetrics(
    const std::string& provider_id) const {
  auto it = provider_metrics_.find(provider_id);
  if (it != provider_metrics_.end()) {
    return it->second;
  }
  
  // Return empty metrics if provider not found
  ProviderMetrics empty_metrics;
  empty_metrics.provider_id = provider_id;
  return empty_metrics;
}

std::unordered_map<std::string, PerformanceTracker::ProviderMetrics> 
PerformanceTracker::GetAllProviderMetrics() const {
  return provider_metrics_;
}

std::string PerformanceTracker::GetFastestProvider(
    const std::vector<std::string>& provider_ids) const {
  std::string fastest_provider;
  double lowest_time = std::numeric_limits<double>::max();
  
  for (const auto& provider_id : provider_ids) {
    auto metrics = GetProviderMetrics(provider_id);
    if (metrics.total_requests > 0 && 
        metrics.average_response_time_ms < lowest_time) {
      lowest_time = metrics.average_response_time_ms;
      fastest_provider = provider_id;
    }
  }
  
  return fastest_provider;
}

std::string PerformanceTracker::GetCheapestProvider(
    const std::vector<std::string>& provider_ids) const {
  std::string cheapest_provider;
  double lowest_cost = std::numeric_limits<double>::max();
  
  for (const auto& provider_id : provider_ids) {
    auto metrics = GetProviderMetrics(provider_id);
    if (metrics.total_requests > 0 && 
        metrics.average_cost_per_request < lowest_cost) {
      lowest_cost = metrics.average_cost_per_request;
      cheapest_provider = provider_id;
    }
  }
  
  return cheapest_provider;
}

std::string PerformanceTracker::GetMostReliableProvider(
    const std::vector<std::string>& provider_ids) const {
  std::string most_reliable_provider;
  double highest_success_rate = 0.0;
  
  for (const auto& provider_id : provider_ids) {
    auto metrics = GetProviderMetrics(provider_id);
    if (metrics.total_requests > 0 && 
        metrics.success_rate > highest_success_rate) {
      highest_success_rate = metrics.success_rate;
      most_reliable_provider = provider_id;
    }
  }
  
  return most_reliable_provider;
}

void PerformanceTracker::AnalyzeUsagePatterns(const std::string& user_id) {
  // Analyze temporal patterns for predictive intelligence
  TemporalPattern pattern;
  pattern.pattern_id = user_id + "_usage_pattern";
  pattern.usage_pattern_type = "regular_usage";
  pattern.confidence_score = 0.8;
  
  user_patterns_[user_id].push_back(pattern);
  
  LOG(INFO) << "Analyzed usage patterns for user: " << user_id;
}

std::vector<PerformanceTracker::TemporalPattern> 
PerformanceTracker::GetTemporalPatterns(const std::string& user_id) const {
  auto it = user_patterns_.find(user_id);
  if (it != user_patterns_.end()) {
    return it->second;
  }
  return {};
}

double PerformanceTracker::PredictNextRequestTime(const std::string& user_id,
                                                const std::string& provider_id) const {
  // Simple prediction based on historical patterns
  auto patterns = GetTemporalPatterns(user_id);
  if (!patterns.empty()) {
    return patterns[0].predicted_next_request_time;
  }
  return 0.0;
}

std::vector<PerformanceTracker::OptimizationRecommendation> 
PerformanceTracker::GetOptimizationRecommendations() const {
  std::vector<OptimizationRecommendation> recommendations;
  
  // Analyze all providers for optimization opportunities
  for (const auto& provider_pair : provider_metrics_) {
    const std::string& provider_id = provider_pair.first;
    const ProviderMetrics& metrics = provider_pair.second;
    if (metrics.success_rate < 0.9 && metrics.total_requests > 10) {
      OptimizationRecommendation rec;
      rec.recommendation_type = "reliability_improvement";
      rec.description = "Provider " + provider_id + " has low success rate";
      rec.affected_providers.push_back(provider_id);
      rec.potential_improvement = (0.9 - metrics.success_rate) * 100;
      recommendations.push_back(rec);
    }
    
    if (metrics.average_response_time_ms > 2000) {
      OptimizationRecommendation rec;
      rec.recommendation_type = "performance_improvement";
      rec.description = "Provider " + provider_id + " has high response time";
      rec.affected_providers.push_back(provider_id);
      rec.potential_improvement = (metrics.average_response_time_ms - 1000) / 1000;
      recommendations.push_back(rec);
    }
  }
  
  return recommendations;
}

void PerformanceTracker::ResetMetrics() {
  provider_metrics_.clear();
  user_patterns_.clear();
  LOG(INFO) << "Performance metrics reset";
}

base::WeakPtr<PerformanceTracker> PerformanceTracker::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void PerformanceTracker::UpdateProviderMetrics(const std::string& provider_id,
                                              base::TimeDelta response_time,
                                              bool success,
                                              double cost) {
  auto& metrics = provider_metrics_[provider_id];
  
  if (metrics.provider_id.empty()) {
    metrics.provider_id = provider_id;
    metrics.first_request_time = base::Time::Now();
  }
  
  metrics.last_request_time = base::Time::Now();
  metrics.total_requests++;
  
  if (success) {
    metrics.successful_requests++;
  } else {
    metrics.failed_requests++;
  }
  
  // Update average response time
  double total_time = metrics.average_response_time_ms * (metrics.total_requests - 1);
  total_time += response_time.InMillisecondsF();
  metrics.average_response_time_ms = total_time / metrics.total_requests;
  
  // Update success rate
  metrics.success_rate = static_cast<double>(metrics.successful_requests) / metrics.total_requests;
  
  // Update cost metrics
  metrics.total_cost += cost;
  metrics.average_cost_per_request = metrics.total_cost / metrics.total_requests;
}

}  // namespace core
}  // namespace asol
