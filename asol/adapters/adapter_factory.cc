// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "asol/adapters/adapter_factory.h"

#include <utility>

#include "asol/adapters/gemini/gemini_service_provider.h"
#include "asol/adapters/openai/openai_service_provider.h"
#include "asol/adapters/copilot/copilot_service_provider.h"
#include "asol/adapters/claude/claude_service_provider.h"
#include "base/logging.h"

namespace asol {
namespace adapters {

namespace {
// Constants for adapter IDs
constexpr char kGeminiAdapterId[] = "gemini";
constexpr char kOpenAIAdapterId[] = "openai";
constexpr char kCopilotAdapterId[] = "copilot";
constexpr char kClaudeAdapterId[] = "claude";

// Constants for adapter names
constexpr char kGeminiAdapterName[] = "Google Gemini";
constexpr char kOpenAIAdapterName[] = "OpenAI";
constexpr char kCopilotAdapterName[] = "Microsoft Copilot";
constexpr char kClaudeAdapterName[] = "Anthropic Claude";

// Configuration keys
constexpr char kConfigKeyApiKey[] = "api_key";
constexpr char kConfigKeyDefaultProvider[] = "default_provider";
}  // namespace

std::unique_ptr<core::MultiAdapterManager> AdapterFactory::CreateMultiAdapterManager(
    const std::unordered_map<std::string, std::string>& config) {
  auto manager = std::make_unique<core::MultiAdapterManager>();
  
  // Create and register all available adapters
  for (const auto& adapter_id : GetSupportedAdapterIds()) {
    auto adapter = CreateAdapter(adapter_id, config);
    if (adapter) {
      manager->RegisterProvider(std::move(adapter));
    }
  }
  
  // Set the default provider if specified in the config
  if (config.find(kConfigKeyDefaultProvider) != config.end()) {
    std::string default_provider = config.at(kConfigKeyDefaultProvider);
    if (manager->SetActiveProvider(default_provider)) {
      LOG(INFO) << "Set default provider to " << default_provider;
    } else {
      LOG(ERROR) << "Failed to set default provider to " << default_provider;
    }
  }
  
  return manager;
}

std::unique_ptr<core::AIServiceProvider> AdapterFactory::CreateAdapter(
    const std::string& adapter_id,
    const std::unordered_map<std::string, std::string>& config) {
  if (adapter_id == kGeminiAdapterId) {
    return CreateGeminiAdapter(config);
  } else if (adapter_id == kOpenAIAdapterId) {
    return CreateOpenAIAdapter(config);
  } else if (adapter_id == kCopilotAdapterId) {
    return CreateCopilotAdapter(config);
  } else if (adapter_id == kClaudeAdapterId) {
    return CreateClaudeAdapter(config);
  } else {
    LOG(ERROR) << "Unknown adapter ID: " << adapter_id;
    return nullptr;
  }
}

std::vector<std::string> AdapterFactory::GetSupportedAdapterIds() {
  return {
    kGeminiAdapterId,
    kOpenAIAdapterId,
    kCopilotAdapterId,
    kClaudeAdapterId
  };
}

std::vector<std::string> AdapterFactory::GetSupportedAdapterNames() {
  return {
    kGeminiAdapterName,
    kOpenAIAdapterName,
    kCopilotAdapterName,
    kClaudeAdapterName
  };
}

bool AdapterFactory::IsAdapterSupported(const std::string& adapter_id) {
  return adapter_id == kGeminiAdapterId ||
         adapter_id == kOpenAIAdapterId ||
         adapter_id == kCopilotAdapterId ||
         adapter_id == kClaudeAdapterId;
}

// Helper method to extract provider-specific configuration
std::unordered_map<std::string, std::string> ExtractProviderConfig(
    const std::string& adapter_id,
    const std::unordered_map<std::string, std::string>& config) {
  std::unordered_map<std::string, std::string> provider_config;
  const std::string prefix = adapter_id + "_";
  
  for (const auto& [key, value] : config) {
    // Extract provider-specific config (keys starting with "<adapter_id>_")
    if (key.find(prefix) == 0) {
      std::string config_key = key.substr(prefix.length());
      provider_config[config_key] = value;
    }
  }
  
  return provider_config;
}

// Helper method to get API key for a provider
std::string GetApiKey(
    const std::string& adapter_id,
    const std::unordered_map<std::string, std::string>& config) {
  std::string api_key;
  const std::string provider_key = adapter_id + "_" + kConfigKeyApiKey;
  
  if (config.find(provider_key) != config.end()) {
    api_key = config.at(provider_key);
  } else if (config.find(kConfigKeyApiKey) != config.end()) {
    api_key = config.at(kConfigKeyApiKey);
  }
  
  return api_key;
}

std::unique_ptr<core::AIServiceProvider> AdapterFactory::CreateGeminiAdapter(
    const std::unordered_map<std::string, std::string>& config) {
  std::string api_key = GetApiKey(kGeminiAdapterId, config);
  auto provider = std::make_unique<gemini::GeminiServiceProvider>(api_key);
  
  // Apply any additional configuration
  auto provider_config = ExtractProviderConfig(kGeminiAdapterId, config);
  if (!provider_config.empty()) {
    provider->Configure(provider_config);
  }
  
  return provider;
}

std::unique_ptr<core::AIServiceProvider> AdapterFactory::CreateOpenAIAdapter(
    const std::unordered_map<std::string, std::string>& config) {
  std::string api_key = GetApiKey(kOpenAIAdapterId, config);
  auto provider = std::make_unique<openai::OpenAIServiceProvider>(api_key);
  
  // Apply any additional configuration
  auto provider_config = ExtractProviderConfig(kOpenAIAdapterId, config);
  if (!provider_config.empty()) {
    provider->Configure(provider_config);
  }
  
  return provider;
}

std::unique_ptr<core::AIServiceProvider> AdapterFactory::CreateCopilotAdapter(
    const std::unordered_map<std::string, std::string>& config) {
  std::string api_key = GetApiKey(kCopilotAdapterId, config);
  auto provider = std::make_unique<copilot::CopilotServiceProvider>(api_key);
  
  // Apply any additional configuration
  auto provider_config = ExtractProviderConfig(kCopilotAdapterId, config);
  if (!provider_config.empty()) {
    provider->Configure(provider_config);
  }
  
  return provider;
}

std::unique_ptr<core::AIServiceProvider> AdapterFactory::CreateClaudeAdapter(
    const std::unordered_map<std::string, std::string>& config) {
  std::string api_key = GetApiKey(kClaudeAdapterId, config);
  auto provider = std::make_unique<claude::ClaudeServiceProvider>(api_key);
  
  // Apply any additional configuration
  auto provider_config = ExtractProviderConfig(kClaudeAdapterId, config);
  if (!provider_config.empty()) {
    provider->Configure(provider_config);
  }
  
  return provider;
}

}  // namespace adapters
}  // namespace asol