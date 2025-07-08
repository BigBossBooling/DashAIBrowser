// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser_core/ui/ai_provider_menu_button.h"

#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/controls/menu/menu_model_adapter.h"
#include "ui/views/controls/menu/menu_runner.h"

namespace browser_core {
namespace ui {

namespace {
// Menu item IDs
constexpr int kSettingsMenuItemId = -1;
constexpr int kProviderMenuItemIdBase = 100;

// Provider icons (placeholder for now)
const gfx::VectorIcon& GetProviderIcon(const std::string& provider_id) {
  // In a real implementation, we would have custom icons for each provider
  // For now, we'll use placeholder icons
  static const gfx::VectorIcon kPlaceholderIcon;
  return kPlaceholderIcon;
}

// Provider colors
SkColor GetProviderColor(const std::string& provider_id) {
  // In a real implementation, we would have custom colors for each provider
  if (provider_id == "gemini") {
    return SkColorSetRGB(0x42, 0x85, 0xF4);  // Google Blue
  } else if (provider_id == "openai") {
    return SkColorSetRGB(0x10, 0xA3, 0x7F);  // OpenAI Green
  } else if (provider_id == "copilot") {
    return SkColorSetRGB(0x00, 0x78, 0xD4);  // Microsoft Blue
  } else if (provider_id == "claude") {
    return SkColorSetRGB(0xA1, 0x00, 0xFF);  // Anthropic Purple
  }
  return gfx::kPlaceholderColor;
}
}  // namespace

// static
DEFINE_CLASS_ELEMENT_IDENTIFIER_VALUE(AIProviderMenuButton);

AIProviderMenuButton::AIProviderMenuButton(BrowserAIIntegration* ai_integration)
    : views::MenuButton(
          base::BindRepeating(&AIProviderMenuButton::ShowMenu,
                             base::Unretained(this)),
          base::UTF8ToUTF16("AI"),
          views::MenuButton::Type::kRect,
          false),
      ai_integration_(ai_integration),
      multi_adapter_manager_(ai_integration_->GetMultiAdapterManager()),
      weak_ptr_factory_(this) {
  DCHECK(ai_integration_);
  DCHECK(multi_adapter_manager_);
  
  SetTooltipText(base::UTF8ToUTF16("Select AI Provider"));
  SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
  
  // Update the button appearance based on the active provider
  UpdateButtonAppearance();
  
  LOG(INFO) << "AIProviderMenuButton initialized.";
}

AIProviderMenuButton::~AIProviderMenuButton() = default;

void AIProviderMenuButton::OnBoundsChanged(const gfx::Rect& previous_bounds) {
  MenuButton::OnBoundsChanged(previous_bounds);
  // Update the button appearance if the size changed
  if (previous_bounds.size() != size()) {
    UpdateButtonAppearance();
  }
}

void AIProviderMenuButton::OnThemeChanged() {
  MenuButton::OnThemeChanged();
  // Update the button appearance when the theme changes
  UpdateButtonAppearance();
}

void AIProviderMenuButton::CreateMenuModel() {
  // Create a new menu model
  auto menu_model = std::make_unique<ui::SimpleMenuModel>(this);
  
  // Add provider options
  auto provider_ids = multi_adapter_manager_->GetRegisteredProviderIds();
  auto provider_names = multi_adapter_manager_->GetRegisteredProviderNames();
  std::string active_provider_id = multi_adapter_manager_->GetActiveProviderId();
  
  for (size_t i = 0; i < provider_ids.size(); ++i) {
    const std::string& provider_id = provider_ids[i];
    const std::string& provider_name = provider_names[i];
    
    // Add a radio item for each provider
    menu_model->AddRadioItemWithStringId(
        kProviderMenuItemIdBase + i,
        base::UTF8ToUTF16(provider_name),
        0,  // group id
        provider_id == active_provider_id);
  }
  
  // Add a separator
  menu_model->AddSeparator(ui::NORMAL_SEPARATOR);
  
  // Add settings option
  menu_model->AddItemWithStringId(
      kSettingsMenuItemId,
      base::UTF8ToUTF16("AI Settings..."),
      base::UTF8ToUTF16("Configure AI providers"));
  
  // Create the menu runner
  menu_runner_ = std::make_unique<views::MenuRunner>(
      std::make_unique<views::MenuModelAdapter>(std::move(menu_model)),
      views::MenuRunner::HAS_MNEMONICS);
}

void AIProviderMenuButton::ShowMenu() {
  // Create the menu model
  CreateMenuModel();
  
  // Show the menu
  menu_runner_->RunMenuAt(
      GetWidget(),
      nullptr,  // button controller
      GetBoundsInScreen(),
      views::MenuAnchorPosition::kTopRight,
      ui::MENU_SOURCE_MOUSE);
}

void AIProviderMenuButton::OnProviderSelected(const std::string& provider_id) {
  // Set the active provider
  if (multi_adapter_manager_->SetActiveProvider(provider_id)) {
    LOG(INFO) << "Switched to AI provider: " << provider_id;
    
    // Update the button appearance
    UpdateButtonAppearance();
  }
}

void AIProviderMenuButton::OnSettingsSelected() {
  // Show the AI settings page
  ai_integration_->ShowAISettingsPage();
}

void AIProviderMenuButton::UpdateButtonAppearance() {
  // Get the active provider
  std::string active_provider_id = multi_adapter_manager_->GetActiveProviderId();
  auto provider = multi_adapter_manager_->GetActiveProvider();
  
  if (!provider) {
    return;
  }
  
  // Set the button text to the provider name
  SetText(base::UTF8ToUTF16(provider->GetProviderName()));
  
  // Set the button color based on the provider
  SetEnabledTextColors(GetProviderColor(active_provider_id));
  
  // Set the button icon based on the provider
  // In a real implementation, we would use custom icons for each provider
  // SetImage(views::Button::STATE_NORMAL, gfx::CreateVectorIcon(
  //     GetProviderIcon(active_provider_id),
  //     16,
  //     GetProviderColor(active_provider_id)));
}

}  // namespace ui
}  // namespace browser_core