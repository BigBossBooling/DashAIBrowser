// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iostream>
#include <memory>
#include <string>

#include "asol/adapters/gemini/gemini_text_adapter.h"
#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/logging.h"
#include "base/run_loop.h"
#include "base/task/single_thread_task_executor.h"

// Callback function to handle Gemini API responses
void OnGeminiResponse(base::OnceClosure quit_closure,
                     bool success,
                     const std::string& response) {
  if (success) {
    std::cout << "Gemini API Response:" << std::endl;
    std::cout << "-------------------" << std::endl;
    std::cout << response << std::endl;
    std::cout << "-------------------" << std::endl;
  } else {
    std::cerr << "Error: " << response << std::endl;
  }
  
  // Signal that we're done
  std::move(quit_closure).Run();
}

int main(int argc, char* argv[]) {
  // Initialize the CommandLine object.
  base::CommandLine::Init(argc, argv);
  
  // Set up the process.
  base::AtExitManager at_exit_manager;
  base::SingleThreadTaskExecutor main_task_executor;
  
  // Get command line arguments
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  
  // Check for API key
  std::string api_key;
  if (command_line->HasSwitch("api-key")) {
    api_key = command_line->GetSwitchValueASCII("api-key");
  } else {
    std::cerr << "Error: API key is required. Use --api-key=YOUR_API_KEY" << std::endl;
    return 1;
  }
  
  // Check for prompt
  std::string prompt;
  if (command_line->HasSwitch("prompt")) {
    prompt = command_line->GetSwitchValueASCII("prompt");
  } else {
    std::cerr << "Error: Prompt is required. Use --prompt=\"Your prompt here\"" << std::endl;
    return 1;
  }
  
  // Create the Gemini adapter
  auto adapter = std::make_unique<asol::adapters::gemini::GeminiTextAdapter>(api_key);
  
  // Configure the adapter (optional)
  if (command_line->HasSwitch("temperature")) {
    std::string temp_str = command_line->GetSwitchValueASCII("temperature");
    float temperature = std::stof(temp_str);
    
    asol::adapters::gemini::GeminiRequestConfig config = adapter->GetRequestConfig();
    config.temperature = temperature;
    adapter->SetRequestConfig(config);
    
    std::cout << "Set temperature to: " << temperature << std::endl;
  }
  
  // Create a RunLoop to wait for the async operation to complete
  base::RunLoop run_loop;
  
  std::cout << "Sending prompt to Gemini API: " << prompt << std::endl;
  
  // Process the text with the Gemini API
  adapter->ProcessText(
      prompt,
      base::BindOnce(&OnGeminiResponse, run_loop.QuitClosure()));
  
  // Wait for the response
  run_loop.Run();
  
  return 0;
}