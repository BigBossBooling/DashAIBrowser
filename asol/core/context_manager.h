// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASOL_CORE_CONTEXT_MANAGER_H_
#define ASOL_CORE_CONTEXT_MANAGER_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/time/time.h"

namespace asol {
namespace core {

// Represents a message in a conversation context
struct ContextMessage {
  enum class Role {
    USER,
    ASSISTANT,
    SYSTEM
  };

  Role role;
  std::string content;
  base::Time timestamp;
};

// Represents a conversation context
class ConversationContext {
 public:
  ConversationContext();
  ~ConversationContext();

  // Add a message to the context
  void AddMessage(ContextMessage::Role role, const std::string& content);

  // Get all messages in the context
  const std::vector<ContextMessage>& GetMessages() const;

  // Get the context ID
  std::string GetContextId() const;

  // Get the creation time
  base::Time GetCreationTime() const;

  // Get the last update time
  base::Time GetLastUpdateTime() const;

  // Clear the context
  void Clear();

 private:
  std::string context_id_;
  std::vector<ContextMessage> messages_;
  base::Time creation_time_;
  base::Time last_update_time_;
};

// Manages conversation contexts for AI interactions
class ContextManager {
 public:
  ContextManager();
  ~ContextManager();

  // Create a new context
  std::string CreateContext();

  // Get a context by ID
  ConversationContext* GetContext(const std::string& context_id);

  // Delete a context
  void DeleteContext(const std::string& context_id);

  // Add a message to a context
  void AddMessage(const std::string& context_id, 
                 ContextMessage::Role role, 
                 const std::string& content);

  // Get all context IDs
  std::vector<std::string> GetAllContextIds() const;

  // Clear all contexts
  void ClearAllContexts();

 private:
  // Private implementation
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace core
}  // namespace asol

#endif  // ASOL_CORE_CONTEXT_MANAGER_H_