// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASOL_CORE_ECHOSPHERE_BRIDGE_H_
#define ASOL_CORE_ECHOSPHERE_BRIDGE_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "asol/core/ai_service_provider.h"

namespace asol {
namespace core {

// EchoSphereBridge provides integration with EchoSphere AI-vCPU components
// implementing the Behavioral Orchestrator (EBO) and Enrichment Engine
class EchoSphereBridge {
 public:
  // EchoSphere context types
  enum class ContextType {
    HOLOGRAPHIC_MEMORY,
    L3_CACHE,
    TEMPORAL_INTELLIGENCE,
    CREATIVE_GENERATOR,
    SECURITY_GUARDIAN,
    FUSION_CORE
  };

  // Behavioral orchestration result
  struct BehavioralDecision {
    std::string action_request;
    std::string interaction_goal;
    std::unordered_map<std::string, std::string> context_modifiers;
    double confidence_score = 1.0;
  };

  // Context enrichment result
  struct EnrichmentResult {
    std::string enriched_prompt;
    std::vector<std::string> relevant_contexts;
    std::unordered_map<std::string, std::string> persona_details;
    std::string knowledge_cache_data;
  };

  using BehavioralCallback = base::OnceCallback<void(const BehavioralDecision&)>;
  using EnrichmentCallback = base::OnceCallback<void(const EnrichmentResult&)>;

  EchoSphereBridge();
  ~EchoSphereBridge();

  // Disallow copy and assign
  EchoSphereBridge(const EchoSphereBridge&) = delete;
  EchoSphereBridge& operator=(const EchoSphereBridge&) = delete;

  // Initialize connection to EchoSphere AI-vCPU
  bool Initialize(const std::string& echosphere_endpoint = "");

  // Behavioral Orchestrator (EBO) - translates user context into AI requests
  void ProcessBehavioralOrchestration(const AIServiceProvider::AIRequestParams& params,
                                    const std::string& user_context,
                                    BehavioralCallback callback);

  // Enrichment Engine - adds context from EchoSphere caches
  void EnrichPrompt(const std::string& base_prompt,
                   const std::vector<ContextType>& context_types,
                   EnrichmentCallback callback);

  // Get context from specific EchoSphere components
  void GetHolographicMemory(const std::string& query,
                          base::OnceCallback<void(const std::string&)> callback);

  void GetL3CacheData(const std::string& key,
                     base::OnceCallback<void(const std::string&)> callback);

  void GetTemporalIntelligence(const std::string& context,
                             base::OnceCallback<void(const std::string&)> callback);

  // Update EchoSphere state with AI interaction results
  void UpdateInteractionResult(const std::string& request_id,
                             const std::string& response,
                             bool success);

  // Enable/disable specific EchoSphere features
  void EnableFeature(const std::string& feature_name, bool enabled);
  bool IsFeatureEnabled(const std::string& feature_name) const;

  // Get connection status
  bool IsConnected() const;
  std::string GetConnectionStatus() const;

  // Get a weak pointer to this instance
  base::WeakPtr<EchoSphereBridge> GetWeakPtr();

 private:
  // Helper methods for EchoSphere communication
  void SendEchoSphereRequest(const std::string& endpoint,
                           const std::string& payload,
                           base::OnceCallback<void(const std::string&)> callback);

  bool ProcessEchoSphereResponse(const std::string& response,
                               BehavioralDecision* decision);

  std::string BuildEnrichmentRequest(const std::string& prompt,
                                   const std::vector<ContextType>& contexts);

  // Connection management
  std::string echosphere_endpoint_;
  bool is_connected_ = false;
  std::unordered_map<std::string, bool> enabled_features_;

  // For weak pointers
  base::WeakPtrFactory<EchoSphereBridge> weak_ptr_factory_{this};
};

}  // namespace core
}  // namespace asol

#endif  // ASOL_CORE_ECHOSPHERE_BRIDGE_H_
