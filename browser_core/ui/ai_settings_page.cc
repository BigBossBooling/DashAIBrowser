// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser_core/ui/ai_settings_page.h"

#include <utility>

#include "base/logging.h"

namespace browser_core {
namespace ui {

namespace {
// Default configuration values for each provider
std::unordered_map<std::string, std::unordered_map<std::string, std::string>> GetDefaultConfigs() {
  std::unordered_map<std::string, std::unordered_map<std::string, std::string>> defaults;
  
  // Gemini defaults
  defaults["gemini"] = {
    {"model", "gemini-pro"},
    {"temperature", "0.7"},
    {"max_output_tokens", "1024"},
    {"top_p", "0.95"},
    {"top_k", "40"}
  };
  
  // OpenAI defaults
  defaults["openai"] = {
    {"model", "gpt-4o"},
    {"temperature", "0.7"},
    {"max_tokens", "1024"},
    {"top_p", "0.95"},
    {"frequency_penalty", "0.0"},
    {"presence_penalty", "0.0"}
  };
  
  // Copilot defaults
  defaults["copilot"] = {
    {"model", "copilot-4"},
    {"temperature", "0.7"},
    {"max_tokens", "1024"},
    {"api_version", "2023-12-01-preview"}
  };
  
  // Claude defaults
  defaults["claude"] = {
    {"model", "claude-3-opus-20240229"},
    {"temperature", "0.7"},
    {"max_tokens", "1024"},
    {"top_p", "0.95"},
    {"top_k", "40"},
    {"anthropic_version", "2023-06-01"}
  };
  
  return defaults;
}
}  // namespace

AISettingsPage::AISettingsPage(asol::core::MultiAdapterManager* adapter_manager)
    : adapter_manager_(adapter_manager) {
  DCHECK(adapter_manager_);
  LOG(INFO) << "AISettingsPage created.";
}

AISettingsPage::~AISettingsPage() {
  LOG(INFO) << "AISettingsPage destroyed.";
}

void AISettingsPage::Initialize() {
  if (initialized_) {
    return;
  }
  
  LOG(INFO) << "Initializing AI Settings Page";
  
  // Create the provider selector
  provider_selector_ = std::make_unique<AIProviderSelector>(adapter_manager_);
  
  // Set up the callback for provider changes
  provider_selector_->SetProviderChangedCallback(
      base::BindRepeating(&AISettingsPage::OnProviderChanged,
                         weak_ptr_factory_.GetWeakPtr()));
  
  // Initialize the current configuration with the selected provider's config
  UpdateConfigUI();
  
  initialized_ = true;
}

void AISettingsPage::Show() {
  if (!initialized_) {
    Initialize();
  }
  
  LOG(INFO) << "Showing AI Settings Page";
  
  // Create the UI elements
  provider_selector_->CreateUI();
  
  // Log the current configuration
  LOG(INFO) << "Current configuration for " << provider_selector_->GetSelectedProviderId() << ":";
  for (const auto& [key, value] : current_config_) {
    LOG(INFO) << "  " << key << ": " << value;
  }
  
  visible_ = true;
}

void AISettingsPage::Hide() {
  LOG(INFO) << "Hiding AI Settings Page";
  visible_ = false;
}

void AISettingsPage::ApplySettings() {
  if (!initialized_) {
    return;
  }
  
  std::string provider_id = provider_selector_->GetSelectedProviderId();
  LOG(INFO) << "Applying settings for " << provider_id;
  
  // Apply the configuration to the provider
  adapter_manager_->ConfigureProvider(provider_id, current_config_);
  
  LOG(INFO) << "Settings applied successfully.";
}

void AISettingsPage::ResetToDefaults() {
  if (!initialized_) {
    return;
  }
  
  std::string provider_id = provider_selector_->GetSelectedProviderId();
  LOG(INFO) << "Resetting " << provider_id << " to default settings";
  
  // Get the default configuration for the selected provider
  auto defaults = GetDefaultConfigs();
  if (defaults.find(provider_id) != defaults.end()) {
    current_config_ = defaults[provider_id];
    
    // Apply the defaults
    adapter_manager_->ConfigureProvider(provider_id, current_config_);
    
    // Update the UI
    if (visible_) {
      LOG(INFO) << "Updated configuration to defaults:";
      for (const auto& [key, value] : current_config_) {
        LOG(INFO) << "  " << key << ": " << value;
      }
    }
  }
}

std::unordered_map<std::string, std::string> AISettingsPage::GetCurrentConfig() const {
  return current_config_;
}

void AISettingsPage::SetConfigValue(const std::string& key, const std::string& value) {
  current_config_[key] = value;
  
  LOG(INFO) << "Updated configuration value: " << key << " = " << value;
}

void AISettingsPage::OnProviderChanged(const std::string& provider_id) {
  LOG(INFO) << "Provider changed to: " << provider_id;
  
  // Update the configuration UI for the new provider
  UpdateConfigUI();
}

void AISettingsPage::UpdateConfigUI() {
  std::string provider_id = provider_selector_->GetSelectedProviderId();
  
  // Get the current configuration from the provider
  current_config_ = adapter_manager_->GetProviderConfiguration(provider_id);
  
  // If the configuration is empty, use defaults
  if (current_config_.empty()) {
    auto defaults = GetDefaultConfigs();
    if (defaults.find(provider_id) != defaults.end()) {
      current_config_ = defaults[provider_id];
    }
  }
  
  // Update the UI if visible
  if (visible_) {
    LOG(INFO) << "Updated configuration for " << provider_id << ":";
    for (const auto& [key, value] : current_config_) {
      LOG(INFO) << "  " << key << ": " << value;
    }
  }
}