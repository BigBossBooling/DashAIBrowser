// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASOL_CORE_SERVICE_MANAGER_H_
#define ASOL_CORE_SERVICE_MANAGER_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "asol/adapters/adapter_interface.h"
#include "asol/util/performance_tracker.h"
#include "asol/util/response_cache.h"

namespace asol {
namespace core {

// ServiceManager is the central component that manages AI service adapters
// and routes requests to the appropriate adapter based on capabilities.
class ServiceManager {
 public:
  // Get the singleton instance
  static ServiceManager* GetInstance();

  // Register an adapter with the service manager
  bool RegisterAdapter(const std::string& adapter_id,
                      std::unique_ptr<adapters::AdapterInterface> adapter);

  // Get an adapter by ID
  adapters::AdapterInterface* GetAdapter(const std::string& adapter_id);

  // Find adapters that support a specific capability
  std::vector<std::string> FindAdaptersByCapability(const std::string& capability);

  // Process text with the specified adapter
  adapters::ModelResponse ProcessText(const std::string& adapter_id,
                                     const std::string& text_input);

  // Process text asynchronously with the specified adapter
  void ProcessTextAsync(const std::string& adapter_id,
                       const std::string& text_input,
                       adapters::ResponseCallback callback);
                       
  // Process text with streaming response from the specified adapter
  void ProcessTextStream(const std::string& adapter_id,
                        const std::string& text_input,
                        adapters::StreamingResponseCallback callback);

  // Process text with the best available adapter for the given capability
  adapters::ModelResponse ProcessTextWithCapability(
      const std::string& capability,
      const std::string& text_input);
      
  // Process text asynchronously with the best available adapter for the given capability
  void ProcessTextWithCapabilityAsync(
      const std::string& capability,
      const std::string& text_input,
      adapters::ResponseCallback callback);
      
  // Process text with streaming response from the best available adapter for the given capability
  void ProcessTextWithCapabilityStream(
      const std::string& capability,
      const std::string& text_input,
      adapters::StreamingResponseCallback callback);

  // Initialize all registered adapters with their respective configurations
  bool InitializeAdapters(const std::string& config_json);
  
  // Get a list of registered adapters
  std::vector<std::string> GetRegisteredAdapters() const;
  
  // Get a list of available capabilities across all adapters
  std::vector<std::string> GetAvailableCapabilities() const;
  
  // Check if an adapter supports streaming
  bool AdapterSupportsStreaming(const std::string& adapter_id) const;
  
  // Enable or disable response caching
  void EnableResponseCache(bool enable, size_t capacity = 100);
  
  // Clear the response cache
  void ClearResponseCache();
  
  // Get adapter performance metrics for intelligent routing
  std::vector<std::pair<std::string, double>> GetAdapterPerformanceMetrics() const;
  
  // Enhanced ASOL integration methods
  void SetAPIGateway(std::unique_ptr<APIGateway> gateway);
  void SetPrivacyProxy(std::unique_ptr<PrivacyProxy> proxy);
  void SetEchoSphereBridge(std::unique_ptr<EchoSphereBridge> bridge);
  void SetWeb3Integration(std::unique_ptr<Web3Integration> web3);
  void SetEnhancedSecurityManager(std::unique_ptr<EnhancedSecurityManager> security);
  void SetPerformanceTracker(std::unique_ptr<PerformanceTracker> tracker);
  void SetMultimodalProcessor(std::unique_ptr<MultimodalProcessor> processor);

 private:
  ServiceManager();
  ~ServiceManager();

  // Find the best adapter for a given capability
  std::string FindBestAdapter(const std::string& capability);

  // Map of adapter ID to adapter instance
  std::unordered_map<std::string, std::unique_ptr<adapters::AdapterInterface>> adapters_;

  // Response cache for improved performance
  std::unique_ptr<util::ResponseCache> response_cache_;
  
  // Enhanced ASOL components
  std::unique_ptr<APIGateway> api_gateway_;
  std::unique_ptr<PrivacyProxy> privacy_proxy_;
  std::unique_ptr<EchoSphereBridge> echosphere_bridge_;
  std::unique_ptr<Web3Integration> web3_integration_;
  std::unique_ptr<EnhancedSecurityManager> enhanced_security_manager_;
  std::unique_ptr<PerformanceTracker> performance_tracker_;
  std::unique_ptr<MultimodalProcessor> multimodal_processor_;

  // Singleton instance
  static ServiceManager* instance_;
};

}  // namespace core
}  // namespace asol

#endif  // ASOL_CORE_SERVICE_MANAGER_H_
