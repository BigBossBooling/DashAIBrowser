// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "asol/adapters/gemini/gemini_text_adapter.h"

#include <memory>
#include <string>

#include "asol/test/mock_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/nlohmann_json/json.hpp"

namespace asol {
namespace adapters {
namespace gemini {
namespace {

class GeminiTextAdapterTest : public testing::Test {
 protected:
  void SetUp() override {
    // Create a mock URL loader factory
    mock_url_loader_factory_ = std::make_unique<test::MockUrlLoaderFactory>();
    
    // Create a sample successful response
    nlohmann::json response_json;
    response_json["candidates"] = nlohmann::json::array();
    nlohmann::json candidate;
    candidate["content"]["parts"] = nlohmann::json::array();
    candidate["content"]["parts"][0]["text"] = "This is a test response from the mock Gemini API.";
    response_json["candidates"].push_back(candidate);
    response_json["usage"]["promptTokenCount"] = 10;
    response_json["usage"]["candidatesTokenCount"] = 20;
    response_json["usage"]["totalTokenCount"] = 30;
    response_json["model"] = "gemini-pro";
    
    // Add the response to the mock factory
    mock_url_loader_factory_->AddResponseForUrlPrefix(
        "https://generativelanguage.googleapis.com/",
        response_json.dump());
    
    // Create the adapter with the mock factory
    adapter_ = std::make_unique<GeminiTextAdapter>(
        mock_url_loader_factory_->GetSharedFactory());
  }

  std::unique_ptr<GeminiTextAdapter> adapter_;
  std::unique_ptr<test::MockUrlLoaderFactory> mock_url_loader_factory_;
};

TEST_F(GeminiTextAdapterTest, InitializeWithValidConfig) {
  // Create a valid configuration
  nlohmann::json config;
  config["api_key"] = "test_api_key";
  config["model_name"] = "gemini-pro-test";
  config["temperature"] = 0.5;
  config["max_output_tokens"] = 100;
  
  // Initialize the adapter
  EXPECT_TRUE(adapter_->Initialize(config.dump()));
  
  // Verify the adapter is ready
  EXPECT_TRUE(adapter_->IsReady());
  
  // Verify the model name
  EXPECT_EQ(adapter_->GetModelName(), "gemini-pro-test");
}

TEST_F(GeminiTextAdapterTest, InitializeWithInvalidConfig) {
  // Create an invalid configuration (missing required api_key)
  nlohmann::json config;
  config["model_name"] = "gemini-pro-test";
  
  // Initialize should fail
  EXPECT_FALSE(adapter_->Initialize(config.dump()));
  
  // Adapter should not be ready
  EXPECT_FALSE(adapter_->IsReady());
}

TEST_F(GeminiTextAdapterTest, GetCapabilities) {
  // Verify the adapter reports the expected capabilities
  std::vector<std::string> capabilities = adapter_->GetCapabilities();
  
  // Check that the adapter supports text generation
  EXPECT_NE(std::find(capabilities.begin(), capabilities.end(), "text-generation"),
            capabilities.end());
}

TEST_F(GeminiTextAdapterTest, ProcessTextWithoutInitialization) {
  // Attempt to process text without initialization
  ModelResponse response = adapter_->ProcessText("Test input");
  
  // Verify the response indicates failure
  EXPECT_FALSE(response.success);
  EXPECT_FALSE(response.error_message.empty());
}

TEST_F(GeminiTextAdapterTest, ProcessTextWithInitialization) {
  // Initialize the adapter
  nlohmann::json config;
  config["api_key"] = "test_api_key";
  EXPECT_TRUE(adapter_->Initialize(config.dump()));
  
  // Process text
  ModelResponse response = adapter_->ProcessText("Test input");
  
  // In our mock implementation, this should succeed
  EXPECT_TRUE(response.success);
  EXPECT_FALSE(response.text.empty());
  
  // Check that metadata includes the adapter name
  bool has_adapter_info = false;
  for (const auto& meta : response.metadata) {
    if (meta.first == "adapter" && meta.second == "Gemini") {
      has_adapter_info = true;
      break;
    }
  }
  EXPECT_TRUE(has_adapter_info);
}

}  // namespace
}  // namespace gemini
}  // namespace adapters
}  // namespace asol