// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASOL_BROWSER_RESEARCH_MODE_CONTROLLER_H_
#define ASOL_BROWSER_RESEARCH_MODE_CONTROLLER_H_

#include <memory>
#include <string>
#include <vector>

#include "asol/browser/page_context_extractor.h"
#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class WebContents;
}

namespace asol {
namespace browser {

// Structure to hold research data from a page
struct ResearchPageData {
  std::string url;
  std::string title;
  std::string content;
  base::Time timestamp;
  std::vector<std::string> key_points;
  bool is_processed = false;
};

// Structure to hold a research session
struct ResearchSession {
  std::string id;
  std::string name;
  std::string topic;
  std::vector<ResearchPageData> pages;
  base::Time created;
  base::Time last_updated;
};

// ResearchModeController manages the research mode functionality.
// It tracks visited pages, extracts relevant content, and provides
// tools for organizing and analyzing research data.
class ResearchModeController
    : public content::WebContentsObserver,
      public content::WebContentsUserData<ResearchModeController> {
 public:
  // Callback for receiving research data
  using ResearchDataCallback = base::OnceCallback<void(const ResearchSession&)>;
  
  // Callback for receiving a list of research sessions
  using ResearchSessionsCallback = 
      base::OnceCallback<void(const std::vector<ResearchSession>&)>;

  ~ResearchModeController() override;

  // Disallow copy and assign
  ResearchModeController(const ResearchModeController&) = delete;
  ResearchModeController& operator=(const ResearchModeController&) = delete;

  // Enable or disable research mode
  void SetResearchModeEnabled(bool enabled);

  // Check if research mode is enabled
  bool IsResearchModeEnabled() const;

  // Create a new research session
  void CreateResearchSession(const std::string& name, 
                            const std::string& topic);

  // Get the current research session
  const ResearchSession* GetCurrentSession() const;

  // Get all research sessions
  void GetAllSessions(ResearchSessionsCallback callback);

  // Switch to a different research session
  void SwitchSession(const std::string& session_id);

  // Add the current page to the research session
  void AddCurrentPageToSession();

  // Add a specific page to the research session
  void AddPageToSession(const std::string& url, 
                       const std::string& title,
                       const std::string& content);

  // Remove a page from the research session
  void RemovePageFromSession(const std::string& url);

  // Generate a summary of the research session
  void GenerateSessionSummary(ResearchDataCallback callback);

  // Generate key points from a page
  void GenerateKeyPoints(const std::string& url, 
                        base::OnceCallback<void(const std::vector<std::string>&)> callback);

  // Export the research session to a document
  void ExportSessionToDocument(const std::string& format,
                              base::OnceCallback<void(const std::string&)> callback);

  // Search within the research session
  void SearchSession(const std::string& query,
                    base::OnceCallback<void(const std::vector<ResearchPageData>&)> callback);

 private:
  // Allow WebContentsUserData to create instances of this class
  friend class content::WebContentsUserData<ResearchModeController>;

  // Constructor
  explicit ResearchModeController(content::WebContents* web_contents);

  // WebContentsObserver implementation
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void WebContentsDestroyed() override;
  void TitleWasSet(content::NavigationEntry* entry) override;

  // Process a page for research data
  void ProcessPage(const std::string& url, 
                  const std::string& title,
                  PageContextExtractor::ContextCallback callback);

  // Handle extracted context from a page
  void HandleExtractedContext(const std::string& url,
                             const std::string& title,
                             const std::string& context);

  // Generate a unique ID for a research session
  std::string GenerateSessionId() const;

  // Save research sessions to disk
  void SaveSessions();

  // Load research sessions from disk
  void LoadSessions();

  // The context extractor
  std::unique_ptr<PageContextExtractor> context_extractor_;

  // Whether research mode is enabled
  bool research_mode_enabled_ = false;

  // The current research session
  std::string current_session_id_;

  // All research sessions
  std::vector<ResearchSession> sessions_;

  // For generating weak pointers to this
  base::WeakPtrFactory<ResearchModeController> weak_ptr_factory_{this};

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace browser
}  // namespace asol

#endif  // ASOL_BROWSER_RESEARCH_MODE_CONTROLLER_H_