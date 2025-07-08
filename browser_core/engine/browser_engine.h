// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_CORE_ENGINE_BROWSER_ENGINE_H_
#define BROWSER_CORE_ENGINE_BROWSER_ENGINE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "browser_core/engine/navigation_controller.h"
#include "browser_core/engine/tab.h"
#include "browser_core/engine/web_contents.h"

namespace browser_core {

// BrowserEngine is the main interface for browser functionality.
// It manages tabs, navigation, and browser-wide settings.
class BrowserEngine {
 public:
  BrowserEngine();
  ~BrowserEngine();

  // Disallow copy and assign
  BrowserEngine(const BrowserEngine&) = delete;
  BrowserEngine& operator=(const BrowserEngine&) = delete;

  // Initialize the browser engine
  bool Initialize();

  // Tab management
  std::unique_ptr<Tab> CreateTab();
  bool CloseTab(int tab_id);
  Tab* GetTabById(int tab_id);
  std::vector<Tab*> GetAllTabs();
  Tab* GetActiveTab();
  void SetActiveTab(int tab_id);

  // Navigation
  void Navigate(int tab_id, const std::string& url);
  void GoBack(int tab_id);
  void GoForward(int tab_id);
  void Reload(int tab_id);
  void StopLoading(int tab_id);

  // Bookmarks
  void AddBookmark(const std::string& url, const std::string& title);
  void RemoveBookmark(const std::string& url);
  bool IsBookmarked(const std::string& url);
  std::vector<std::pair<std::string, std::string>> GetBookmarks();

  // History
  std::vector<std::pair<std::string, std::string>> GetHistory(int max_items);
  void ClearHistory();
  void RemoveFromHistory(const std::string& url);

  // Settings
  void SetHomePage(const std::string& url);
  std::string GetHomePage() const;
  void SetUserAgent(const std::string& user_agent);
  std::string GetUserAgent() const;
  void SetDownloadDirectory(const std::string& directory);
  std::string GetDownloadDirectory() const;

  // Security settings
  void SetJavaScriptEnabled(bool enabled);
  bool IsJavaScriptEnabled() const;
  void SetCookiesEnabled(bool enabled);
  bool AreCookiesEnabled() const;
  void SetPopupsBlocked(bool blocked);
  bool ArePopupsBlocked() const;

  // Get a weak pointer to this instance
  base::WeakPtr<BrowserEngine> GetWeakPtr();

 private:
  // Private implementation details
  class Impl;
  std::unique_ptr<Impl> impl_;

  // For weak pointers
  base::WeakPtrFactory<BrowserEngine> weak_ptr_factory_{this};
};

}  // namespace browser_core

#endif  // BROWSER_CORE_ENGINE_BROWSER_ENGINE_H_