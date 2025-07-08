// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "asol/adapters/claude/claude_text_adapter.h"

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
namespace claude {
namespace {

// Test fixture for ClaudeTextAdapter tests
class ClaudeTextAdapterTest : public testing::Test {
 protected:
  ClaudeTextAdapterTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    adapter_ = std::make_unique<ClaudeTextAdapter>("test_api_key");
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
        base::BindOnce(&ClaudeTextAdapterTest::OnResponseReceived,
                      base::Unretained(this),
                      success_out,
                      response_out,
                      run_loop.QuitClosure()));
    
    run_loop.Run();
  }

  // Helper method to run ProcessConversation and wait for the callback
  void RunProcessConversationAndWait(
      const std::vector<ClaudeMessage>& messages,
      bool* success_out,
      std::string* response_out) {
    base::RunLoop run_loop;
    
    adapter_->ProcessConversation(
        messages,
        base::BindOnce(&ClaudeTextAdapterTest::OnResponseReceived,
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
  std::unique_ptr<ClaudeTextAdapter> adapter_;
};

// Test basic initialization
TEST_F(ClaudeTextAdapterTest, Initialize) {
  EXPECT_NE(adapter_, nullptr);
  
  // Check default configuration
  const ClaudeRequestConfig& config = adapter_->GetRequestConfig();
  EXPECT_EQ(config.model_name, "claude-3-opus-20240229");
  EXPECT_FLOAT_EQ(config.temperature, 0.7f);
  EXPECT_EQ(config.max_tokens, 1024);
  EXPECT_EQ(config.anthropic_version, "2023-06-01");
}

// Test configuration setting
TEST_F(ClaudeTextAdapterTest, SetConfiguration) {
  ClaudeRequestConfig custom_config;
  custom_config.model_name = "claude-3-sonnet-20240229";
  custom_config.temperature = 0.3f;
  custom_config.max_tokens = 2048;
  custom_config.anthropic_version = "2023-12-01";
  
  adapter_->SetRequestConfig(custom_config);
  
  const ClaudeRequestConfig& config = adapter_->GetRequestConfig();
  EXPECT_EQ(config.model_name, "claude-3-sonnet-20240229");
  EXPECT_FLOAT_EQ(config.temperature, 0.3f);
  EXPECT_EQ(config.max_tokens, 2048);
  EXPECT_EQ(config.anthropic_version, "2023-12-01");
}

// Test API key setting
TEST_F(ClaudeTextAdapterTest, SetApiKey) {
  adapter_->SetApiKey("new_api_key");
  // We can't directly test the API key value as it's private,
  // but we can verify the adapter still works
  
  bool success = false;
  std::string response;
  RunProcessTextAndWait("Test prompt", &success, &response);
  
  EXPECT_TRUE(success);
  EXPECT_FALSE(response.empty());
}

// Test processing a single text prompt
TEST_F(ClaudeTextAdapterTest, ProcessText) {
  bool success = false;
  std::string response;
  RunProcessTextAndWait("What is the capital of France?", &success, &response);
  
  EXPECT_TRUE(success);
  EXPECT_FALSE(response.empty());
}

// Test processing a conversation
TEST_F(ClaudeTextAdapterTest, ProcessConversation) {
  std::vector<ClaudeMessage> messages;
  
  ClaudeMessage system_message;
  system_message.role = ClaudeMessage::Role::SYSTEM;
  system_message.content = "You are Claude, a helpful AI assistant.";
  
  ClaudeMessage user_message;
  user_message.role = ClaudeMessage::Role::USER;
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
TEST_F(ClaudeTextAdapterTest, BuildRequestPayload) {
  nlohmann::json payload = adapter_->BuildRequestPayload("Test prompt");
  
  EXPECT_TRUE(payload.contains("model"));
  EXPECT_TRUE(payload.contains("messages"));
  EXPECT_TRUE(payload.contains("system"));
  
  auto messages = payload["messages"];
  EXPECT_TRUE(messages.is_array());
  EXPECT_EQ(messages.size(), 1);  // Just user message (system is separate in Claude API)
  
  auto user_message = messages[0];
  EXPECT_TRUE(user_message.contains("role"));
  EXPECT_EQ(user_message["role"], "user");
  EXPECT_TRUE(user_message.contains("content"));
  EXPECT_EQ(user_message["content"], "Test prompt");
  
  EXPECT_TRUE(payload.contains("temperature"));
  EXPECT_TRUE(payload.contains("max_tokens"));
}

// Test request payload building for conversation
TEST_F(ClaudeTextAdapterTest, BuildConversationPayload) {
  std::vector<ClaudeMessage> messages;
  
  ClaudeMessage system_message;
  system_message.role = ClaudeMessage::Role::SYSTEM;
  system_message.content = "You are Claude, a helpful AI assistant.";
  
  ClaudeMessage user_message;
  user_message.role = ClaudeMessage::Role::USER;
  user_message.content = "Hello!";
  
  ClaudeMessage assistant_message;
  assistant_message.role = ClaudeMessage::Role::ASSISTANT;
  assistant_message.content = "Hi there! How can I help you today?";
  
  messages.push_back(system_message);
  messages.push_back(user_message);
  messages.push_back(assistant_message);
  
  nlohmann::json payload = adapter_->BuildConversationPayload(messages);
  
  EXPECT_TRUE(payload.contains("model"));
  EXPECT_TRUE(payload.contains("messages"));
  EXPECT_TRUE(payload.contains("system"));
  
  // Check that system message is extracted to the system field
  EXPECT_EQ(payload["system"], "You are Claude, a helpful AI assistant.");
  
  auto payload_messages = payload["messages"];
  EXPECT_TRUE(payload_messages.is_array());
  EXPECT_EQ(payload_messages.size(), 2);  // User and assistant (system is separate)
  
  auto user = payload_messages[0];
  EXPECT_TRUE(user.contains("role"));
  EXPECT_EQ(user["role"], "user");
  EXPECT_TRUE(user.contains("content"));
  EXPECT_EQ(user["content"], "Hello!");
  
  auto assistant = payload_messages[1];
  EXPECT_TRUE(assistant.contains("role"));
  EXPECT_EQ(assistant["role"], "assistant");
  EXPECT_TRUE(assistant.contains("content"));
  EXPECT_EQ(assistant["content"], "Hi there! How can I help you today?");
}

// Test role to string conversion
TEST_F(ClaudeTextAdapterTest, RoleToString) {
  EXPECT_EQ(adapter_->RoleToString(ClaudeMessage::Role::USER), "user");
  EXPECT_EQ(adapter_->RoleToString(ClaudeMessage::Role::ASSISTANT), "assistant");
  EXPECT_EQ(adapter_->RoleToString(ClaudeMessage::Role::SYSTEM), "system");
}

}  // namespace
}  // namespace claude
}  // namespace adapters
}  // namespace asol