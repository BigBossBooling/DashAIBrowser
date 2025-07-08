// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

#include "asol/adapters/adapter_factory.h"
#include "asol/core/ai_service_provider.h"
#include "asol/core/multi_adapter_manager.h"
#include "browser_core/ui/ai_settings_page.h"
#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/run_loop.h"
#include "base/task/single_thread_task_executor.h"

// Example of using the AI Settings Page in the browser
int main(int argc, char* argv[]) {
  // Initialize base
  base::AtExitManager exit_manager;
  base::CommandLine::Init(argc, argv);
  base::SingleThreadTaskExecutor task_executor;
  
  // Create configuration
  std::unordered_map<std::string, std::string> config;
  
  // Add API keys for each provider
  // In a real application, these would be loaded from secure storage
  config["gemini_api_key"] = "GEMINI_API_KEY";
  config["openai_api_key"] = "OPENAI_API_KEY";
  config["copilot_api_key"] = "COPILOT_API_KEY";
  config["claude_api_key"] = "CLAUDE_API_KEY";
  
  // Set default provider
  config["default_provider"] = "gemini";
  
  // Create the multi-adapter manager
  auto adapter_manager = asol::adapters::AdapterFactory::CreateMultiAdapterManager(config);
  
  // Create the AI settings page
  auto settings_page = std::make_unique<browser_core::ui::AISettingsPage>(adapter_manager.get());
  
  // Initialize and show the settings page
  settings_page->Initialize();
  settings_page->Show();
  
  // Simulate user interaction: change the provider
  std::cout << "\nSimulating user selecting OpenAI provider...\n" << std::endl;
  settings_page->provider_selector_->SelectProvider("openai");
  
  // Simulate user interaction: change a configuration value
  std::cout << "\nSimulating user changing temperature to 0.5...\n" << std::endl;
  settings_page->SetConfigValue("temperature", "0.5");
  
  // Apply the settings
  std::cout << "\nSimulating user clicking 'Apply'...\n" << std::endl;
  settings_page->ApplySettings();
  
  // Create a run loop for async operations
  base::RunLoop run_loop;
  
  // Example request parameters
  asol::core::AIRequestParams params;
  params.task_type = asol::core::AIServiceProvider::TaskType::TEXT_GENERATION;
  params.input_text = "What are the benefits of using multiple AI models in a browser?";
  
  // Process the request with the active provider (now OpenAI with temperature 0.5)
  std::cout << "\nSending request to OpenAI provider with updated settings...\n" << std::endl;
  adapter_manager->ProcessRequest(
      params,
      base::BindOnce([](base::OnceClosure quit_closure, bool success, const std::string& response) {
        if (success) {
          std::cout << "Response from OpenAI provider:" << std::endl;
          std::cout << response << std::endl;
        } else {
          std::cout << "Error: " << response << std::endl;
        }
        std::move(quit_closure).Run();
      }, run_loop.QuitClosure()));
  
  // Run the loop to wait for the response
  run_loop.Run();
  
  // Simulate user interaction: reset to defaults
  std::cout << "\nSimulating user clicking 'Reset to Defaults'...\n" << std::endl;
  settings_page->ResetToDefaults();
  
  // Hide the settings page
  std::cout << "\nSimulating user closing the settings page...\n" << std::endl;
  settings_page->Hide();
  
  return 0;
}