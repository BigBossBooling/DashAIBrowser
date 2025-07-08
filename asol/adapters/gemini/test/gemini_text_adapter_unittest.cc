// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "asol/adapters/gemini/gemini_text_adapter.h"

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
namespace gemini {
namespace {

// Test fixture for GeminiTextAdapter tests
class GeminiTextAdapterTest : public testing::Test {
 protected:
  GeminiTextAdapterTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    adapter_ = std::make_unique<GeminiTextAdapter>("test_api_key");
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
        base::BindOnce(&GeminiTextAdapterTest::OnResponseReceived,
                      base::Unretained(this),
                      success_out,
                      response_out,
                      run_loop.QuitClosure()));
    
    run_loop.Run();
  }

  // Helper method to run ProcessConversation and wait for the callback
  void RunProcessConversationAndWait(
      const std::vector<GeminiMessage>& messages,
      bool* success_out,
      std::string* response_out) {
    base::RunLoop run_loop;
    
    adapter_->ProcessConversation(
        messages,
        base::BindOnce(&GeminiTextAdapterTest::OnResponseReceived,
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
  std::unique_ptr<GeminiTextAdapter> adapter_;
};

// Test basic initialization
TEST_F(GeminiTextAdapterTest, Initialize) {
  EXPECT_NE(adapter_, nullptr);
  
  // Check default configuration
  const GeminiRequestConfig& config = adapter_->GetRequestConfig();
  EXPECT_EQ(config.model_name, "gemini-pro");
  EXPECT_FLOAT_EQ(config.temperature, 0.7f);
  EXPECT_EQ(config.max_output_tokens, 1024);
}

// Test configuration setting
TEST_F(GeminiTextAdapterTest, SetConfiguration) {
  GeminiRequestConfig custom_config;
  custom_config.model_name = "gemini-pro-vision";
  custom_config.temperature = 0.3f;
  custom_config.max_output_tokens = 2048;
  
  adapter_->SetRequestConfig(custom_config);
  
  const GeminiRequestConfig& config = adapter_->GetRequestConfig();
  EXPECT_EQ(config.model_name, "gemini-pro-vision");
  EXPECT_FLOAT_EQ(config.temperature, 0.3f);
  EXPECT_EQ(config.max_output_tokens, 2048);
}

// Test API key setting
TEST_F(GeminiTextAdapterTest, SetApiKey) {
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
TEST_F(GeminiTextAdapterTest, ProcessText) {
  bool success = false;
  std::string response;
  RunProcessTextAndWait("What is the capital of France?", &success, &response);
  
  EXPECT_TRUE(success);
  EXPECT_FALSE(response.empty());
}

// Test processing a conversation
TEST_F(GeminiTextAdapterTest, ProcessConversation) {
  std::vector<GeminiMessage> messages;
  
  GeminiMessage system_message;
  system_message.role = GeminiMessage::Role::SYSTEM;
  system_message.content = "You are a helpful assistant.";
  
  GeminiMessage user_message;
  user_message.role = GeminiMessage::Role::USER;
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
TEST_F(GeminiTextAdapterTest, BuildRequestPayload) {
  nlohmann::json payload = adapter_->BuildRequestPayload("Test prompt");
  
  EXPECT_TRUE(payload.contains("contents"));
  EXPECT_TRUE(payload.contains("generationConfig"));
  
  auto contents = payload["contents"];
  EXPECT_TRUE(contents.is_array());
  EXPECT_EQ(contents.size(), 1);
  
  auto content = contents[0];
  EXPECT_TRUE(content.contains("role"));
  EXPECT_EQ(content["role"], "user");
  
  EXPECT_TRUE(content.contains("parts"));
  EXPECT_TRUE(content["parts"].is_array());
  EXPECT_EQ(content["parts"].size(), 1);
  
  auto part = content["parts"][0];
  EXPECT_TRUE(part.contains("text"));
  EXPECT_EQ(part["text"], "Test prompt");
  
  auto gen_config = payload["generationConfig"];
  EXPECT_TRUE(gen_config.contains("temperature"));
  EXPECT_TRUE(gen_config.contains("maxOutputTokens"));
  EXPECT_TRUE(gen_config.contains("topP"));
  EXPECT_TRUE(gen_config.contains("topK"));
}

// Test request payload building for conversation
TEST_F(GeminiTextAdapterTest, BuildConversationPayload) {
  std::vector<GeminiMessage> messages;
  
  GeminiMessage system_message;
  system_message.role = GeminiMessage::Role::SYSTEM;
  system_message.content = "You are a helpful assistant.";
  
  GeminiMessage user_message;
  user_message.role = GeminiMessage::Role::USER;
  user_message.content = "Hello!";
  
  messages.push_back(system_message);
  messages.push_back(user_message);
  
  nlohmann::json payload = adapter_->BuildConversationPayload(messages);
  
  EXPECT_TRUE(payload.contains("contents"));
  EXPECT_TRUE(payload.contains("generationConfig"));
  
  auto contents = payload["contents"];
  EXPECT_TRUE(contents.is_array());
  EXPECT_EQ(contents.size(), 2);
  
  auto system_content = contents[0];
  EXPECT_TRUE(system_content.contains("role"));
  EXPECT_EQ(system_content["role"], "system");
  
  auto user_content = contents[1];
  EXPECT_TRUE(user_content.contains("role"));
  EXPECT_EQ(user_content["role"], "user");
  
  EXPECT_TRUE(user_content.contains("parts"));
  EXPECT_TRUE(user_content["parts"].is_array());
  EXPECT_EQ(user_content["parts"].size(), 1);
  
  auto part = user_content["parts"][0];
  EXPECT_TRUE(part.contains("text"));
  EXPECT_EQ(part["text"], "Hello!");
}

// Test role to string conversion
TEST_F(GeminiTextAdapterTest, RoleToString) {
  EXPECT_EQ(adapter_->RoleToString(GeminiMessage::Role::USER), "user");
  EXPECT_EQ(adapter_->RoleToString(GeminiMessage::Role::MODEL), "model");
  EXPECT_EQ(adapter_->RoleToString(GeminiMessage::Role::SYSTEM), "system");
}

}  // namespace
}  // namespace gemini
}  // namespace adapters
}  // namespace asol