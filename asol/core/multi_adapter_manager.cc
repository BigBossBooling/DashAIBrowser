// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "asol/core/multi_adapter_manager.h"

#include <utility>
#include <functional>
#include <sstream>
#include <algorithm>

#include "base/logging.h"

namespace asol {
namespace core {

MultiAdapterManager::MultiAdapterManager() {
  LOG(INFO) << "MultiAdapterManager initialized.";
}

MultiAdapterManager::~MultiAdapterManager() {
  LOG(INFO) << "MultiAdapterManager destroyed.";
}

void MultiAdapterManager::RegisterProvider(std::unique_ptr<AIServiceProvider> provider) {
  if (!provider) {
    LOG(ERROR) << "Attempted to register a null provider.";
    return;
  }
  
  std::string provider_id = provider->GetProviderId();
  LOG(INFO) << "Registering AI provider: " << provider_id;
  
  // Check if this is the first provider being registered
  bool is_first_provider = providers_.empty();
  
  // Add the provider to our map
  providers_[provider_id] = std::move(provider);
  
  // If this is the first provider, make it the active one
  if (is_first_provider) {
    active_provider_id_ = provider_id;
    LOG(INFO) << "Set " << provider_id << " as the active provider.";
  }
}

std::vector<std::string> MultiAdapterManager::GetRegisteredProviderIds() const {
  std::vector<std::string> provider_ids;
  provider_ids.reserve(providers_.size());
  
  for (const auto& [id, _] : providers_) {
    provider_ids.push_back(id);
  }
  
  return provider_ids;
}

std::vector<std::string> MultiAdapterManager::GetRegisteredProviderNames() const {
  std::vector<std::string> provider_names;
  provider_names.reserve(providers_.size());
  
  for (const auto& [_, provider] : providers_) {
    provider_names.push_back(provider->GetProviderName());
  }
  
  return provider_names;
}

bool MultiAdapterManager::SetActiveProvider(const std::string& provider_id) {
  if (providers_.find(provider_id) == providers_.end()) {
    LOG(ERROR) << "Attempted to set unknown provider as active: " << provider_id;
    return false;
  }
  
  active_provider_id_ = provider_id;
  LOG(INFO) << "Set " << provider_id << " as the active provider.";
  return true;
}

AIServiceProvider* MultiAdapterManager::GetActiveProvider() const {
  if (active_provider_id_.empty() || providers_.find(active_provider_id_) == providers_.end()) {
    LOG(ERROR) << "No active provider set or active provider not found.";
    return nullptr;
  }
  
  return providers_.at(active_provider_id_).get();
}

AIServiceProvider* MultiAdapterManager::GetProvider(const std::string& provider_id) const {
  if (providers_.find(provider_id) == providers_.end()) {
    LOG(ERROR) << "Provider not found: " << provider_id;
    return nullptr;
  }
  
  return providers_.at(provider_id).get();
}

std::string MultiAdapterManager::GetActiveProviderId() const {
  return active_provider_id_;
}

void MultiAdapterManager::ProcessRequest(
    const AIRequestParams& params,
    AIResponseCallback callback) {
  // Check if the response is in the cache
  if (cache_config_.enabled) {
    std::string cache_key = GenerateCacheKey(params);
    std::string cached_response;
    
    if (CheckCache(cache_key, &cached_response)) {
      LOG(INFO) << "Cache hit for request: " << cache_key;
      std::move(callback).Run(true, cached_response);
      return;
    }
  }
  
  AIServiceProvider* active_provider = GetActiveProvider();
  if (!active_provider) {
    std::move(callback).Run(false, "No active AI provider available.");
    return;
  }
  
  // Check if the active provider supports this task type
  if (!active_provider->SupportsTaskType(params.task_type)) {
    // Try to find a provider that supports this task type
    std::string best_provider_id = FindBestProviderForTask(params.task_type);
    if (!best_provider_id.empty() && best_provider_id != active_provider_id_) {
      LOG(INFO) << "Active provider doesn't support task type " 
                << static_cast<int>(params.task_type) 
                << ", switching to " << best_provider_id;
      ProcessRequestWithProvider(best_provider_id, params, std::move(callback));
      return;
    }
    
    // No suitable provider found
    std::move(callback).Run(false, "Active provider doesn't support this task type.");
    return;
  }
  
  // Process the request with the active provider
  if (cache_config_.enabled) {
    std::string cache_key = GenerateCacheKey(params);
    active_provider->ProcessRequest(
        params,
        base::BindOnce(
            [](base::WeakPtr<MultiAdapterManager> self,
               std::string cache_key,
               std::string provider_id,
               AIResponseCallback callback,
               bool success,
               const std::string& response) {
              if (success && self) {
                // Add the response to the cache
                self->AddToCache(cache_key, response, provider_id);
              }
              std::move(callback).Run(success, response);
            },
            weak_ptr_factory_.GetWeakPtr(),
            cache_key,
            active_provider_id_,
            std::move(callback)));
  } else {
    active_provider->ProcessRequest(params, std::move(callback));
  }
}

void MultiAdapterManager::ProcessRequestWithProvider(
    const std::string& provider_id,
    const AIRequestParams& params,
    AIResponseCallback callback) {
  // Check if the response is in the cache
  if (cache_config_.enabled) {
    std::string cache_key = GenerateCacheKey(params);
    std::string cached_response;
    
    if (CheckCache(cache_key, &cached_response)) {
      LOG(INFO) << "Cache hit for request: " << cache_key;
      std::move(callback).Run(true, cached_response);
      return;
    }
  }
  
  AIServiceProvider* provider = GetProvider(provider_id);
  if (!provider) {
    std::move(callback).Run(false, "Provider not found: " + provider_id);
    return;
  }
  
  // Check if the provider supports this task type
  if (!provider->SupportsTaskType(params.task_type)) {
    std::move(callback).Run(false, "Provider " + provider_id + " doesn't support this task type.");
    return;
  }
  
  // Process the request with the specified provider
  if (cache_config_.enabled) {
    std::string cache_key = GenerateCacheKey(params);
    provider->ProcessRequest(
        params,
        base::BindOnce(
            [](base::WeakPtr<MultiAdapterManager> self,
               std::string cache_key,
               std::string provider_id,
               AIResponseCallback callback,
               bool success,
               const std::string& response) {
              if (success && self) {
                // Add the response to the cache
                self->AddToCache(cache_key, response, provider_id);
              }
              std::move(callback).Run(success, response);
            },
            weak_ptr_factory_.GetWeakPtr(),
            cache_key,
            provider_id,
            std::move(callback)));
  } else {
    provider->ProcessRequest(params, std::move(callback));
  }
}

bool MultiAdapterManager::ConfigureProvider(
    const std::string& provider_id,
    const std::unordered_map<std::string, std::string>& config) {
  AIServiceProvider* provider = GetProvider(provider_id);
  if (!provider) {
    return false;
  }
  
  provider->Configure(config);
  return true;
}

std::unordered_map<std::string, std::string> MultiAdapterManager::GetProviderConfiguration(
    const std::string& provider_id) const {
  AIServiceProvider* provider = GetProvider(provider_id);
  if (!provider) {
    return {};
  }
  
  return provider->GetConfiguration();
}

std::string MultiAdapterManager::FindBestProviderForTask(
    AIServiceProvider::TaskType task_type) const {
  // First, check if the active provider supports this task
  AIServiceProvider* active_provider = GetActiveProvider();
  if (active_provider && active_provider->SupportsTaskType(task_type)) {
    return active_provider_id_;
  }
  
  // Otherwise, find the first provider that supports this task
  for (const auto& [id, provider] : providers_) {
    if (provider->SupportsTaskType(task_type)) {
      return id;
    }
  }
  
  // No provider found that supports this task
  return "";
}

void MultiAdapterManager::ConfigureCache(const CacheConfig& config) {
  cache_config_ = config;
  
  // If the cache size is being reduced, clean it up
  if (response_cache_.size() > cache_config_.max_entries) {
    CleanCache();
  }
  
  LOG(INFO) << "Cache configured: enabled=" << (cache_config_.enabled ? "true" : "false")
            << ", max_entries=" << cache_config_.max_entries
            << ", max_age_seconds=" << cache_config_.max_age_seconds;
}

void MultiAdapterManager::ClearCache() {
  response_cache_.clear();
  LOG(INFO) << "Cache cleared";
}

MultiAdapterManager::CacheStats MultiAdapterManager::GetCacheStats() const {
  CacheStats stats;
  stats.total_entries = response_cache_.size();
  stats.hits = cache_hits_;
  stats.misses = cache_misses_;
  stats.hit_rate = (cache_hits_ + cache_misses_ > 0) 
      ? static_cast<double>(cache_hits_) / (cache_hits_ + cache_misses_) 
      : 0.0;
  
  return stats;
}

std::string MultiAdapterManager::GenerateCacheKey(const AIRequestParams& params) const {
  // Create a hash of the request parameters
  std::stringstream ss;
  ss << static_cast<int>(params.task_type) << "|"
     << params.input_text << "|";
  
  // Include any additional parameters that affect the response
  for (const auto& [key, value] : params.additional_params) {
    ss << key << "=" << value << "|";
  }
  
  // Use a simple hash function for the key
  std::hash<std::string> hasher;
  return std::to_string(hasher(ss.str()));
}

bool MultiAdapterManager::CheckCache(const std::string& cache_key, std::string* response) const {
  if (!cache_config_.enabled) {
    return false;
  }
  
  auto it = response_cache_.find(cache_key);
  if (it == response_cache_.end()) {
    // Cache miss
    cache_misses_++;
    return false;
  }
  
  // Check if the entry is expired
  auto now = std::chrono::steady_clock::now();
  auto age = std::chrono::duration_cast<std::chrono::seconds>(
      now - it->second.timestamp).count();
  
  if (age > cache_config_.max_age_seconds) {
    // Entry is expired
    cache_misses_++;
    return false;
  }
  
  // Cache hit
  *response = it->second.response;
  cache_hits_++;
  return true;
}

void MultiAdapterManager::AddToCache(
    const std::string& cache_key,
    const std::string& response,
    const std::string& provider_id) {
  if (!cache_config_.enabled) {
    return;
  }
  
  // Clean the cache if it's getting too large
  if (response_cache_.size() >= cache_config_.max_entries) {
    CleanCache();
  }
  
  // Add the new entry
  CacheEntry entry;
  entry.response = response;
  entry.timestamp = std::chrono::steady_clock::now();
  entry.provider_id = provider_id;
  
  response_cache_[cache_key] = std::move(entry);
}

void MultiAdapterManager::CleanCache() {
  // If the cache is empty, nothing to do
  if (response_cache_.empty()) {
    return;
  }
  
  auto now = std::chrono::steady_clock::now();
  
  // First, remove expired entries
  for (auto it = response_cache_.begin(); it != response_cache_.end();) {
    auto age = std::chrono::duration_cast<std::chrono::seconds>(
        now - it->second.timestamp).count();
    
    if (age > cache_config_.max_age_seconds) {
      it = response_cache_.erase(it);
    } else {
      ++it;
    }
  }
  
  // If we're still over the limit, remove the oldest entries
  if (response_cache_.size() > cache_config_.max_entries) {
    // Create a vector of pairs (key, timestamp) for sorting
    std::vector<std::pair<std::string, std::chrono::time_point<std::chrono::steady_clock>>> 
        entries;
    entries.reserve(response_cache_.size());
    
    for (const auto& [key, entry] : response_cache_) {
      entries.emplace_back(key, entry.timestamp);
    }
    
    // Sort by timestamp (oldest first)
    std::sort(entries.begin(), entries.end(),
              [](const auto& a, const auto& b) {
                return a.second < b.second;
              });
    
    // Remove oldest entries until we're under the limit
    size_t to_remove = response_cache_.size() - cache_config_.max_entries;
    for (size_t i = 0; i < to_remove; ++i) {
      response_cache_.erase(entries[i].first);
    }
  }
}

}  // namespace core
}  // namespace asol