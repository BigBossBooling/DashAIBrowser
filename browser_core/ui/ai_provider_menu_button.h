// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_CORE_UI_AI_PROVIDER_MENU_BUTTON_H_
#define BROWSER_CORE_UI_AI_PROVIDER_MENU_BUTTON_H_

#include <memory>
#include <string>
#include <vector>

#include "asol/core/multi_adapter_manager.h"
#include "base/memory/weak_ptr.h"
#include "browser_core/browser_ai_integration.h"
#include "ui/views/controls/button/menu_button.h"
#include "ui/views/controls/menu/menu_runner.h"

namespace browser_core {
namespace ui {

// AIProviderMenuButton is a toolbar button that shows a menu for selecting
// the active AI provider and accessing AI settings.
class AIProviderMenuButton : public views::MenuButton {
 public:
  METADATA_HEADER(AIProviderMenuButton);

  explicit AIProviderMenuButton(BrowserAIIntegration* ai_integration);
  ~AIProviderMenuButton() override;

  // views::MenuButton:
  void OnBoundsChanged(const gfx::Rect& previous_bounds) override;
  void OnThemeChanged() override;

 private:
  // Create the menu model
  void CreateMenuModel();

  // Show the menu
  void ShowMenu();

  // Menu callbacks
  void OnProviderSelected(const std::string& provider_id);
  void OnSettingsSelected();

  // Update the button appearance based on the active provider
  void UpdateButtonAppearance();

  // The AI integration
  BrowserAIIntegration* ai_integration_;

  // The multi-adapter manager
  asol::core::MultiAdapterManager* multi_adapter_manager_;

  // The menu runner
  std::unique_ptr<views::MenuRunner> menu_runner_;

  // For weak pointers
  base::WeakPtrFactory<AIProviderMenuButton> weak_ptr_factory_{this};
};

}  // namespace ui
}  // namespace browser_core

#endif  // BROWSER_CORE_UI_AI_PROVIDER_MENU_BUTTON_H_