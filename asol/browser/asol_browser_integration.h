// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASOL_BROWSER_ASOL_BROWSER_INTEGRATION_H_
#define ASOL_BROWSER_ASOL_BROWSER_INTEGRATION_H_

#include <memory>
#include <string>

#include "asol/ui/asol_ui_controller.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class WebContents;
}

namespace asol {
namespace browser {

// AsolBrowserIntegration integrates the ASOL system with the browser.
// It observes web contents and provides AI capabilities to the browser.
class AsolBrowserIntegration
    : public content::WebContentsObserver,
      public content::WebContentsUserData<AsolBrowserIntegration> {
 public:
  ~AsolBrowserIntegration() override;

  // Disallow copy and assign
  AsolBrowserIntegration(const AsolBrowserIntegration&) = delete;
  AsolBrowserIntegration& operator=(const AsolBrowserIntegration&) = delete;

  // Show the AI panel
  void ShowAiPanel();

  // Hide the AI panel
  void HideAiPanel();

  // Toggle the AI panel
  void ToggleAiPanel();

  // Get the UI controller
  ui::AsolUiController* GetUiController() const;

  // Initialize the browser integration
  bool Initialize();

 private:
  // Allow WebContentsUserData to create instances of this class
  friend class content::WebContentsUserData<AsolBrowserIntegration>;

  // Constructor
  explicit AsolBrowserIntegration(content::WebContents* web_contents);

  // WebContentsObserver implementation
  void WebContentsDestroyed() override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                    const GURL& validated_url) override;

  // The UI controller
  std::unique_ptr<ui::AsolUiController> ui_controller_;

  // Whether the AI panel is visible
  bool is_panel_visible_ = false;

  // For generating weak pointers to this
  base::WeakPtrFactory<AsolBrowserIntegration> weak_ptr_factory_{this};

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace browser
}  // namespace asol

#endif  // ASOL_BROWSER_ASOL_BROWSER_INTEGRATION_H_