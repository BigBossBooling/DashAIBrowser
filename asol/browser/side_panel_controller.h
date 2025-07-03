// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASOL_BROWSER_SIDE_PANEL_CONTROLLER_H_
#define ASOL_BROWSER_SIDE_PANEL_CONTROLLER_H_

#include <memory>
#include <string>

#include "asol/browser/asol_browser_integration.h"
#include "asol/ui/asol_ui_controller.h"
#include "base/memory/weak_ptr.h"
#include "components/side_panel/side_panel_entry.h"
#include "components/side_panel/side_panel_registry.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class WebContents;
}

namespace asol {
namespace browser {

// SidePanelController manages the ASOL side panel integration.
// It registers the ASOL side panel with the side panel registry
// and handles showing and hiding the panel.
class SidePanelController
    : public content::WebContentsObserver,
      public content::WebContentsUserData<SidePanelController> {
 public:
  ~SidePanelController() override;

  // Disallow copy and assign
  SidePanelController(const SidePanelController&) = delete;
  SidePanelController& operator=(const SidePanelController&) = delete;

  // Show the ASOL side panel
  void ShowSidePanel();

  // Hide the ASOL side panel
  void HideSidePanel();

  // Toggle the ASOL side panel
  void ToggleSidePanel();

  // Check if the ASOL side panel is visible
  bool IsSidePanelVisible() const;

  // Get the UI controller
  ui::AsolUiController* GetUiController() const;

  // Initialize the side panel controller
  bool Initialize();

 private:
  // Allow WebContentsUserData to create instances of this class
  friend class content::WebContentsUserData<SidePanelController>;

  // Constructor
  explicit SidePanelController(content::WebContents* web_contents);

  // WebContentsObserver implementation
  void WebContentsDestroyed() override;

  // Register the ASOL side panel with the side panel registry
  void RegisterSidePanel();

  // Create the side panel entry
  std::unique_ptr<side_panel::SidePanelEntry> CreateSidePanelEntry();

  // The UI controller
  std::unique_ptr<ui::AsolUiController> ui_controller_;

  // The side panel registry
  side_panel::SidePanelRegistry* side_panel_registry_ = nullptr;

  // The side panel entry ID
  std::string side_panel_entry_id_;

  // Whether the side panel is visible
  bool is_side_panel_visible_ = false;

  // For generating weak pointers to this
  base::WeakPtrFactory<SidePanelController> weak_ptr_factory_{this};

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace browser
}  // namespace asol

#endif  // ASOL_BROWSER_SIDE_PANEL_CONTROLLER_H_