// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "asol/browser/asol_browser_integration.h"

#include <memory>
#include <string>

#include "asol/adapters/adapter_interface.h"
#include "asol/core/config_loader.h"
#include "asol/core/service_manager.h"
#include "asol/ui/asol_ui_controller.h"
#include "base/logging.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "ui/gfx/native_widget_types.h"

namespace asol {
namespace browser {

// static
WEB_CONTENTS_USER_DATA_KEY_IMPL(AsolBrowserIntegration);

AsolBrowserIntegration::AsolBrowserIntegration(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<AsolBrowserIntegration>(*web_contents) {
  DLOG(INFO) << "AsolBrowserIntegration created for WebContents: " 
             << web_contents;
  
  // Create the UI controller
  ui_controller_ = ui::AsolUiController::Create();
  
  // Initialize the integration
  Initialize();
}

AsolBrowserIntegration::~AsolBrowserIntegration() {
  DLOG(INFO) << "AsolBrowserIntegration destroyed";
}

void AsolBrowserIntegration::ShowAiPanel() {
  if (!ui_controller_) {
    return;
  }
  
  // Get the native window from the web contents
  gfx::NativeWindow native_window = nullptr;
  if (web_contents()) {
    native_window = web_contents()->GetTopLevelNativeWindow();
  }
  
  // Show the AI panel
  ui_controller_->ShowAiPanel(native_window);
  is_panel_visible_ = true;
}

void AsolBrowserIntegration::HideAiPanel() {
  if (!ui_controller_) {
    return;
  }
  
  // Hide the AI panel
  ui_controller_->HideAiPanel();
  is_panel_visible_ = false;
}

void AsolBrowserIntegration::ToggleAiPanel() {
  if (is_panel_visible_) {
    HideAiPanel();
  } else {
    ShowAiPanel();
  }
}

ui::AsolUiController* AsolBrowserIntegration::GetUiController() const {
  return ui_controller_.get();
}

bool AsolBrowserIntegration::Initialize() {
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

void AsolBrowserIntegration::WebContentsDestroyed() {
  // Hide the AI panel before the web contents is destroyed
  HideAiPanel();
}

void AsolBrowserIntegration::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() || !navigation_handle->HasCommitted()) {
    return;
  }
  
  // The main frame has navigated to a new page
  DLOG(INFO) << "Main frame navigated to: " << navigation_handle->GetURL();
  
  // In a real implementation, we might want to update the AI panel
  // with context from the new page
}

void AsolBrowserIntegration::DidFinishLoad(
    content::RenderFrameHost* render_frame_host,
    const GURL& validated_url) {
  if (!render_frame_host->IsInPrimaryMainFrame()) {
    return;
  }
  
  // The main frame has finished loading
  DLOG(INFO) << "Main frame finished loading: " << validated_url;
  
  // In a real implementation, we might want to extract content from the page
  // to provide context for the AI
}

}  // namespace browser
}  // namespace asol