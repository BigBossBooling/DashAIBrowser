// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "asol/adapters/adapter_factory.h"
#include "browser_core/browser_ai_integration.h"
#include "browser_core/ui/ai_provider_menu_button.h"
#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/run_loop.h"
#include "base/task/single_thread_task_executor.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

// Example window that contains the AI provider menu button
class ExampleWindow : public views::WidgetDelegateView {
 public:
  explicit ExampleWindow(browser_core::BrowserAIIntegration* ai_integration)
      : ai_integration_(ai_integration) {
    SetLayoutManager(std::make_unique<views::FillLayout>());
    
    // Create a toolbar
    auto* toolbar = AddChildView(std::make_unique<views::View>());
    toolbar->SetLayoutManager(std::make_unique<views::FillLayout>());
    
    // Add the AI provider menu button to the toolbar
    toolbar->AddChildView(
        std::make_unique<browser_core::ui::AIProviderMenuButton>(ai_integration_));
    
    SetPreferredSize(gfx::Size(400, 50));
  }
  
  // views::WidgetDelegate:
  bool CanResize() const override { return true; }
  std::u16string GetWindowTitle() const override {
    return u"AI Provider Menu Button Example";
  }
  
 private:
  browser_core::BrowserAIIntegration* ai_integration_;
};

// Example of using the AI Provider Menu Button in the browser
int main(int argc, char* argv[]) {
  // Initialize base
  base::AtExitManager exit_manager;
  base::CommandLine::Init(argc, argv);
  base::SingleThreadTaskExecutor task_executor;
  
  // Create the AI integration
  auto ai_integration = std::make_unique<browser_core::BrowserAIIntegration>();
  ai_integration->Initialize(nullptr, nullptr);
  
  // Create the example window
  auto example_window = std::make_unique<ExampleWindow>(ai_integration.get());
  
  // Create the widget
  views::Widget::InitParams params;
  params.delegate = example_window.get();
  params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  params.bounds = gfx::Rect(100, 100, 400, 50);
  
  auto widget = std::make_unique<views::Widget>();
  widget->Init(std::move(params));
  widget->Show();
  
  // Run the message loop
  base::RunLoop().Run();
  
  return 0;
}