// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "asol/core/api_gateway.h"

#include <algorithm>
#include <numeric>
#include <random>

#include "base/logging.h"
#include "base/strings/string_util.h"
#include "asol/core/ai_service_manager.h"

namespace asol {
namespace core {

APIGateway::APIGateway() {
  // Initialize default security configuration
  security_config_["threat_detection"] = true;
  security_config_["content_filtering"] = true;
  security_config_["pii_detection"] = true;
  security_config_["malicious_prompt_detection"] = true;
}

APIGateway::~APIGateway() = default;

bool APIGateway::Initialize() {
  LOG(INFO) << "Initializing API Gateway (Control_Core)";
  
  // Set up default rate limiting for common providers
  RateLimitConfig default_config;
  default_config.requests_per_minute = 60;
  default_config.requests_per_hour = 1000;
  default_config.requests_per_day = 10000;
  
  // Apply to common provider types
  std::vector<std::string> common_providers = {
    "gemini", "openai", "claude", "copilot"
  };
  
  for (const auto& provider : common_providers) {
    ConfigureRateLimit(provider, default_config);
  }
  
  LOG(INFO) << "API Gateway initialized successfully";
  return true;
}

void APIGateway::ConfigureRateLimit(const std::string& provider_id,
                                  const RateLimitConfig& config) {
  rate_limit_trackers_[provider_id].config = config;
  LOG(INFO) << "Rate limit configured for provider: " << provider_id
            << " (RPM: " << config.requests_per_minute << ")";
}

bool APIGateway::CheckRateLimit(const std::string& provider_id,
                               const std::string& user_id) {
  auto it = rate_limit_trackers_.find(provider_id);
  if (it == rate_limit_trackers_.end()) {
    return true;  // No rate limit configured
  }
  
  auto& tracker = it->second;
  if (!tracker.config.enabled) {
    return true;
  }
  
  base::Time now = base::Time::Now();
  auto& timestamps = tracker.request_timestamps;
  
  // Clean old entries
  timestamps.erase(
    std::remove_if(timestamps.begin(), timestamps.end(),
      [&](const base::Time& timestamp) {
        return (now - timestamp).InMinutes() > 60;  // Keep last hour
      }),
    timestamps.end());
  
  // Check minute limit
  size_t requests_last_minute = std::count_if(
    timestamps.begin(), timestamps.end(),
    [&](const base::Time& timestamp) {
      return (now - timestamp).InMinutes() < 1;
    });
  
  if (requests_last_minute >= tracker.config.requests_per_minute) {
    gateway_stats_.rate_limited_requests++;
    LOG(WARNING) << "Rate limit exceeded for provider: " << provider_id
                 << " (requests in last minute: " << requests_last_minute << ")";
    return false;
  }
  
  // Check hour limit
  size_t requests_last_hour = timestamps.size();
  if (requests_last_hour >= tracker.config.requests_per_hour) {
    gateway_stats_.rate_limited_requests++;
    return false;
  }
  
  return true;
}

void APIGateway::RecordRequest(const std::string& provider_id,
                              const std::string& user_id) {
  auto& tracker = rate_limit_trackers_[provider_id];
  tracker.request_timestamps.push_back(base::Time::Now());
  
  gateway_stats_.total_requests++;
  gateway_stats_.requests_per_provider[provider_id]++;
}

void APIGateway::SelectOptimalProvider(
    const AIRequestParams& params,
    const std::vector<std::string>& available_providers,
    RoutingCallback callback) {
  
  if (available_providers.empty()) {
    RoutingDecision decision;
    decision.reason = "No providers available";
    decision.confidence_score = 0.0;
    std::move(callback).Run(decision);
    return;
  }
  
  // Calculate scores for each provider
  std::vector<std::pair<std::string, double>> provider_scores;
  
  for (const auto& provider_id : available_providers) {
    if (!CheckRateLimit(provider_id)) {
      continue;  // Skip rate-limited providers
    }
    
    double score = CalculateProviderScore(provider_id, params);
    provider_scores.emplace_back(provider_id, score);
  }
  
  if (provider_scores.empty()) {
    RoutingDecision decision;
    decision.reason = "All providers rate-limited or unavailable";
    decision.confidence_score = 0.0;
    std::move(callback).Run(decision);
    return;
  }
  
  // Sort by score (highest first)
  std::sort(provider_scores.begin(), provider_scores.end(),
           [](const auto& a, const auto& b) { return a.second > b.second; });
  
  RoutingDecision decision;
  decision.selected_provider_id = provider_scores[0].first;
  decision.confidence_score = provider_scores[0].second;
  decision.reason = "Selected based on performance metrics and availability";
  
  LOG(INFO) << "Selected provider: " << decision.selected_provider_id
            << " (score: " << decision.confidence_score << ")";
  
  std::move(callback).Run(decision);
}

void APIGateway::UpdateProviderMetrics(const std::string& provider_id,
                                     double response_time_ms,
                                     bool success,
                                     double cost) {
  auto& metrics = provider_metrics_[provider_id];
  
  // Update running averages
  double alpha = 0.1;  // Exponential moving average factor
  metrics.average_response_time_ms = 
    alpha * response_time_ms + (1 - alpha) * metrics.average_response_time_ms;
  
  metrics.total_requests++;
  if (!success) {
    metrics.failed_requests++;
  }
  
  metrics.success_rate = 1.0 - (static_cast<double>(metrics.failed_requests) / 
                               metrics.total_requests);
  
  if (cost > 0.0) {
    metrics.cost_per_request = 
      alpha * cost + (1 - alpha) * metrics.cost_per_request;
  }
  
  metrics.last_updated = base::Time::Now();
  
  LOG(INFO) << "Updated metrics for " << provider_id 
            << " - Response time: " << response_time_ms << "ms"
            << ", Success rate: " << metrics.success_rate;
}

APIGateway::ProviderMetrics APIGateway::GetProviderMetrics(
    const std::string& provider_id) const {
  auto it = provider_metrics_.find(provider_id);
  return it != provider_metrics_.end() ? it->second : ProviderMetrics{};
}

std::unordered_map<std::string, APIGateway::ProviderMetrics> 
APIGateway::GetAllProviderMetrics() const {
  return provider_metrics_;
}

void APIGateway::AssessRequestSecurity(const AIRequestParams& params,
                                     const std::string& user_context,
                                     SecurityCallback callback) {
  SecurityAssessment assessment;
  
  if (!security_config_.at("threat_detection")) {
    std::move(callback).Run(assessment);
    return;
  }
  
  // Detect potential threats
  assessment.detected_threats = DetectThreats(params, user_context);
  
  // Determine threat level based on detected threats
  if (assessment.detected_threats.empty()) {
    assessment.threat_level = ThreatLevel::NONE;
  } else if (assessment.detected_threats.size() == 1) {
    assessment.threat_level = ThreatLevel::LOW;
  } else if (assessment.detected_threats.size() <= 3) {
    assessment.threat_level = ThreatLevel::MEDIUM;
  } else {
    assessment.threat_level = ThreatLevel::HIGH;
  }
  
  // Decide whether to allow request
  assessment.allow_request = (assessment.threat_level <= ThreatLevel::MEDIUM);
  
  if (!assessment.allow_request) {
    gateway_stats_.blocked_requests++;
    assessment.mitigation_action = "Request blocked due to security concerns";
    LOG(WARNING) << "Request blocked - Threat level: " 
                 << static_cast<int>(assessment.threat_level);
  }
  
  std::move(callback).Run(assessment);
}

double APIGateway::CalculateProviderScore(const std::string& provider_id,
                                        const AIRequestParams& params) const {
  auto it = provider_metrics_.find(provider_id);
  if (it == provider_metrics_.end()) {
    return 0.5;  // Default score for new providers
  }
  
  const auto& metrics = it->second;
  
  // Weighted scoring: success rate (40%), speed (30%), cost (30%)
  double success_weight = 0.4;
  double speed_weight = 0.3;
  double cost_weight = 0.3;
  
  // Normalize response time (lower is better, max expected 5000ms)
  double speed_score = std::max(0.0, 1.0 - (metrics.average_response_time_ms / 5000.0));
  
  // Normalize cost (lower is better, max expected $0.10 per request)
  double cost_score = std::max(0.0, 1.0 - (metrics.cost_per_request / 0.10));
  
  double total_score = (success_weight * metrics.success_rate) +
                      (speed_weight * speed_score) +
                      (cost_weight * cost_score);
  
  return std::min(1.0, std::max(0.0, total_score));
}

std::vector<std::string> APIGateway::DetectThreats(
    const AIRequestParams& params,
    const std::string& user_context) const {
  
  std::vector<std::string> threats;
  
  if (!security_config_.at("threat_detection")) {
    return threats;
  }
  
  // Check for potential PII in input
  if (security_config_.at("pii_detection")) {
    std::string input_lower = base::ToLowerASCII(params.input_text);
    
    // Simple PII detection patterns
    if (input_lower.find("ssn") != std::string::npos ||
        input_lower.find("social security") != std::string::npos ||
        input_lower.find("credit card") != std::string::npos ||
        input_lower.find("password") != std::string::npos) {
      threats.push_back("potential_pii_detected");
    }
  }
  
  // Check for malicious prompt patterns
  if (security_config_.at("malicious_prompt_detection")) {
    std::string input_lower = base::ToLowerASCII(params.input_text);
    
    if (input_lower.find("ignore previous instructions") != std::string::npos ||
        input_lower.find("jailbreak") != std::string::npos ||
        input_lower.find("pretend you are") != std::string::npos) {
      threats.push_back("potential_prompt_injection");
    }
  }
  
  // Check input length for potential DoS
  if (params.input_text.length() > 50000) {  // 50KB limit
    threats.push_back("excessive_input_length");
  }
  
  return threats;
}

std::string APIGateway::GetCheapestProvider(
    const std::vector<std::string>& providers) const {
  
  std::string cheapest_provider;
  double lowest_cost = std::numeric_limits<double>::max();
  
  for (const auto& provider_id : providers) {
    ProviderMetrics metrics = GetProviderMetrics(provider_id);
    if (metrics.cost_per_request < lowest_cost && metrics.total_requests > 0) {
      lowest_cost = metrics.cost_per_request;
      cheapest_provider = provider_id;
    }
  }
  
  return cheapest_provider.empty() ? providers[0] : cheapest_provider;
}

std::string APIGateway::GetFastestProvider(
    const std::vector<std::string>& providers) const {
  
  std::string fastest_provider;
  double lowest_time = std::numeric_limits<double>::max();
  
  for (const auto& provider_id : providers) {
    ProviderMetrics metrics = GetProviderMetrics(provider_id);
    if (metrics.average_response_time_ms < lowest_time && metrics.total_requests > 0) {
      lowest_time = metrics.average_response_time_ms;
      fastest_provider = provider_id;
    }
  }
  
  return fastest_provider.empty() ? providers[0] : fastest_provider;
}

APIGateway::GatewayStats APIGateway::GetGatewayStats() const {
  return gateway_stats_;
}

void APIGateway::ResetMetrics() {
  provider_metrics_.clear();
  gateway_stats_ = GatewayStats{};
  LOG(INFO) << "API Gateway metrics reset";
}

}  // namespace core
}  // namespace asol
