// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASOL_CORE_ASOL_INTEGRATION_MANAGER_H_
#define ASOL_CORE_ASOL_INTEGRATION_MANAGER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "asol/core/api_gateway.h"
#include "asol/core/echosphere_bridge.h"
#include "asol/core/enhanced_security_manager.h"
#include "asol/core/performance_tracker.h"
#include "asol/core/privacy_proxy.h"
#include "asol/core/service_manager.h"
#include "asol/core/web3_integration.h"
#include "asol/core/multimodal_processor.h"

namespace asol {
namespace core {

// ASOL Integration Manager coordinates all enhanced ASOL components
// and provides a unified interface for the comprehensive AI-native browser
class ASOLIntegrationManager {
 public:
  // Integration status
  struct IntegrationStatus {
    bool api_gateway_ready = false;
    bool privacy_proxy_ready = false;
    bool echosphere_bridge_ready = false;
    bool web3_integration_ready = false;
    bool security_manager_ready = false;
    bool performance_tracker_ready = false;
    bool multimodal_processor_ready = false;
    bool service_manager_ready = false;
  };

  using InitializationCallback = base::OnceCallback<void(bool success)>;

  ASOLIntegrationManager();
  ~ASOLIntegrationManager();

  // Disallow copy and assign
  ASOLIntegrationManager(const ASOLIntegrationManager&) = delete;
  ASOLIntegrationManager& operator=(const ASOLIntegrationManager&) = delete;

  // Initialize all ASOL components in the correct order
  void Initialize(InitializationCallback callback);

  // Get integration status
  IntegrationStatus GetIntegrationStatus() const;

  // Access to individual components
  APIGateway* GetAPIGateway() const;
  PrivacyProxy* GetPrivacyProxy() const;
  EchoSphereBridge* GetEchoSphereBridge() const;
  Web3Integration* GetWeb3Integration() const;
  EnhancedSecurityManager* GetEnhancedSecurityManager() const;
  PerformanceTracker* GetPerformanceTracker() const;
  MultimodalProcessor* GetMultimodalProcessor() const;
  ServiceManager* GetServiceManager() const;

  // Comprehensive AI request processing with all ASOL features
  void ProcessEnhancedAIRequest(const AIServiceProvider::AIRequestParams& params,
                              const std::string& user_context,
                              AIServiceProvider::AIResponseCallback callback);

  // Enable/disable comprehensive features
  void EnableComprehensiveMode(bool enabled);
  bool IsComprehensiveModeEnabled() const;

  // Get a weak pointer to this instance
  base::WeakPtr<ASOLIntegrationManager> GetWeakPtr();

 private:
  // Initialization helpers
  void InitializeAPIGateway();
  void InitializePrivacyProxy();
  void InitializeEchoSphereBridge();
  void InitializeWeb3Integration();
  void InitializeSecurityManager();
  void InitializePerformanceTracker();
  void InitializeMultimodalProcessor();
  void IntegrateWithServiceManager();

  // Check if all components are ready
  bool AllComponentsReady() const;

  // Component instances
  std::unique_ptr<APIGateway> api_gateway_;
  std::unique_ptr<PrivacyProxy> privacy_proxy_;
  std::unique_ptr<EchoSphereBridge> echosphere_bridge_;
  std::unique_ptr<Web3Integration> web3_integration_;
  std::unique_ptr<EnhancedSecurityManager> enhanced_security_manager_;
  std::unique_ptr<PerformanceTracker> performance_tracker_;
  std::unique_ptr<MultimodalProcessor> multimodal_processor_;

  // Service manager reference (singleton)
  ServiceManager* service_manager_ = nullptr;

  // State
  bool comprehensive_mode_enabled_ = true;
  IntegrationStatus integration_status_;

  // For weak pointers
  base::WeakPtrFactory<ASOLIntegrationManager> weak_ptr_factory_{this};
};

}  // namespace core
}  // namespace asol

#endif  // ASOL_CORE_ASOL_INTEGRATION_MANAGER_H_
