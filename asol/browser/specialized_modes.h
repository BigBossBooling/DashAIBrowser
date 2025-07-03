// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASOL_BROWSER_SPECIALIZED_MODES_H_
#define ASOL_BROWSER_SPECIALIZED_MODES_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class WebContents;
}

namespace asol {
namespace browser {

// Enum for the different specialized modes
enum class SpecializedMode {
  kNone,
  kDeveloper,
  kWork,
  kGaming
};

// Structure to hold code snippet data
struct CodeSnippet {
  std::string language;
  std::string code;
  std::string description;
  std::string source_url;
};

// Structure to hold work document data
struct WorkDocument {
  std::string title;
  std::string content;
  std::string format;
  std::string url;
  bool is_draft = true;
};

// Structure to hold gaming data
struct GameInfo {
  std::string title;
  std::string genre;
  std::string platform;
  std::string tips;
  std::string strategies;
  std::string url;
};

// SpecializedModesController manages the specialized modes functionality.
// It provides developer, work, and gaming modes with specific features
// tailored to each use case.
class SpecializedModesController
    : public content::WebContentsObserver,
      public content::WebContentsUserData<SpecializedModesController> {
 public:
  // Callback for receiving code snippets
  using CodeSnippetsCallback = 
      base::OnceCallback<void(const std::vector<CodeSnippet>&)>;
  
  // Callback for receiving work documents
  using WorkDocumentsCallback = 
      base::OnceCallback<void(const std::vector<WorkDocument>&)>;
  
  // Callback for receiving game info
  using GameInfoCallback = 
      base::OnceCallback<void(const std::vector<GameInfo>&)>;

  ~SpecializedModesController() override;

  // Disallow copy and assign
  SpecializedModesController(const SpecializedModesController&) = delete;
  SpecializedModesController& operator=(const SpecializedModesController&) = delete;

  // Set the current specialized mode
  void SetMode(SpecializedMode mode);

  // Get the current specialized mode
  SpecializedMode GetMode() const;

  // Get a string representation of the current mode
  std::string GetModeString() const;

  //
  // Developer Mode Features
  //

  // Detect programming language on the current page
  void DetectProgrammingLanguage(base::OnceCallback<void(const std::string&)> callback);

  // Extract code snippets from the current page
  void ExtractCodeSnippets(CodeSnippetsCallback callback);

  // Save a code snippet to the collection
  void SaveCodeSnippet(const CodeSnippet& snippet);

  // Get all saved code snippets
  void GetSavedCodeSnippets(CodeSnippetsCallback callback);

  // Generate code documentation
  void GenerateCodeDocumentation(const std::string& code,
                                base::OnceCallback<void(const std::string&)> callback);

  // Explain code
  void ExplainCode(const std::string& code,
                  base::OnceCallback<void(const std::string&)> callback);

  // Optimize code
  void OptimizeCode(const std::string& code,
                   base::OnceCallback<void(const std::string&)> callback);

  // Debug code
  void DebugCode(const std::string& code, const std::string& error_message,
                base::OnceCallback<void(const std::string&)> callback);

  //
  // Work Mode Features
  //

  // Create a new document
  void CreateDocument(const std::string& title, const std::string& format);

  // Get all documents
  void GetAllDocuments(WorkDocumentsCallback callback);

  // Update a document
  void UpdateDocument(const std::string& title, const std::string& content);

  // Generate document content based on a prompt
  void GenerateDocumentContent(const std::string& title, const std::string& prompt,
                              base::OnceCallback<void(const std::string&)> callback);

  // Summarize a document
  void SummarizeDocument(const std::string& content,
                        base::OnceCallback<void(const std::string&)> callback);

  // Format a document
  void FormatDocument(const std::string& content, const std::string& format,
                     base::OnceCallback<void(const std::string&)> callback);

  // Extract action items from a document
  void ExtractActionItems(const std::string& content,
                         base::OnceCallback<void(const std::vector<std::string>&)> callback);

  //
  // Gaming Mode Features
  //

  // Detect game on the current page
  void DetectGame(base::OnceCallback<void(const GameInfo&)> callback);

  // Save game info
  void SaveGameInfo(const GameInfo& game_info);

  // Get all saved game info
  void GetAllGameInfo(GameInfoCallback callback);

  // Get tips for a specific game
  void GetGameTips(const std::string& game_title,
                  base::OnceCallback<void(const std::string&)> callback);

  // Get strategies for a specific game
  void GetGameStrategies(const std::string& game_title,
                        base::OnceCallback<void(const std::string&)> callback);

  // Generate game walkthrough
  void GenerateGameWalkthrough(const std::string& game_title, const std::string& level,
                              base::OnceCallback<void(const std::string&)> callback);

  // Optimize game settings
  void OptimizeGameSettings(const std::string& game_title, const std::string& hardware,
                           base::OnceCallback<void(const std::string&)> callback);

 private:
  // Allow WebContentsUserData to create instances of this class
  friend class content::WebContentsUserData<SpecializedModesController>;

  // Constructor
  explicit SpecializedModesController(content::WebContents* web_contents);

  // WebContentsObserver implementation
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void WebContentsDestroyed() override;

  // Save data to disk
  void SaveData();

  // Load data from disk
  void LoadData();

  // Get the file path for storing specialized mode data
  base::FilePath GetDataFilePath() const;

  // The current specialized mode
  SpecializedMode current_mode_ = SpecializedMode::kNone;

  // Saved code snippets for developer mode
  std::vector<CodeSnippet> code_snippets_;

  // Saved documents for work mode
  std::vector<WorkDocument> documents_;

  // Saved game info for gaming mode
  std::vector<GameInfo> game_info_;

  // For generating weak pointers to this
  base::WeakPtrFactory<SpecializedModesController> weak_ptr_factory_{this};

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace browser
}  // namespace asol

#endif  // ASOL_BROWSER_SPECIALIZED_MODES_H_