// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "asol/core/service_manager.h"

#include <memory>
#include <string>

#include "asol/adapters/adapter_interface.h"
#include "asol/adapters/gemini/gemini_text_adapter.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/nlohmann_json/json.hpp"

namespace asol {
namespace core {
namespace {

class ServiceManagerTest : public testing::Test {
 protected:
  void SetUp() override {
    service_manager_ = ServiceManager::GetInstance();
    
    // Register a Gemini adapter for testing
    auto gemini_adapter = std::make_unique<adapters::gemini::GeminiTextAdapter>();
    service_manager_->RegisterAdapter("gemini", std::move(gemini_adapter));
    
    // Initialize the adapter
    nlohmann::json config;
    config["adapters"]["gemini"]["api_key"] = "test_api_key";
    service_manager_->InitializeAdapters(config.dump());
  }

  ServiceManager* service_manager_;
};

TEST_F(ServiceManagerTest, GetAdapter) {
  // Get the registered adapter
  adapters::AdapterInterface* adapter = service_manager_->GetAdapter("gemini");
  EXPECT_NE(adapter, nullptr);
  EXPECT_EQ(adapter->GetName(), "Gemini");
  
  // Try to get a non-existent adapter
  adapter = service_manager_->GetAdapter("nonexistent");
  EXPECT_EQ(adapter, nullptr);
}

TEST_F(ServiceManagerTest, FindAdaptersByCapability) {
  // Find adapters that support text generation
  std::vector<std::string> adapters = 
      service_manager_->FindAdaptersByCapability("text-generation");
  
  // Verify that the Gemini adapter is found
  EXPECT_FALSE(adapters.empty());
  EXPECT_NE(std::find(adapters.begin(), adapters.end(), "gemini"), adapters.end());
  
  // Try to find adapters for a non-existent capability
  adapters = service_manager_->FindAdaptersByCapability("nonexistent-capability");
  EXPECT_TRUE(adapters.empty());
}

TEST_F(ServiceManagerTest, ProcessText) {
  // Process text with the Gemini adapter
  adapters::ModelResponse response = 
      service_manager_->ProcessText("gemini", "Test input");
  
  // Verify the response
  EXPECT_TRUE(response.success);
  EXPECT_FALSE(response.text.empty());
  
  // Try to process text with a non-existent adapter
  response = service_manager_->ProcessText("nonexistent", "Test input");
  EXPECT_FALSE(response.success);
  EXPECT_FALSE(response.error_message.empty());
}

TEST_F(ServiceManagerTest, ProcessTextWithCapability) {
  // Process text with an adapter that supports text generation
  adapters::ModelResponse response = 
      service_manager_->ProcessTextWithCapability("text-generation", "Test input");
  
  // Verify the response
  EXPECT_TRUE(response.success);
  EXPECT_FALSE(response.text.empty());
  
  // Try to process text with a non-existent capability
  response = service_manager_->ProcessTextWithCapability("nonexistent-capability", "Test input");
  EXPECT_FALSE(response.success);
  EXPECT_FALSE(response.error_message.empty());
}

}  // namespace
}  // namespace core
}  // namespace asol