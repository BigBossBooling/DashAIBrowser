// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_CORE_UI_AI_PROVIDER_SELECTOR_H_
#define BROWSER_CORE_UI_AI_PROVIDER_SELECTOR_H_

#include <memory>
#include <string>
#include <vector>

#include "asol/core/multi_adapter_manager.h"
#include "base/memory/weak_ptr.h"

namespace browser_core {
namespace ui {

// AIProviderSelector provides UI for selecting and configuring AI providers
class AIProviderSelector {
 public:
  // Callback for provider selection changes
  using ProviderChangedCallback = base::RepeatingCallback<void(const std::string&)>;
  
  explicit AIProviderSelector(asol::core::MultiAdapterManager* adapter_manager);
  ~AIProviderSelector();

  // Disallow copy and assign
  AIProviderSelector(const AIProviderSelector&) = delete;
  AIProviderSelector& operator=(const AIProviderSelector&) = delete;

  // Get the list of available provider IDs
  std::vector<std::string> GetAvailableProviderIds() const;
  
  // Get the list of available provider names
  std::vector<std::string> GetAvailableProviderNames() const;
  
  // Get the currently selected provider ID
  std::string GetSelectedProviderId() const;
  
  // Select a provider by ID
  bool SelectProvider(const std::string& provider_id);
  
  // Get the configuration for the selected provider
  std::unordered_map<std::string, std::string> GetSelectedProviderConfig() const;
  
  // Update the configuration for the selected provider
  bool UpdateSelectedProviderConfig(
      const std::unordered_map<std::string, std::string>& config);
  
  // Set a callback to be notified when the provider changes
  void SetProviderChangedCallback(ProviderChangedCallback callback);
  
  // Create the UI elements for provider selection
  // In a real implementation, this would return UI components
  void CreateUI();
  
 private:
  // The multi-adapter manager
  asol::core::MultiAdapterManager* adapter_manager_;
  
  // Callback for provider changes
  ProviderChangedCallback provider_changed_callback_;
  
  // For weak pointers
  base::WeakPtrFactory<AIProviderSelector> weak_ptr_factory_{this};
};

}  // namespace ui
}  // namespace browser_core

#endif  // BROWSER_CORE_UI_AI_PROVIDER_SELECTOR_H_