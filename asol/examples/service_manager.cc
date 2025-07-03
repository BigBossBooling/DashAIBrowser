// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "asol/core/service_manager.h"

#include <algorithm>
#include <string>
#include <vector>

#include "base/logging.h"
#include "base/strings/string_util.h"
#include "third_party/nlohmann_json/json.hpp"

namespace asol {
namespace core {

// Initialize static instance
ServiceManager* ServiceManager::instance_ = nullptr;

ServiceManager* ServiceManager::GetInstance() {
  if (!instance_) {
    instance_ = new ServiceManager();
  }
  return instance_;
}

ServiceManager::ServiceManager() {
  LOG(INFO) << "ASOL ServiceManager created.";
  
  // Enable response caching by default
  EnableResponseCache(true);
}

ServiceManager::~ServiceManager() {
  LOG(INFO) << "ASOL ServiceManager destroyed.";
}

bool ServiceManager::RegisterAdapter(
    const std::string& adapter_id,
    std::unique_ptr<adapters::AdapterInterface> adapter) {
  if (!adapter) {
    LOG(ERROR) << "Attempted to register null adapter with ID: " << adapter_id;
    return false;
  }

  if (adapters_.find(adapter_id) != adapters_.end()) {
    LOG(WARNING) << "Adapter with ID '" << adapter_id << "' already registered. Replacing.";
  }

  LOG(INFO) << "Registering adapter: " << adapter_id << " (" << adapter->GetName() << ")";
  adapters_[adapter_id] = std::move(adapter);
  return true;
}

adapters::AdapterInterface* ServiceManager::GetAdapter(const std::string& adapter_id) {
  auto it = adapters_.find(adapter_id);
  if (it == adapters_.end()) {
    LOG(ERROR) << "Adapter not found: " << adapter_id;
    return nullptr;
  }
  return it->second.get();
}

std::vector<std::string> ServiceManager::FindAdaptersByCapability(
    const std::string& capability) {
  std::vector<std::string> matching_adapters;

  for (const auto& adapter_pair : adapters_) {
    const auto& capabilities = adapter_pair.second->GetCapabilities();
    if (std::find(capabilities.begin(), capabilities.end(), capability) != capabilities.end()) {
      matching_adapters.push_back(adapter_pair.first);
    }
  }

  return matching_adapters;
}

adapters::ModelResponse ServiceManager::ProcessText(
    const std::string& adapter_id,
    const std::string& text_input) {
  // Track performance of this operation
  util::ScopedPerformanceTracker tracker("ServiceManager_ProcessText");
  
  // Check if we have a cached response
  if (response_cache_) {
    const auto* cached_entry = response_cache_->Get(text_input, adapter_id, "");
    if (cached_entry) {
      return cached_entry->response;
    }
  }
  
  adapters::AdapterInterface* adapter = GetAdapter(adapter_id);
  if (!adapter) {
    adapters::ModelResponse response;
    response.success = false;
    response.error_message = "Adapter not found: " + adapter_id;
    return response;
  }

  // Get the response from the adapter
  adapters::ModelResponse response = adapter->ProcessText(text_input);
  
  // Cache the successful response
  if (response.success && response_cache_) {
    response_cache_->Put(text_input, response, adapter_id, "");
  }
  
  return response;
}

void ServiceManager::ProcessTextAsync(
    const std::string& adapter_id,
    const std::string& text_input,
    adapters::ResponseCallback callback) {
  adapters::AdapterInterface* adapter = GetAdapter(adapter_id);
  if (!adapter) {
    adapters::ModelResponse response;
    response.success = false;
    response.error_message = "Adapter not found: " + adapter_id;
    callback(response);
    return;
  }

  adapter->ProcessTextAsync(text_input, callback);
}

void ServiceManager::ProcessTextStream(
    const std::string& adapter_id,
    const std::string& text_input,
    adapters::StreamingResponseCallback callback) {
  adapters::AdapterInterface* adapter = GetAdapter(adapter_id);
  if (!adapter) {
    adapters::ModelResponse response;
    response.success = false;
    response.error_message = "Adapter not found: " + adapter_id;
    callback(response, true);  // true indicates this is the final response
    return;
  }

  if (!adapter->SupportsStreaming()) {
    adapters::ModelResponse response;
    response.success = false;
    response.error_message = "Adapter does not support streaming: " + adapter_id;
    callback(response, true);
    return;
  }

  adapter->ProcessTextStream(text_input, callback);
}

adapters::ModelResponse ServiceManager::ProcessTextWithCapability(
    const std::string& capability,
    const std::string& text_input) {
  std::string adapter_id = FindBestAdapter(capability);
  if (adapter_id.empty()) {
    adapters::ModelResponse response;
    response.success = false;
    response.error_message = "No adapter found with capability: " + capability;
    return response;
  }

  return ProcessText(adapter_id, text_input);
}

void ServiceManager::ProcessTextWithCapabilityAsync(
    const std::string& capability,
    const std::string& text_input,
    adapters::ResponseCallback callback) {
  std::string adapter_id = FindBestAdapter(capability);
  if (adapter_id.empty()) {
    adapters::ModelResponse response;
    response.success = false;
    response.error_message = "No adapter found with capability: " + capability;
    callback(response);
    return;
  }

  ProcessTextAsync(adapter_id, text_input, callback);
}

void ServiceManager::ProcessTextWithCapabilityStream(
    const std::string& capability,
    const std::string& text_input,
    adapters::StreamingResponseCallback callback) {
  std::string adapter_id = FindBestAdapter(capability);
  if (adapter_id.empty()) {
    adapters::ModelResponse response;
    response.success = false;
    response.error_message = "No adapter found with capability: " + capability;
    callback(response, true);
    return;
  }

  adapters::AdapterInterface* adapter = GetAdapter(adapter_id);
  if (!adapter->SupportsStreaming()) {
    // Fall back to non-streaming if the adapter doesn't support it
    ProcessTextAsync(adapter_id, text_input,
                    [callback](const adapters::ModelResponse& response) {
                      callback(response, true);
                    });
    return;
  }

  ProcessTextStream(adapter_id, text_input, callback);
}

bool ServiceManager::InitializeAdapters(const std::string& config_json) {
  try {
    auto config = nlohmann::json::parse(config_json);
    
    if (!config.contains("adapters") || !config["adapters"].is_object()) {
      LOG(ERROR) << "Invalid configuration: missing or invalid 'adapters' object";
      return false;
    }
    
    bool all_success = true;
    
    // Initialize each adapter with its configuration
    for (auto& [adapter_id, adapter_config] : config["adapters"].items()) {
      adapters::AdapterInterface* adapter = GetAdapter(adapter_id);
      if (!adapter) {
        LOG(WARNING) << "Configuration provided for unknown adapter: " << adapter_id;
        continue;
      }
      
      std::string adapter_config_str = adapter_config.dump();
      if (!adapter->Initialize(adapter_config_str)) {
        LOG(ERROR) << "Failed to initialize adapter: " << adapter_id;
        all_success = false;
      } else {
        LOG(INFO) << "Successfully initialized adapter: " << adapter_id;
      }
    }
    
    return all_success;
  } catch (const std::exception& e) {
    LOG(ERROR) << "Failed to parse configuration: " << e.what();
    return false;
  }
}

std::string ServiceManager::FindBestAdapter(const std::string& capability) {
  std::vector<std::string> matching_adapters = FindAdaptersByCapability(capability);
  
  if (matching_adapters.empty()) {
    return "";
  }
  
  // For now, just return the first matching adapter
  // In a more sophisticated implementation, this could consider:
  // - Adapter performance metrics
  // - Load balancing
  // - User preferences
  // - Specific model capabilities
  return matching_adapters[0];
}

std::vector<std::string> ServiceManager::GetRegisteredAdapters() const {
  std::vector<std::string> adapter_ids;
  adapter_ids.reserve(adapters_.size());
  
  for (const auto& adapter_pair : adapters_) {
    adapter_ids.push_back(adapter_pair.first);
  }
  
  return adapter_ids;
}

std::vector<std::string> ServiceManager::GetAvailableCapabilities() const {
  std::vector<std::string> all_capabilities;
  
  for (const auto& adapter_pair : adapters_) {
    const auto& capabilities = adapter_pair.second->GetCapabilities();
    all_capabilities.insert(all_capabilities.end(), 
                           capabilities.begin(), capabilities.end());
  }
  
  // Remove duplicates
  std::sort(all_capabilities.begin(), all_capabilities.end());
  all_capabilities.erase(
      std::unique(all_capabilities.begin(), all_capabilities.end()),
      all_capabilities.end());
  
  return all_capabilities;
}

bool ServiceManager::AdapterSupportsStreaming(const std::string& adapter_id) const {
  auto it = adapters_.find(adapter_id);
  if (it == adapters_.end()) {
    return false;
  }
  
  return it->second->SupportsStreaming();
}

void ServiceManager::EnableResponseCache(bool enable, size_t capacity) {
  if (enable) {
    if (!response_cache_) {
      response_cache_ = std::make_unique<util::ResponseCache>(capacity);
      LOG(INFO) << "Response cache enabled with capacity " << capacity;
    } else {
      response_cache_->SetCapacity(capacity);
      LOG(INFO) << "Response cache capacity updated to " << capacity;
    }
  } else {
    if (response_cache_) {
      response_cache_.reset();
      LOG(INFO) << "Response cache disabled";
    }
  }
}

void ServiceManager::ClearResponseCache() {
  if (response_cache_) {
    response_cache_->Clear();
    LOG(INFO) << "Response cache cleared";
  }
}

}  // namespace core
}  // namespace asol