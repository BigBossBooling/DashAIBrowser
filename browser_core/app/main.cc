// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/run_loop.h"
#include "base/task/single_thread_task_executor.h"
#include "browser_core/app/browser_main.h"

int main(int argc, char* argv[]) {
  // Initialize the CommandLine object.
  base::CommandLine::Init(argc, argv);
  
  // Set up the process.
  base::AtExitManager at_exit_manager;
  base::SingleThreadTaskExecutor main_task_executor;
  
  // Initialize logging.
  logging::LoggingSettings logging_settings;
  logging_settings.logging_dest = logging::LOG_TO_SYSTEM_DEBUG_LOG;
  logging::InitLogging(logging_settings);
  
  // Get command line arguments
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  
  // Create and initialize the browser
  browser_core::app::BrowserMain browser;
  browser_core::app::BrowserMain::StartupParams params;
  
  // Parse command line arguments
  if (command_line->HasSwitch("incognito")) {
    params.start_incognito = true;
  }
  
  if (command_line->HasSwitch("maximized")) {
    params.start_maximized = true;
  }
  
  if (command_line->HasSwitch("url")) {
    params.initial_url = command_line->GetSwitchValueASCII("url");
  } else {
    params.initial_url = "https://www.dashaibrowser.com";
  }
  
  if (command_line->HasSwitch("user-data-dir")) {
    params.user_data_dir = command_line->GetSwitchValueASCII("user-data-dir");
  }
  
  if (command_line->HasSwitch("disable-ai")) {
    params.enable_ai_features = false;
  }
  
  if (command_line->HasSwitch("disable-voice")) {
    params.enable_voice_commands = false;
  }
  
  if (command_line->HasSwitch("disable-research")) {
    params.enable_research_assistant = false;
  }
  
  if (command_line->HasSwitch("disable-advanced-security")) {
    params.enable_advanced_security = false;
  }
  
  // Initialize the browser
  if (!browser.Initialize(params)) {
    LOG(ERROR) << "Failed to initialize browser";
    return 1;
  }
  
  // Run the browser
  int result = browser.Run();
  
  // Shutdown the browser
  browser.Shutdown();
  
  return result;
}