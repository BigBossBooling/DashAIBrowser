// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASOL_CORE_MULTI_ADAPTER_MANAGER_H_
#define ASOL_CORE_MULTI_ADAPTER_MANAGER_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <utility>

#include "asol/core/ai_service_provider.h"
#include "base/memory/weak_ptr.h"

namespace asol {
namespace core {

// MultiAdapterManager manages multiple AI service providers and allows
// switching between them based on user preferences or task requirements.
class MultiAdapterManager {
 public:
  // Cache entry for storing AI responses
  struct CacheEntry {
    std::string response;
    std::chrono::time_point<std::chrono::steady_clock> timestamp;
    std::string provider_id;
  };

  // Cache configuration
  struct CacheConfig {
    // Maximum number of entries in the cache
    size_t max_entries = 100;
    
    // Maximum age of cache entries in seconds
    int max_age_seconds = 3600;  // 1 hour by default
    
    // Whether to enable caching
    bool enabled = true;
  };

  MultiAdapterManager();
  ~MultiAdapterManager();

  // Disallow copy and assign
  MultiAdapterManager(const MultiAdapterManager&) = delete;
  MultiAdapterManager& operator=(const MultiAdapterManager&) = delete;

  // Register a new AI service provider
  void RegisterProvider(std::unique_ptr<AIServiceProvider> provider);

  // Get a list of all registered provider IDs
  std::vector<std::string> GetRegisteredProviderIds() const;

  // Get a list of all registered provider names
  std::vector<std::string> GetRegisteredProviderNames() const;

  // Set the active provider by ID
  bool SetActiveProvider(const std::string& provider_id);

  // Get the currently active provider
  AIServiceProvider* GetActiveProvider() const;

  // Get a specific provider by ID
  AIServiceProvider* GetProvider(const std::string& provider_id) const;

  // Get the ID of the currently active provider
  std::string GetActiveProviderId() const;

  // Process a request using the active provider
  void ProcessRequest(const AIRequestParams& params, AIResponseCallback callback);

  // Process a request using a specific provider
  void ProcessRequestWithProvider(const std::string& provider_id,
                                const AIRequestParams& params,
                                AIResponseCallback callback);

  // Configure a specific provider
  bool ConfigureProvider(const std::string& provider_id,
                       const std::unordered_map<std::string, std::string>& config);

  // Get the configuration for a specific provider
  std::unordered_map<std::string, std::string> GetProviderConfiguration(
      const std::string& provider_id) const;

  // Find the best provider for a specific task type
  std::string FindBestProviderForTask(AIServiceProvider::TaskType task_type) const;

  // Configure the response cache
  void ConfigureCache(const CacheConfig& config);

  // Clear the response cache
  void ClearCache();

  // Get cache statistics
  struct CacheStats {
    size_t total_entries;
    size_t hits;
    size_t misses;
    double hit_rate;
  };
  CacheStats GetCacheStats() const;

 private:
  // Generate a cache key for a request
  std::string GenerateCacheKey(const AIRequestParams& params) const;
  
  // Check if a response is in the cache
  bool CheckCache(const std::string& cache_key, std::string* response) const;
  
  // Add a response to the cache
  void AddToCache(const std::string& cache_key, 
                 const std::string& response,
                 const std::string& provider_id);
  
  // Clean expired entries from the cache
  void CleanCache();

  // Map of provider ID to provider instance
  std::unordered_map<std::string, std::unique_ptr<AIServiceProvider>> providers_;
  
  // Currently active provider ID
  std::string active_provider_id_;
  
  // Response cache
  std::unordered_map<std::string, CacheEntry> response_cache_;
  
  // Cache configuration
  CacheConfig cache_config_;
  
  // Cache statistics
  size_t cache_hits_ = 0;
  size_t cache_misses_ = 0;
  
  // For weak pointers
  base::WeakPtrFactory<MultiAdapterManager> weak_ptr_factory_{this};
};

}  // namespace core
}  // namespace asol

#endif  // ASOL_CORE_MULTI_ADAPTER_MANAGER_H_