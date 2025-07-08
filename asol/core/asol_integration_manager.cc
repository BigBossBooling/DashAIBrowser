// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "asol/core/asol_integration_manager.h"

#include <utility>

#include "base/logging.h"

namespace asol {
namespace core {

ASOLIntegrationManager::ASOLIntegrationManager() = default;

ASOLIntegrationManager::~ASOLIntegrationManager() = default;

void ASOLIntegrationManager::Initialize(InitializationCallback callback) {
  LOG(INFO) << "Initializing comprehensive ASOL integration";

  // Initialize components in dependency order
  InitializeAPIGateway();
  InitializePrivacyProxy();
  InitializeEchoSphereBridge();
  InitializeWeb3Integration();
  InitializeSecurityManager();
  InitializePerformanceTracker();
  InitializeMultimodalProcessor();
  IntegrateWithServiceManager();

  bool success = AllComponentsReady();
  
  if (success) {
    LOG(INFO) << "ASOL comprehensive integration completed successfully";
  } else {
    LOG(ERROR) << "ASOL integration failed - some components not ready";
  }

  std::move(callback).Run(success);
}

ASOLIntegrationManager::IntegrationStatus ASOLIntegrationManager::GetIntegrationStatus() const {
  return integration_status_;
}

APIGateway* ASOLIntegrationManager::GetAPIGateway() const {
  return api_gateway_.get();
}

PrivacyProxy* ASOLIntegrationManager::GetPrivacyProxy() const {
  return privacy_proxy_.get();
}

EchoSphereBridge* ASOLIntegrationManager::GetEchoSphereBridge() const {
  return echosphere_bridge_.get();
}

Web3Integration* ASOLIntegrationManager::GetWeb3Integration() const {
  return web3_integration_.get();
}

EnhancedSecurityManager* ASOLIntegrationManager::GetEnhancedSecurityManager() const {
  return enhanced_security_manager_.get();
}

PerformanceTracker* ASOLIntegrationManager::GetPerformanceTracker() const {
  return performance_tracker_.get();
}

MultimodalProcessor* ASOLIntegrationManager::GetMultimodalProcessor() const {
  return multimodal_processor_.get();
}

ServiceManager* ASOLIntegrationManager::GetServiceManager() const {
  return service_manager_;
}

void ASOLIntegrationManager::ProcessEnhancedAIRequest(
    const AIServiceProvider::AIRequestParams& params,
    const std::string& user_context,
    AIServiceProvider::AIResponseCallback callback) {
  
  if (!comprehensive_mode_enabled_ || !service_manager_) {
    // Fallback to basic processing
    service_manager_->ProcessRequest(params, std::move(callback));
    return;
  }

  LOG(INFO) << "Processing enhanced AI request with full ASOL pipeline";

  // Step 1: Security assessment
  if (enhanced_security_manager_) {
    enhanced_security_manager_->DetectPromptThreats(
        params.input_text, user_context,
        base::BindOnce([](base::WeakPtr<ASOLIntegrationManager> self,
                         AIServiceProvider::AIRequestParams params,
                         std::string user_context,
                         AIServiceProvider::AIResponseCallback callback,
                         const EnhancedSecurityManager::SecurityAssessment& assessment) {
          if (!self || !assessment.allow_request) {
            LOG(WARNING) << "Request blocked by security assessment";
            std::move(callback).Run(false, "Request blocked for security reasons");
            return;
          }
          
          // Step 2: Privacy processing
          if (self->privacy_proxy_) {
            auto processed_result = self->privacy_proxy_->ProcessTextSync(params.input_text);
            params.input_text = processed_result.processed_text;
          }
          
          // Step 3: EchoSphere behavioral orchestration
          if (self->echosphere_bridge_) {
            self->echosphere_bridge_->ProcessBehavioralOrchestration(
                params, user_context,
                base::BindOnce([](base::WeakPtr<ASOLIntegrationManager> self,
                                 AIServiceProvider::AIRequestParams params,
                                 AIServiceProvider::AIResponseCallback callback,
                                 const EchoSphereBridge::BehavioralDecision& decision) {
                  if (!self || !self->service_manager_) {
                    std::move(callback).Run(false, "Integration manager unavailable");
                    return;
                  }
                  
                  // Step 4: Process with enhanced service manager
                  self->service_manager_->ProcessRequest(params, std::move(callback));
                }, self, params, std::move(callback)));
          } else {
            // Process without EchoSphere
            self->service_manager_->ProcessRequest(params, std::move(callback));
          }
        }, weak_ptr_factory_.GetWeakPtr(), params, user_context, std::move(callback)));
  } else {
    // Process without security assessment
    service_manager_->ProcessRequest(params, std::move(callback));
  }
}

void ASOLIntegrationManager::EnableComprehensiveMode(bool enabled) {
  comprehensive_mode_enabled_ = enabled;
  LOG(INFO) << "Comprehensive ASOL mode " << (enabled ? "enabled" : "disabled");
}

bool ASOLIntegrationManager::IsComprehensiveModeEnabled() const {
  return comprehensive_mode_enabled_;
}

base::WeakPtr<ASOLIntegrationManager> ASOLIntegrationManager::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void ASOLIntegrationManager::InitializeAPIGateway() {
  api_gateway_ = std::make_unique<APIGateway>();
  integration_status_.api_gateway_ready = api_gateway_->Initialize();
  LOG(INFO) << "API Gateway initialization: " 
            << (integration_status_.api_gateway_ready ? "SUCCESS" : "FAILED");
}

void ASOLIntegrationManager::InitializePrivacyProxy() {
  privacy_proxy_ = std::make_unique<PrivacyProxy>();
  integration_status_.privacy_proxy_ready = privacy_proxy_->Initialize();
  LOG(INFO) << "Privacy Proxy initialization: " 
            << (integration_status_.privacy_proxy_ready ? "SUCCESS" : "FAILED");
}

void ASOLIntegrationManager::InitializeEchoSphereBridge() {
  echosphere_bridge_ = std::make_unique<EchoSphereBridge>();
  integration_status_.echosphere_bridge_ready = echosphere_bridge_->Initialize();
  LOG(INFO) << "EchoSphere Bridge initialization: " 
            << (integration_status_.echosphere_bridge_ready ? "SUCCESS" : "FAILED");
}

void ASOLIntegrationManager::InitializeWeb3Integration() {
  web3_integration_ = std::make_unique<Web3Integration>();
  integration_status_.web3_integration_ready = web3_integration_->Initialize();
  LOG(INFO) << "Web3 Integration initialization: " 
            << (integration_status_.web3_integration_ready ? "SUCCESS" : "FAILED");
}

void ASOLIntegrationManager::InitializeSecurityManager() {
  enhanced_security_manager_ = std::make_unique<EnhancedSecurityManager>();
  integration_status_.security_manager_ready = enhanced_security_manager_->Initialize();
  LOG(INFO) << "Enhanced Security Manager initialization: " 
            << (integration_status_.security_manager_ready ? "SUCCESS" : "FAILED");
}

void ASOLIntegrationManager::InitializePerformanceTracker() {
  performance_tracker_ = std::make_unique<PerformanceTracker>();
  integration_status_.performance_tracker_ready = performance_tracker_->Initialize();
  LOG(INFO) << "Performance Tracker initialization: " 
            << (integration_status_.performance_tracker_ready ? "SUCCESS" : "FAILED");
}

void ASOLIntegrationManager::InitializeMultimodalProcessor() {
  multimodal_processor_ = std::make_unique<MultimodalProcessor>();
  integration_status_.multimodal_processor_ready = multimodal_processor_->Initialize();
  LOG(INFO) << "Multimodal Processor initialization: " 
            << (integration_status_.multimodal_processor_ready ? "SUCCESS" : "FAILED");
}

void ASOLIntegrationManager::IntegrateWithServiceManager() {
  service_manager_ = ServiceManager::GetInstance();
  if (service_manager_) {
    // Integrate all components with ServiceManager
    if (api_gateway_) {
      service_manager_->SetAPIGateway(std::move(api_gateway_));
    }
    if (privacy_proxy_) {
      service_manager_->SetPrivacyProxy(std::move(privacy_proxy_));
    }
    if (echosphere_bridge_) {
      service_manager_->SetEchoSphereBridge(std::move(echosphere_bridge_));
    }
    if (web3_integration_) {
      service_manager_->SetWeb3Integration(std::move(web3_integration_));
    }
    if (enhanced_security_manager_) {
      service_manager_->SetEnhancedSecurityManager(std::move(enhanced_security_manager_));
    }
    if (performance_tracker_) {
      service_manager_->SetPerformanceTracker(std::move(performance_tracker_));
    }
    if (multimodal_processor_) {
      service_manager_->SetMultimodalProcessor(std::move(multimodal_processor_));
    }
    
    integration_status_.service_manager_ready = true;
    LOG(INFO) << "All components integrated with ServiceManager";
  } else {
    integration_status_.service_manager_ready = false;
    LOG(ERROR) << "Failed to get ServiceManager instance";
  }
}

bool ASOLIntegrationManager::AllComponentsReady() const {
  return integration_status_.api_gateway_ready &&
         integration_status_.privacy_proxy_ready &&
         integration_status_.echosphere_bridge_ready &&
         integration_status_.web3_integration_ready &&
         integration_status_.security_manager_ready &&
         integration_status_.performance_tracker_ready &&
         integration_status_.multimodal_processor_ready &&
         integration_status_.service_manager_ready;
}

}  // namespace core
}  // namespace asol
