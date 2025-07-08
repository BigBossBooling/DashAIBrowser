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
#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/run_loop.h"
#include "base/task/single_thread_task_executor.h"

// Example of using the MultiAdapterManager to interact with multiple AI providers
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
  
  // Print available providers
  std::cout << "Available AI providers:" << std::endl;
  for (const auto& provider_id : adapter_manager->GetRegisteredProviderIds()) {
    auto provider = adapter_manager->GetProvider(provider_id);
    std::cout << "- " << provider->GetProviderName() 
              << " (ID: " << provider_id << ")" << std::endl;
  }
  
  // Print active provider
  std::cout << "Active provider: " << adapter_manager->GetActiveProviderId() << std::endl;
  
  // Create a run loop for async operations
  base::RunLoop run_loop;
  
  // Example request parameters
  asol::core::AIRequestParams params;
  params.task_type = asol::core::AIServiceProvider::TaskType::TEXT_GENERATION;
  params.input_text = "What are the benefits of using multiple AI models in a browser?";
  
  // Process the request with the active provider
  adapter_manager->ProcessRequest(
      params,
      base::BindOnce([](base::OnceClosure quit_closure, bool success, const std::string& response) {
        if (success) {
          std::cout << "Response from active provider:" << std::endl;
          std::cout << response << std::endl;
        } else {
          std::cout << "Error: " << response << std::endl;
        }
        std::move(quit_closure).Run();
      }, run_loop.QuitClosure()));
  
  // Run the loop to wait for the response
  run_loop.Run();
  
  // Create a new run loop for the next request
  base::RunLoop run_loop2;
  
  // Switch to a different provider
  std::string new_provider = "openai";
  if (adapter_manager->SetActiveProvider(new_provider)) {
    std::cout << "Switched to provider: " << new_provider << std::endl;
    
    // Process the same request with the new provider
    adapter_manager->ProcessRequest(
        params,
        base::BindOnce([](base::OnceClosure quit_closure, bool success, const std::string& response) {
          if (success) {
            std::cout << "Response from new provider:" << std::endl;
            std::cout << response << std::endl;
          } else {
            std::cout << "Error: " << response << std::endl;
          }
          std::move(quit_closure).Run();
        }, run_loop2.QuitClosure()));
    
    // Run the loop to wait for the response
    run_loop2.Run();
  } else {
    std::cout << "Failed to switch provider." << std::endl;
  }
  
  // Example of using a specific provider without changing the active one
  base::RunLoop run_loop3;
  
  std::cout << "Using Claude provider directly:" << std::endl;
  adapter_manager->ProcessRequestWithProvider(
      "claude",
      params,
      base::BindOnce([](base::OnceClosure quit_closure, bool success, const std::string& response) {
        if (success) {
          std::cout << "Response from Claude:" << std::endl;
          std::cout << response << std::endl;
        } else {
          std::cout << "Error: " << response << std::endl;
        }
        std::move(quit_closure).Run();
      }, run_loop3.QuitClosure()));
  
  // Run the loop to wait for the response
  run_loop3.Run();
  
  return 0;
}