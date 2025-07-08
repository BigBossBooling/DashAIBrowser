// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASOL_CORE_API_GATEWAY_H_
#define ASOL_CORE_API_GATEWAY_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"

namespace asol {
namespace core {

// Forward declarations
class AIServiceProvider;
struct AIRequestParams;

// API Gateway serves as the Control_Core for ASOL, managing rate limits,
// intelligent routing, and security for AI service requests
class APIGateway {
 public:
  // Rate limiting configuration
  struct RateLimitConfig {
    size_t requests_per_minute = 60;
    size_t requests_per_hour = 1000;
    size_t requests_per_day = 10000;
    bool enabled = true;
  };

  // Performance metrics for intelligent routing
  struct ProviderMetrics {
    double average_response_time_ms = 0.0;
    double success_rate = 1.0;
    size_t total_requests = 0;
    size_t failed_requests = 0;
    double cost_per_request = 0.0;
    base::Time last_updated;
  };

  // Request routing decision
  struct RoutingDecision {
    std::string selected_provider_id;
    std::string reason;
    double confidence_score = 1.0;
    bool use_cache = false;
  };

  // Security threat levels
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
    bool allow_request = true;
    std::string mitigation_action;
  };

  using RoutingCallback = base::OnceCallback<void(const RoutingDecision&)>;
  using SecurityCallback = base::OnceCallback<void(const SecurityAssessment&)>;

  APIGateway();
  ~APIGateway();

  // Disallow copy and assign
  APIGateway(const APIGateway&) = delete;
  APIGateway& operator=(const APIGateway&) = delete;

  // Initialize the API Gateway
  bool Initialize();

  // Configure rate limiting
  void ConfigureRateLimit(const std::string& provider_id, 
                         const RateLimitConfig& config);

  // Check if request is within rate limits
  bool CheckRateLimit(const std::string& provider_id, 
                     const std::string& user_id = "");

  // Record request for rate limiting
  void RecordRequest(const std::string& provider_id, 
                    const std::string& user_id = "");

  // Intelligent provider selection based on performance metrics
  void SelectOptimalProvider(const AIRequestParams& params,
                           const std::vector<std::string>& available_providers,
                           RoutingCallback callback);

  // Update provider performance metrics
  void UpdateProviderMetrics(const std::string& provider_id,
                           double response_time_ms,
                           bool success,
                           double cost = 0.0);

  // Get provider performance metrics
  ProviderMetrics GetProviderMetrics(const std::string& provider_id) const;

  // Get all provider metrics for comparison
  std::unordered_map<std::string, ProviderMetrics> GetAllProviderMetrics() const;

  // Security assessment of incoming requests
  void AssessRequestSecurity(const AIRequestParams& params,
                           const std::string& user_context,
                           SecurityCallback callback);

  // Enable/disable security features
  void EnableSecurityFeature(const std::string& feature_name, bool enabled);

  // Get security configuration
  std::unordered_map<std::string, bool> GetSecurityConfig() const;

  // Cost optimization - get cheapest provider for task
  std::string GetCheapestProvider(const std::vector<std::string>& providers) const;

  // Performance optimization - get fastest provider for task
  std::string GetFastestProvider(const std::vector<std::string>& providers) const;

  // Load balancing - distribute requests across providers
  std::string GetLoadBalancedProvider(const std::vector<std::string>& providers) const;

  // Get gateway statistics
  struct GatewayStats {
    size_t total_requests = 0;
    size_t blocked_requests = 0;
    size_t rate_limited_requests = 0;
    double average_routing_time_ms = 0.0;
    std::unordered_map<std::string, size_t> requests_per_provider;
  };
  GatewayStats GetGatewayStats() const;

  // Reset all metrics and statistics
  void ResetMetrics();

 private:
  // Rate limiting tracking
  struct RateLimitTracker {
    std::vector<base::Time> request_timestamps;
    RateLimitConfig config;
  };

  // Clean old rate limit entries
  void CleanRateLimitHistory();

  // Calculate provider score for routing decisions
  double CalculateProviderScore(const std::string& provider_id,
                              const AIRequestParams& params) const;

  // Detect potential security threats in request
  std::vector<std::string> DetectThreats(const AIRequestParams& params,
                                       const std::string& user_context) const;

  // Provider metrics storage
  std::unordered_map<std::string, ProviderMetrics> provider_metrics_;

  // Rate limiting trackers per provider
  std::unordered_map<std::string, RateLimitTracker> rate_limit_trackers_;

  // Security configuration
  std::unordered_map<std::string, bool> security_config_;

  // Gateway statistics
  GatewayStats gateway_stats_;

  // For weak pointers
  base::WeakPtrFactory<APIGateway> weak_ptr_factory_{this};
};

}  // namespace core
}  // namespace asol

#endif  // ASOL_CORE_API_GATEWAY_H_
