// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "asol/adapters/openai/openai_text_adapter.h"

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
namespace openai {
namespace {

// Test fixture for OpenAITextAdapter tests
class OpenAITextAdapterTest : public testing::Test {
 protected:
  OpenAITextAdapterTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    adapter_ = std::make_unique<OpenAITextAdapter>("test_api_key");
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
        base::BindOnce(&OpenAITextAdapterTest::OnResponseReceived,
                      base::Unretained(this),
                      success_out,
                      response_out,
                      run_loop.QuitClosure()));
    
    run_loop.Run();
  }

  // Helper method to run ProcessConversation and wait for the callback
  void RunProcessConversationAndWait(
      const std::vector<OpenAIMessage>& messages,
      bool* success_out,
      std::string* response_out) {
    base::RunLoop run_loop;
    
    adapter_->ProcessConversation(
        messages,
        base::BindOnce(&OpenAITextAdapterTest::OnResponseReceived,
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
  std::unique_ptr<OpenAITextAdapter> adapter_;
};

// Test basic initialization
TEST_F(OpenAITextAdapterTest, Initialize) {
  EXPECT_NE(adapter_, nullptr);
  
  // Check default configuration
  const OpenAIRequestConfig& config = adapter_->GetRequestConfig();
  EXPECT_EQ(config.model_name, "gpt-4o");
  EXPECT_FLOAT_EQ(config.temperature, 0.7f);
  EXPECT_EQ(config.max_tokens, 1024);
}

// Test configuration setting
TEST_F(OpenAITextAdapterTest, SetConfiguration) {
  OpenAIRequestConfig custom_config;
  custom_config.model_name = "gpt-3.5-turbo";
  custom_config.temperature = 0.3f;
  custom_config.max_tokens = 2048;
  
  adapter_->SetRequestConfig(custom_config);
  
  const OpenAIRequestConfig& config = adapter_->GetRequestConfig();
  EXPECT_EQ(config.model_name, "gpt-3.5-turbo");
  EXPECT_FLOAT_EQ(config.temperature, 0.3f);
  EXPECT_EQ(config.max_tokens, 2048);
}

// Test API key setting
TEST_F(OpenAITextAdapterTest, SetApiKey) {
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
TEST_F(OpenAITextAdapterTest, ProcessText) {
  bool success = false;
  std::string response;
  RunProcessTextAndWait("What is the capital of France?", &success, &response);
  
  EXPECT_TRUE(success);
  EXPECT_FALSE(response.empty());
}

// Test processing a conversation
TEST_F(OpenAITextAdapterTest, ProcessConversation) {
  std::vector<OpenAIMessage> messages;
  
  OpenAIMessage system_message;
  system_message.role = OpenAIMessage::Role::SYSTEM;
  system_message.content = "You are a helpful assistant.";
  
  OpenAIMessage user_message;
  user_message.role = OpenAIMessage::Role::USER;
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
TEST_F(OpenAITextAdapterTest, BuildRequestPayload) {
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
}

// Test request payload building for conversation
TEST_F(OpenAITextAdapterTest, BuildConversationPayload) {
  std::vector<OpenAIMessage> messages;
  
  OpenAIMessage system_message;
  system_message.role = OpenAIMessage::Role::SYSTEM;
  system_message.content = "You are a helpful assistant.";
  
  OpenAIMessage user_message;
  user_message.role = OpenAIMessage::Role::USER;
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
TEST_F(OpenAITextAdapterTest, RoleToString) {
  EXPECT_EQ(adapter_->RoleToString(OpenAIMessage::Role::USER), "user");
  EXPECT_EQ(adapter_->RoleToString(OpenAIMessage::Role::ASSISTANT), "assistant");
  EXPECT_EQ(adapter_->RoleToString(OpenAIMessage::Role::SYSTEM), "system");
}

}  // namespace
}  // namespace openai
}  // namespace adapters
}  // namespace asol