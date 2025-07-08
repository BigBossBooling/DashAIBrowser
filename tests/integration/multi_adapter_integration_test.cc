// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>
#include <unordered_map>

#include "asol/adapters/adapter_factory.h"
#include "asol/core/ai_service_provider.h"
#include "asol/core/multi_adapter_manager.h"
#include "base/functional/bind.h"
#include "base/run_loop.h"
#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace asol {
namespace {

class MultiAdapterIntegrationTest : public testing::Test {
 protected:
  MultiAdapterIntegrationTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    // Create configuration with test API keys
    std::unordered_map<std::string, std::string> config;
    config["gemini_api_key"] = "test_gemini_api_key";
    config["openai_api_key"] = "test_openai_api_key";
    config["copilot_api_key"] = "test_copilot_api_key";
    config["claude_api_key"] = "test_claude_api_key";
    config["default_provider"] = "gemini";
    
    // Create the multi-adapter manager
    adapter_manager_ = adapters::AdapterFactory::CreateMultiAdapterManager(config);
  }

  void TearDown() override {
    adapter_manager_.reset();
  }

  // Helper method to run ProcessRequest and wait for the callback
  void RunProcessRequestAndWait(
      const core::AIRequestParams& params,
      bool* success_out,
      std::string* response_out) {
    base::RunLoop run_loop;
    
    adapter_manager_->ProcessRequest(
        params,
        base::BindOnce(&MultiAdapterIntegrationTest::OnResponseReceived,
                      base::Unretained(this),
                      success_out,
                      response_out,
                      run_loop.QuitClosure()));
    
    run_loop.Run();
  }

  // Helper method to run ProcessRequestWithProvider and wait for the callback
  void RunProcessRequestWithProviderAndWait(
      const std::string& provider_id,
      const core::AIRequestParams& params,
      bool* success_out,
      std::string* response_out) {
    base::RunLoop run_loop;
    
    adapter_manager_->ProcessRequestWithProvider(
        provider_id,
        params,
        base::BindOnce(&MultiAdapterIntegrationTest::OnResponseReceived,
                      base::Unretained(this),
                      success_out,
                      response_out,
                      run_loop.QuitClosure()));
    
    run_loop.Run();
  }

  // Callback for response handling in tests
  void OnResponseReceived(bool* success_out,
                         std::string* response_out,
                         base::OnceClosure quit_closure,
                         bool success,
                         const std::string& response) {
    *success_out = success;
    *response_out = response;
    std::move(quit_closure).Run();
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<core::MultiAdapterManager> adapter_manager_;
};

// Test that all providers are registered
TEST_F(MultiAdapterIntegrationTest, ProvidersRegistered) {
  auto provider_ids = adapter_manager_->GetRegisteredProviderIds();
  
  // Check that all expected providers are registered
  EXPECT_TRUE(std::find(provider_ids.begin(), provider_ids.end(), "gemini") != provider_ids.end());
  EXPECT_TRUE(std::find(provider_ids.begin(), provider_ids.end(), "openai") != provider_ids.end());
  EXPECT_TRUE(std::find(provider_ids.begin(), provider_ids.end(), "copilot") != provider_ids.end());
  EXPECT_TRUE(std::find(provider_ids.begin(), provider_ids.end(), "claude") != provider_ids.end());
}

// Test that the default provider is set correctly
TEST_F(MultiAdapterIntegrationTest, DefaultProviderSet) {
  EXPECT_EQ(adapter_manager_->GetActiveProviderId(), "gemini");
}

// Test switching between providers
TEST_F(MultiAdapterIntegrationTest, SwitchProvider) {
  // Switch to OpenAI
  EXPECT_TRUE(adapter_manager_->SetActiveProvider("openai"));
  EXPECT_EQ(adapter_manager_->GetActiveProviderId(), "openai");
  
  // Switch to Copilot
  EXPECT_TRUE(adapter_manager_->SetActiveProvider("copilot"));
  EXPECT_EQ(adapter_manager_->GetActiveProviderId(), "copilot");
  
  // Switch to Claude
  EXPECT_TRUE(adapter_manager_->SetActiveProvider("claude"));
  EXPECT_EQ(adapter_manager_->GetActiveProviderId(), "claude");
  
  // Switch back to Gemini
  EXPECT_TRUE(adapter_manager_->SetActiveProvider("gemini"));
  EXPECT_EQ(adapter_manager_->GetActiveProviderId(), "gemini");
  
  // Try to switch to an invalid provider
  EXPECT_FALSE(adapter_manager_->SetActiveProvider("invalid_provider"));
  EXPECT_EQ(adapter_manager_->GetActiveProviderId(), "gemini");  // Should remain unchanged
}

// Test processing a request with the active provider
TEST_F(MultiAdapterIntegrationTest, ProcessRequestWithActiveProvider) {
  core::AIRequestParams params;
  params.task_type = core::AIServiceProvider::TaskType::TEXT_GENERATION;
  params.input_text = "Test prompt";
  
  bool success = false;
  std::string response;
  RunProcessRequestAndWait(params, &success, &response);
  
  EXPECT_TRUE(success);
  EXPECT_FALSE(response.empty());
}

// Test processing a request with a specific provider
TEST_F(MultiAdapterIntegrationTest, ProcessRequestWithSpecificProvider) {
  core::AIRequestParams params;
  params.task_type = core::AIServiceProvider::TaskType::TEXT_GENERATION;
  params.input_text = "Test prompt";
  
  // Test with each provider
  for (const auto& provider_id : {"gemini", "openai", "copilot", "claude"}) {
    bool success = false;
    std::string response;
    RunProcessRequestWithProviderAndWait(provider_id, params, &success, &response);
    
    EXPECT_TRUE(success) << "Failed with provider: " << provider_id;
    EXPECT_FALSE(response.empty()) << "Empty response from provider: " << provider_id;
  }
}

// Test finding the best provider for a task
TEST_F(MultiAdapterIntegrationTest, FindBestProviderForTask) {
  // All providers should support text generation
  std::string best_provider = adapter_manager_->FindBestProviderForTask(
      core::AIServiceProvider::TaskType::TEXT_GENERATION);
  
  // Should return the active provider if it supports the task
  EXPECT_EQ(best_provider, adapter_manager_->GetActiveProviderId());
}

// Test configuring a provider
TEST_F(MultiAdapterIntegrationTest, ConfigureProvider) {
  std::unordered_map<std::string, std::string> config;
  config["model"] = "gpt-4-turbo";
  config["temperature"] = "0.5";
  
  EXPECT_TRUE(adapter_manager_->ConfigureProvider("openai", config));
  
  auto provider_config = adapter_manager_->GetProviderConfiguration("openai");
  EXPECT_EQ(provider_config["model"], "gpt-4-turbo");
  EXPECT_EQ(provider_config["temperature"], "0.5");
}

}  // namespace
}  // namespace asol