// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_CORE_UI_AI_SETTINGS_PAGE_H_
#define BROWSER_CORE_UI_AI_SETTINGS_PAGE_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "browser_core/ui/ai_provider_selector.h"
#include "asol/core/multi_adapter_manager.h"
#include "base/memory/weak_ptr.h"

namespace browser_core {
namespace ui {

// AISettingsPage provides a settings page for configuring AI providers
class AISettingsPage {
 public:
  explicit AISettingsPage(asol::core::MultiAdapterManager* adapter_manager);
  ~AISettingsPage();

  // Disallow copy and assign
  AISettingsPage(const AISettingsPage&) = delete;
  AISettingsPage& operator=(const AISettingsPage&) = delete;

  // Initialize the settings page
  void Initialize();
  
  // Show the settings page
  void Show();
  
  // Hide the settings page
  void Hide();
  
  // Apply the current settings
  void ApplySettings();
  
  // Reset settings to defaults
  void ResetToDefaults();
  
  // Get the current provider configuration
  std::unordered_map<std::string, std::string> GetCurrentConfig() const;
  
  // Set a configuration value
  void SetConfigValue(const std::string& key, const std::string& value);
  
 private:
  // Handle provider selection changes
  void OnProviderChanged(const std::string& provider_id);
  
  // Update the UI with the current provider's configuration
  void UpdateConfigUI();
  
  // The multi-adapter manager
  asol::core::MultiAdapterManager* adapter_manager_;
  
  // The provider selector
  std::unique_ptr<AIProviderSelector> provider_selector_;
  
  // Current configuration values (may be unsaved)
  std::unordered_map<std::string, std::string> current_config_;
  
  // Whether the page is initialized
  bool initialized_ = false;
  
  // Whether the page is visible
  bool visible_ = false;
  
  // For weak pointers
  base::WeakPtrFactory<AISettingsPage> weak_ptr_factory_{this};
};

}  // namespace ui
}  // namespace browser_core

#endif  // BROWSER_CORE_UI_AI_SETTINGS_PAGE_H_