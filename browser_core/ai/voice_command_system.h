// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_CORE_AI_VOICE_COMMAND_SYSTEM_H_
#define BROWSER_CORE_AI_VOICE_COMMAND_SYSTEM_H_

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "asol/core/ai_service_manager.h"
#include "browser_core/engine/browser_engine.h"

namespace browser_core {
namespace ai {

// VoiceCommandSystem provides voice interaction capabilities for the browser.
class VoiceCommandSystem {
 public:
  // Command types
  enum class CommandType {
    NAVIGATION,      // "Go to example.com", "Go back", etc.
    SEARCH,          // "Search for cats", etc.
    BROWSER_ACTION,  // "Open new tab", "Close tab", etc.
    QUESTION,        // "What is the capital of France?", etc.
    PAGE_ACTION,     // "Scroll down", "Click on login", etc.
    CONTENT_ACTION,  // "Summarize this page", "Translate to Spanish", etc.
    SYSTEM,          // "Turn on dark mode", "Increase volume", etc.
    UNKNOWN          // Could not classify the command
  };

  // Command result
  struct CommandResult {
    bool success;
    std::string response;
    std::string error_message;
    CommandType command_type;
  };

  // Callback for command results
  using CommandResultCallback = 
      base::OnceCallback<void(const CommandResult&)>;

  // Voice recognition result
  struct VoiceRecognitionResult {
    bool success;
    std::string text;
    float confidence;
    std::string error_message;
  };

  // Callback for voice recognition
  using VoiceRecognitionCallback = 
      base::OnceCallback<void(const VoiceRecognitionResult&)>;

  VoiceCommandSystem();
  ~VoiceCommandSystem();

  // Disallow copy and assign
  VoiceCommandSystem(const VoiceCommandSystem&) = delete;
  VoiceCommandSystem& operator=(const VoiceCommandSystem&) = delete;

  // Initialize with browser engine and AI service manager
  bool Initialize(BrowserEngine* browser_engine, 
                asol::core::AIServiceManager* ai_service_manager);

  // Start listening for voice commands
  void StartListening(VoiceRecognitionCallback callback);

  // Stop listening
  void StopListening();

  // Process a voice command
  void ProcessCommand(const std::string& command_text, 
                    CommandResultCallback callback);

  // Check if the system is currently listening
  bool IsListening() const;

  // Enable/disable voice commands
  void Enable(bool enable);
  bool IsEnabled() const;

  // Get a weak pointer to this instance
  base::WeakPtr<VoiceCommandSystem> GetWeakPtr();

 private:
  // Command handlers
  void HandleNavigationCommand(const std::string& command, 
                             CommandResultCallback callback);
  
  void HandleSearchCommand(const std::string& command, 
                         CommandResultCallback callback);
  
  void HandleBrowserActionCommand(const std::string& command, 
                                CommandResultCallback callback);
  
  void HandleQuestionCommand(const std::string& command, 
                           CommandResultCallback callback);
  
  void HandlePageActionCommand(const std::string& command, 
                             CommandResultCallback callback);
  
  void HandleContentActionCommand(const std::string& command, 
                                CommandResultCallback callback);
  
  void HandleSystemCommand(const std::string& command, 
                         CommandResultCallback callback);

  // Classify command type
  CommandType ClassifyCommand(const std::string& command);

  // Parse command parameters
  std::unordered_map<std::string, std::string> ParseCommandParameters(
      const std::string& command, CommandType command_type);

  // Browser engine and AI service manager
  BrowserEngine* browser_engine_ = nullptr;
  asol::core::AIServiceManager* ai_service_manager_ = nullptr;

  // Voice recognition state
  bool is_listening_ = false;
  bool is_enabled_ = true;

  // Private implementation
  class Impl;
  std::unique_ptr<Impl> impl_;

  // For weak pointers
  base::WeakPtrFactory<VoiceCommandSystem> weak_ptr_factory_{this};
};

}  // namespace ai
}  // namespace browser_core

#endif  // BROWSER_CORE_AI_VOICE_COMMAND_SYSTEM_H_