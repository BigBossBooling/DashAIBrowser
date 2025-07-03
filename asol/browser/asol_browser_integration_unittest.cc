// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "asol/browser/asol_browser_integration.h"

#include <memory>

#include "asol/ui/asol_ui_controller.h"
#include "base/test/task_environment.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace asol {
namespace browser {
namespace {

class AsolBrowserIntegrationTest : public testing::Test {
 protected:
  AsolBrowserIntegrationTest()
      : render_view_host_factory_(),
        task_environment_(base::test::TaskEnvironment::MainThreadType::UI) {}

  void SetUp() override {
    // Create a test web contents
    web_contents_ = content::WebContentsTester::CreateTestWebContents(
        browser_context_.get(), nullptr);
    
    // Create the browser integration
    browser_integration_ =
        AsolBrowserIntegration::CreateForWebContents(web_contents_.get());
    ASSERT_TRUE(browser_integration_);
  }

  void TearDown() override {
    web_contents_.reset();
  }

  // Task environment for running UI tasks
  base::test::TaskEnvironment task_environment_;
  
  // Test browser context
  content::TestBrowserContext browser_context_;
  
  // Test renderer host factory
  content::RenderViewHostTestEnabler render_view_host_factory_;
  
  // Test web contents
  std::unique_ptr<content::WebContents> web_contents_;
  
  // The browser integration under test
  AsolBrowserIntegration* browser_integration_ = nullptr;
};

TEST_F(AsolBrowserIntegrationTest, GetUiController) {
  // Verify that the UI controller is created
  EXPECT_TRUE(browser_integration_->GetUiController());
}

TEST_F(AsolBrowserIntegrationTest, ToggleAiPanel) {
  // Initially, the panel should not be visible
  EXPECT_FALSE(browser_integration_->GetUiController()->IsAiPanelVisible());
  
  // Show the panel
  browser_integration_->ShowAiPanel();
  
  // The panel should now be visible
  EXPECT_TRUE(browser_integration_->GetUiController()->IsAiPanelVisible());
  
  // Hide the panel
  browser_integration_->HideAiPanel();
  
  // The panel should now be hidden
  EXPECT_FALSE(browser_integration_->GetUiController()->IsAiPanelVisible());
  
  // Toggle the panel (should show it)
  browser_integration_->ToggleAiPanel();
  
  // The panel should now be visible
  EXPECT_TRUE(browser_integration_->GetUiController()->IsAiPanelVisible());
  
  // Toggle the panel again (should hide it)
  browser_integration_->ToggleAiPanel();
  
  // The panel should now be hidden
  EXPECT_FALSE(browser_integration_->GetUiController()->IsAiPanelVisible());
}

}  // namespace
}  // namespace browser
}  // namespace asol