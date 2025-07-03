// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "asol/browser/side_panel_controller.h"

#include <memory>
#include <string>

#include "asol/browser/browser_features.h"
#include "asol/browser/page_context_extractor.h"
#include "asol/core/config_loader.h"
#include "asol/core/service_manager.h"
#include "asol/ui/asol_ui_controller.h"
#include "base/feature_list.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "components/side_panel/side_panel_entry.h"
#include "components/side_panel/side_panel_registry.h"
#include "components/side_panel/side_panel_ui.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/view.h"

namespace asol {
namespace browser {

// static
WEB_CONTENTS_USER_DATA_KEY_IMPL(SidePanelController);

SidePanelController::SidePanelController(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<SidePanelController>(*web_contents) {
  DLOG(INFO) << "SidePanelController created for WebContents: " << web_contents;
  
  // Get the side panel registry
  side_panel_registry_ = side_panel::SidePanelRegistry::GetOrCreateForWebContents(
      web_contents);
  
  // Create the UI controller
  ui_controller_ = ui::AsolUiController::Create();
  
  // Initialize the controller
  Initialize();
  
  // Register the side panel
  RegisterSidePanel();
  
  // Show the panel on startup if enabled
  if (base::GetFieldTrialParamByFeatureAsBool(
          kAsolSidePanelIntegration, "show_panel_on_startup", false)) {
    ShowSidePanel();
  }
}

SidePanelController::~SidePanelController() {
  DLOG(INFO) << "SidePanelController destroyed";
  
  // Unregister the side panel
  if (side_panel_registry_ && !side_panel_entry_id_.empty()) {
    side_panel_registry_->Remove(side_panel_entry_id_);
  }
}

void SidePanelController::ShowSidePanel() {
  if (!side_panel_registry_ || side_panel_entry_id_.empty()) {
    return;
  }
  
  side_panel_registry_->Show(side_panel_entry_id_);
  is_side_panel_visible_ = true;
}

void SidePanelController::HideSidePanel() {
  if (!side_panel_registry_ || side_panel_entry_id_.empty()) {
    return;
  }
  
  side_panel_registry_->Hide(side_panel_entry_id_);
  is_side_panel_visible_ = false;
}

void SidePanelController::ToggleSidePanel() {
  if (IsSidePanelVisible()) {
    HideSidePanel();
  } else {
    ShowSidePanel();
  }
}

bool SidePanelController::IsSidePanelVisible() const {
  return is_side_panel_visible_;
}

ui::AsolUiController* SidePanelController::GetUiController() const {
  return ui_controller_.get();
}

bool SidePanelController::Initialize() {
  if (!ui_controller_) {
    return false;
  }
  
  // Get the service manager
  auto* service_manager = core::ServiceManager::GetInstance();
  
  // Register adapters
  service_manager->RegisterAdapter("gemini", adapters::CreateAdapter("gemini"));
  
  // Load configuration
  std::string config_json = core::ConfigLoader::LoadDefault();
  
  // Initialize the UI controller
  return ui_controller_->Initialize(config_json);
}

void SidePanelController::WebContentsDestroyed() {
  // Hide the side panel before the web contents is destroyed
  HideSidePanel();
}

void SidePanelController::RegisterSidePanel() {
  if (!side_panel_registry_) {
    return;
  }
  
  // Create the side panel entry
  auto entry = CreateSidePanelEntry();
  side_panel_entry_id_ = entry->id();
  
  // Register the entry with the registry
  side_panel_registry_->Register(std::move(entry));
}

std::unique_ptr<side_panel::SidePanelEntry> SidePanelController::CreateSidePanelEntry() {
  // Create a new side panel entry
  auto entry = std::make_unique<side_panel::SidePanelEntry>(
      side_panel::SidePanelEntry::Id("asol_side_panel"));
  
  // Set the title
  entry->SetTitle(base::UTF8ToUTF16("AI Assistant"));
  
  // Set the availability callback
  entry->SetAvailabilityCallback(base::BindRepeating([]() {
    return base::FeatureList::IsEnabled(kAsolSidePanelIntegration);
  }));
  
  // Set the icon
  // TODO: Replace with actual icon when available
  entry->SetIcon(gfx::ImageSkia());
  
  // Set the view factory
  entry->SetViewFactory(base::BindRepeating(
      [](SidePanelController* controller) {
        // Create the view for the side panel
        // In a real implementation, this would create a view that hosts the UI panel
        auto view = std::make_unique<views::View>();
        
        // Get the UI controller
        ui::AsolUiController* ui_controller = controller->GetUiController();
        
        // Show the AI panel in the view
        if (ui_controller) {
          ui_controller->ShowAiPanel(nullptr);
        }
        
        return view;
      },
      base::Unretained(this)));
  
  return entry;
}

}  // namespace browser
}  // namespace asol