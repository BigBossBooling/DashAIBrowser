// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "asol/core/echosphere_bridge.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/values.h"

namespace asol {
namespace core {

EchoSphereBridge::EchoSphereBridge() {
  // Initialize default enabled features
  enabled_features_["behavioral_orchestration"] = true;
  enabled_features_["enrichment_engine"] = true;
  enabled_features_["holographic_memory"] = true;
  enabled_features_["temporal_intelligence"] = true;
  enabled_features_["security_guardian"] = true;
}

EchoSphereBridge::~EchoSphereBridge() = default;

bool EchoSphereBridge::Initialize(const std::string& echosphere_endpoint) {
  echosphere_endpoint_ = echosphere_endpoint.empty() ? 
      "http://localhost:8080/echosphere" : echosphere_endpoint;
  
  LOG(INFO) << "Initializing EchoSphere bridge with endpoint: " << echosphere_endpoint_;
  
  // For now, simulate connection - in real implementation would test connectivity
  is_connected_ = true;
  
  return is_connected_;
}

void EchoSphereBridge::ProcessBehavioralOrchestration(
    const AIServiceProvider::AIRequestParams& params,
    const std::string& user_context,
    BehavioralCallback callback) {
  
  if (!IsFeatureEnabled("behavioral_orchestration")) {
    BehavioralDecision decision;
    decision.action_request = "direct_processing";
    decision.interaction_goal = "basic_response";
    decision.confidence_score = 0.5;
    std::move(callback).Run(decision);
    return;
  }

  // Build request for EchoSphere Behavioral Orchestrator
  base::Value::Dict request;
  request.Set("type", "behavioral_orchestration");
  request.Set("task_type", static_cast<int>(params.task_type));
  request.Set("input_text", params.input_text);
  request.Set("context_id", params.context_id);
  request.Set("user_context", user_context);

  std::string request_json;
  base::JSONWriter::Write(request, &request_json);

  SendEchoSphereRequest("/behavioral_orchestrator", request_json,
    base::BindOnce([](BehavioralCallback callback, const std::string& response) {
      BehavioralDecision decision;
      
      auto parsed_response = base::JSONReader::Read(response);
      if (parsed_response && parsed_response->is_dict()) {
        const base::Value::Dict& dict = parsed_response->GetDict();
        
        if (const std::string* action = dict.FindString("action_request")) {
          decision.action_request = *action;
        }
        if (const std::string* goal = dict.FindString("interaction_goal")) {
          decision.interaction_goal = *goal;
        }
        if (auto confidence = dict.FindDouble("confidence_score")) {
          decision.confidence_score = *confidence;
        }
        
        // Parse context modifiers
        if (const base::Value::Dict* modifiers = dict.FindDict("context_modifiers")) {
          for (const auto& [key, value] : *modifiers) {
            if (value.is_string()) {
              decision.context_modifiers[key] = value.GetString();
            }
          }
        }
      } else {
        // Fallback decision
        decision.action_request = "process_with_context";
        decision.interaction_goal = "contextual_response";
        decision.confidence_score = 0.7;
      }
      
      std::move(callback).Run(decision);
    }, std::move(callback)));
}

void EchoSphereBridge::EnrichPrompt(const std::string& base_prompt,
                                  const std::vector<ContextType>& context_types,
                                  EnrichmentCallback callback) {
  
  if (!IsFeatureEnabled("enrichment_engine")) {
    EnrichmentResult result;
    result.enriched_prompt = base_prompt;
    std::move(callback).Run(result);
    return;
  }

  std::string enrichment_request = BuildEnrichmentRequest(base_prompt, context_types);
  
  SendEchoSphereRequest("/enrichment_engine", enrichment_request,
    base::BindOnce([](const std::string& base_prompt, EnrichmentCallback callback, 
                     const std::string& response) {
      EnrichmentResult result;
      
      auto parsed_response = base::JSONReader::Read(response);
      if (parsed_response && parsed_response->is_dict()) {
        const base::Value::Dict& dict = parsed_response->GetDict();
        
        if (const std::string* enriched = dict.FindString("enriched_prompt")) {
          result.enriched_prompt = *enriched;
        } else {
          result.enriched_prompt = base_prompt;
        }
        
        if (const base::Value::List* contexts = dict.FindList("relevant_contexts")) {
          for (const auto& context : *contexts) {
            if (context.is_string()) {
              result.relevant_contexts.push_back(context.GetString());
            }
          }
        }
        
        if (const base::Value::Dict* persona = dict.FindDict("persona_details")) {
          for (const auto& [key, value] : *persona) {
            if (value.is_string()) {
              result.persona_details[key] = value.GetString();
            }
          }
        }
        
        if (const std::string* cache_data = dict.FindString("knowledge_cache_data")) {
          result.knowledge_cache_data = *cache_data;
        }
      } else {
        result.enriched_prompt = base_prompt;
      }
      
      std::move(callback).Run(result);
    }, base_prompt, std::move(callback)));
}

void EchoSphereBridge::GetHolographicMemory(const std::string& query,
                                          base::OnceCallback<void(const std::string&)> callback) {
  if (!IsFeatureEnabled("holographic_memory")) {
    std::move(callback).Run("");
    return;
  }

  base::Value::Dict request;
  request.Set("type", "holographic_memory_query");
  request.Set("query", query);

  std::string request_json;
  base::JSONWriter::Write(request, &request_json);

  SendEchoSphereRequest("/holographic_memory", request_json, std::move(callback));
}

void EchoSphereBridge::GetL3CacheData(const std::string& key,
                                    base::OnceCallback<void(const std::string&)> callback) {
  base::Value::Dict request;
  request.Set("type", "l3_cache_query");
  request.Set("key", key);

  std::string request_json;
  base::JSONWriter::Write(request, &request_json);

  SendEchoSphereRequest("/l3_cache", request_json, std::move(callback));
}

void EchoSphereBridge::GetTemporalIntelligence(const std::string& context,
                                             base::OnceCallback<void(const std::string&)> callback) {
  if (!IsFeatureEnabled("temporal_intelligence")) {
    std::move(callback).Run("");
    return;
  }

  base::Value::Dict request;
  request.Set("type", "temporal_intelligence_query");
  request.Set("context", context);

  std::string request_json;
  base::JSONWriter::Write(request, &request_json);

  SendEchoSphereRequest("/temporal_intelligence", request_json, std::move(callback));
}

void EchoSphereBridge::UpdateInteractionResult(const std::string& request_id,
                                             const std::string& response,
                                             bool success) {
  base::Value::Dict update;
  update.Set("type", "interaction_result");
  update.Set("request_id", request_id);
  update.Set("response", response);
  update.Set("success", success);

  std::string update_json;
  base::JSONWriter::Write(update, &update_json);

  SendEchoSphereRequest("/update_result", update_json,
    base::BindOnce([](const std::string& response) {
      LOG(INFO) << "EchoSphere interaction result updated";
    }));
}

void EchoSphereBridge::EnableFeature(const std::string& feature_name, bool enabled) {
  enabled_features_[feature_name] = enabled;
  LOG(INFO) << "EchoSphere feature " << feature_name << " " 
            << (enabled ? "enabled" : "disabled");
}

bool EchoSphereBridge::IsFeatureEnabled(const std::string& feature_name) const {
  auto it = enabled_features_.find(feature_name);
  return it != enabled_features_.end() && it->second;
}

bool EchoSphereBridge::IsConnected() const {
  return is_connected_;
}

std::string EchoSphereBridge::GetConnectionStatus() const {
  if (is_connected_) {
    return "Connected to " + echosphere_endpoint_;
  }
  return "Disconnected";
}

base::WeakPtr<EchoSphereBridge> EchoSphereBridge::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void EchoSphereBridge::SendEchoSphereRequest(const std::string& endpoint,
                                           const std::string& payload,
                                           base::OnceCallback<void(const std::string&)> callback) {
  // In a real implementation, this would make HTTP requests to EchoSphere
  // For now, simulate responses based on the endpoint
  std::string simulated_response;
  
  if (endpoint == "/behavioral_orchestrator") {
    simulated_response = R"({
      "action_request": "intelligent_processing",
      "interaction_goal": "contextual_assistance",
      "confidence_score": 0.85,
      "context_modifiers": {
        "priority": "high",
        "style": "conversational"
      }
    })";
  } else if (endpoint == "/enrichment_engine") {
    simulated_response = R"({
      "enriched_prompt": "Enhanced prompt with contextual intelligence",
      "relevant_contexts": ["user_history", "domain_knowledge"],
      "persona_details": {
        "communication_style": "professional",
        "expertise_level": "intermediate"
      },
      "knowledge_cache_data": "Relevant cached knowledge"
    })";
  } else {
    simulated_response = R"({"status": "success", "data": "simulated_data"})";
  }
  
  std::move(callback).Run(simulated_response);
}

std::string EchoSphereBridge::BuildEnrichmentRequest(const std::string& prompt,
                                                   const std::vector<ContextType>& contexts) {
  base::Value::Dict request;
  request.Set("type", "enrichment_request");
  request.Set("base_prompt", prompt);
  
  base::Value::List context_list;
  for (ContextType context : contexts) {
    switch (context) {
      case ContextType::HOLOGRAPHIC_MEMORY:
        context_list.Append("holographic_memory");
        break;
      case ContextType::L3_CACHE:
        context_list.Append("l3_cache");
        break;
      case ContextType::TEMPORAL_INTELLIGENCE:
        context_list.Append("temporal_intelligence");
        break;
      case ContextType::CREATIVE_GENERATOR:
        context_list.Append("creative_generator");
        break;
      case ContextType::SECURITY_GUARDIAN:
        context_list.Append("security_guardian");
        break;
      case ContextType::FUSION_CORE:
        context_list.Append("fusion_core");
        break;
    }
  }
  request.Set("context_types", std::move(context_list));
  
  std::string request_json;
  base::JSONWriter::Write(request, &request_json);
  return request_json;
}

}  // namespace core
}  // namespace asol
