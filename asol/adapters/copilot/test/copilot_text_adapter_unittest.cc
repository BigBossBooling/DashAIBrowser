// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "asol/adapters/copilot/copilot_text_adapter.h"

#include <memory>
#include <string>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/run_loop.h"
#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/nlohmann_json/json.hpp"

namespace asol {
namespace adapters {
namespace copilot {
namespace {

// Test fixture for CopilotTextAdapter tests
class CopilotTextAdapterTest : public testing::Test {
 protected:
  CopilotTextAdapterTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    adapter_ = std::make_unique<CopilotTextAdapter>("test_api_key");
  }

  void TearDown() override {
    adapter_.reset();
  }

  // Helper method to run ProcessText and wait for the callback
  void RunProcessTextAndWait(const std::string& input,
                            bool* success_out,
                            std::string* response_out) {
    base::RunLoop run_loop;
    
    adapter_->ProcessText(
        input,
        base::BindOnce(&CopilotTextAdapterTest::OnResponseReceived,
                      base::Unretained(this),
                      success_out,
                      response_out,
                      run_loop.QuitClosure()));
    
    run_loop.Run();
  }

  // Helper method to run ProcessConversation and wait for the callback
  void RunProcessConversationAndWait(
      const std::vector<CopilotMessage>& messages,
      bool* success_out,
      std::string* response_out) {
    base::RunLoop run_loop;
    
    adapter_->ProcessConversation(
        messages,
        base::BindOnce(&CopilotTextAdapterTest::OnResponseReceived,
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
  std::unique_ptr<CopilotTextAdapter> adapter_;
};

// Test basic initialization
TEST_F(CopilotTextAdapterTest, Initialize) {
  EXPECT_NE(adapter_, nullptr);
  
  // Check default configuration
  const CopilotRequestConfig& config = adapter_->GetRequestConfig();
  EXPECT_EQ(config.model_name, "copilot-4");
  EXPECT_FLOAT_EQ(config.temperature, 0.7f);
  EXPECT_EQ(config.max_tokens, 1024);
  EXPECT_EQ(config.api_version, "2023-12-01-preview");
}

// Test configuration setting
TEST_F(CopilotTextAdapterTest, SetConfiguration) {
  CopilotRequestConfig custom_config;
  custom_config.model_name = "copilot-3.5";
  custom_config.temperature = 0.3f;
  custom_config.max_tokens = 2048;
  custom_config.api_version = "2024-01-01-preview";
  
  adapter_->SetRequestConfig(custom_config);
  
  const CopilotRequestConfig& config = adapter_->GetRequestConfig();
  EXPECT_EQ(config.model_name, "copilot-3.5");
  EXPECT_FLOAT_EQ(config.temperature, 0.3f);
  EXPECT_EQ(config.max_tokens, 2048);
  EXPECT_EQ(config.api_version, "2024-01-01-preview");
}

// Test API key setting
TEST_F(CopilotTextAdapterTest, SetApiKey) {
  adapter_->SetApiKey("new_api_key");
  // We can't directly test the API key value as it's private,
  // but we can verify the adapter still works
  
  bool success = false;
  std::string response;
  RunProcessTextAndWait("Test prompt", &success, &response);
  
  EXPECT_TRUE(success);
  EXPECT_FALSE(response.empty());
}

// Test endpoint setting
TEST_F(CopilotTextAdapterTest, SetEndpoint) {
  adapter_->SetEndpoint("https://custom-endpoint.microsoft.com/copilot/v1/chat/completions");
  
  bool success = false;
  std::string response;
  RunProcessTextAndWait("Test prompt", &success, &response);
  
  EXPECT_TRUE(success);
  EXPECT_FALSE(response.empty());
}

// Test processing a single text prompt
TEST_F(CopilotTextAdapterTest, ProcessText) {
  bool success = false;
  std::string response;
  RunProcessTextAndWait("What is the capital of France?", &success, &response);
  
  EXPECT_TRUE(success);
  EXPECT_FALSE(response.empty());
}

// Test processing a conversation
TEST_F(CopilotTextAdapterTest, ProcessConversation) {
  std::vector<CopilotMessage> messages;
  
  CopilotMessage system_message;
  system_message.role = CopilotMessage::Role::SYSTEM;
  system_message.content = "You are Microsoft Copilot, a helpful assistant.";
  
  CopilotMessage user_message;
  user_message.role = CopilotMessage::Role::USER;
  user_message.content = "Tell me about quantum computing.";
  
  messages.push_back(system_message);
  messages.push_back(user_message);
  
  bool success = false;
  std::string response;
  RunProcessConversationAndWait(messages, &success, &response);
  
  EXPECT_TRUE(success);
  EXPECT_FALSE(response.empty());
}

// Test request payload building for single text
TEST_F(CopilotTextAdapterTest, BuildRequestPayload) {
  nlohmann::json payload = adapter_->BuildRequestPayload("Test prompt");
  
  EXPECT_TRUE(payload.contains("model"));
  EXPECT_TRUE(payload.contains("messages"));
  
  auto messages = payload["messages"];
  EXPECT_TRUE(messages.is_array());
  EXPECT_EQ(messages.size(), 2);  // System message + user message
  
  auto user_message = messages[1];
  EXPECT_TRUE(user_message.contains("role"));
  EXPECT_EQ(user_message["role"], "user");
  EXPECT_TRUE(user_message.contains("content"));
  EXPECT_EQ(user_message["content"], "Test prompt");
  
  EXPECT_TRUE(payload.contains("temperature"));
  EXPECT_TRUE(payload.contains("max_tokens"));
  EXPECT_TRUE(payload.contains("api-version"));
}

// Test request payload building for conversation
TEST_F(CopilotTextAdapterTest, BuildConversationPayload) {
  std::vector<CopilotMessage> messages;
  
  CopilotMessage system_message;
  system_message.role = CopilotMessage::Role::SYSTEM;
  system_message.content = "You are Microsoft Copilot, a helpful assistant.";
  
  CopilotMessage user_message;
  user_message.role = CopilotMessage::Role::USER;
  user_message.content = "Hello!";
  
  messages.push_back(system_message);
  messages.push_back(user_message);
  
  nlohmann::json payload = adapter_->BuildConversationPayload(messages);
  
  EXPECT_TRUE(payload.contains("model"));
  EXPECT_TRUE(payload.contains("messages"));
  
  auto payload_messages = payload["messages"];
  EXPECT_TRUE(payload_messages.is_array());
  EXPECT_EQ(payload_messages.size(), 2);
  
  auto system = payload_messages[0];
  EXPECT_TRUE(system.contains("role"));
  EXPECT_EQ(system["role"], "system");
  
  auto user = payload_messages[1];
  EXPECT_TRUE(user.contains("role"));
  EXPECT_EQ(user["role"], "user");
  EXPECT_TRUE(user.contains("content"));
  EXPECT_EQ(user["content"], "Hello!");
}

// Test role to string conversion
TEST_F(CopilotTextAdapterTest, RoleToString) {
  EXPECT_EQ(adapter_->RoleToString(CopilotMessage::Role::USER), "user");
  EXPECT_EQ(adapter_->RoleToString(CopilotMessage::Role::ASSISTANT), "assistant");
  EXPECT_EQ(adapter_->RoleToString(CopilotMessage::Role::SYSTEM), "system");
}

}  // namespace
}  // namespace copilot
}  // namespace adapters
}  // namespace asol