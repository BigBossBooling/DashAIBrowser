// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASOL_ADAPTERS_ADAPTER_FACTORY_H_
#define ASOL_ADAPTERS_ADAPTER_FACTORY_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "asol/core/ai_service_provider.h"
#include "asol/core/multi_adapter_manager.h"

namespace asol {
namespace adapters {

// AdapterFactory creates and initializes AI service providers
class AdapterFactory {
 public:
  // Create a new MultiAdapterManager with all available adapters
  static std::unique_ptr<core::MultiAdapterManager> CreateMultiAdapterManager(
      const std::unordered_map<std::string, std::string>& config);
  
  // Create a specific adapter by ID
  static std::unique_ptr<core::AIServiceProvider> CreateAdapter(
      const std::string& adapter_id,
      const std::unordered_map<std::string, std::string>& config);
  
  // Get the list of all supported adapter IDs
  static std::vector<std::string> GetSupportedAdapterIds();
  
  // Get the list of all supported adapter names
  static std::vector<std::string> GetSupportedAdapterNames();
  
  // Check if an adapter ID is supported
  static bool IsAdapterSupported(const std::string& adapter_id);
  
 private:
  // Create a Gemini adapter
  static std::unique_ptr<core::AIServiceProvider> CreateGeminiAdapter(
      const std::unordered_map<std::string, std::string>& config);
  
  // Create an OpenAI adapter
  static std::unique_ptr<core::AIServiceProvider> CreateOpenAIAdapter(
      const std::unordered_map<std::string, std::string>& config);
  
  // Create a Microsoft Copilot adapter
  static std::unique_ptr<core::AIServiceProvider> CreateCopilotAdapter(
      const std::unordered_map<std::string, std::string>& config);
  
  // Create a Claude adapter
  static std::unique_ptr<core::AIServiceProvider> CreateClaudeAdapter(
      const std::unordered_map<std::string, std::string>& config);
};

}  // namespace adapters
}  // namespace asol

#endif  // ASOL_ADAPTERS_ADAPTER_FACTORY_H_