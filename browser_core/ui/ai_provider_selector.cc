// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser_core/ui/ai_provider_selector.h"

#include <utility>

#include "base/logging.h"

namespace browser_core {
namespace ui {

AIProviderSelector::AIProviderSelector(asol::core::MultiAdapterManager* adapter_manager)
    : adapter_manager_(adapter_manager) {
  DCHECK(adapter_manager_);
  LOG(INFO) << "AIProviderSelector initialized.";
}

AIProviderSelector::~AIProviderSelector() {
  LOG(INFO) << "AIProviderSelector destroyed.";
}

std::vector<std::string> AIProviderSelector::GetAvailableProviderIds() const {
  return adapter_manager_->GetRegisteredProviderIds();
}

std::vector<std::string> AIProviderSelector::GetAvailableProviderNames() const {
  return adapter_manager_->GetRegisteredProviderNames();
}

std::string AIProviderSelector::GetSelectedProviderId() const {
  return adapter_manager_->GetActiveProviderId();
}

bool AIProviderSelector::SelectProvider(const std::string& provider_id) {
  bool success = adapter_manager_->SetActiveProvider(provider_id);
  
  if (success && !provider_changed_callback_.is_null()) {
    provider_changed_callback_.Run(provider_id);
  }
  
  return success;
}

std::unordered_map<std::string, std::string> AIProviderSelector::GetSelectedProviderConfig() const {
  std::string provider_id = GetSelectedProviderId();
  if (provider_id.empty()) {
    return {};
  }
  
  return adapter_manager_->GetProviderConfiguration(provider_id);
}

bool AIProviderSelector::UpdateSelectedProviderConfig(
    const std::unordered_map<std::string, std::string>& config) {
  std::string provider_id = GetSelectedProviderId();
  if (provider_id.empty()) {
    return false;
  }
  
  return adapter_manager_->ConfigureProvider(provider_id, config);
}

void AIProviderSelector::SetProviderChangedCallback(ProviderChangedCallback callback) {
  provider_changed_callback_ = std::move(callback);
}

void AIProviderSelector::CreateUI() {
  // In a real implementation, this would create UI components for provider selection
  // For now, we'll just log the available providers
  
  LOG(INFO) << "Creating AI Provider Selector UI";
  LOG(INFO) << "Available AI providers:";
  
  auto provider_ids = GetAvailableProviderIds();
  auto provider_names = GetAvailableProviderNames();
  
  for (size_t i = 0; i < provider_ids.size(); ++i) {
    LOG(INFO) << "- " << provider_names[i] << " (ID: " << provider_ids[i] << ")";
  }
  
  LOG(INFO) << "Currently selected provider: " << GetSelectedProviderId();
}

}  // namespace ui
}  // namespace browser_core