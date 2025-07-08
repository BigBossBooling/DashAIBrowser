// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_CORE_ENGINE_NAVIGATION_CONTROLLER_H_
#define BROWSER_CORE_ENGINE_NAVIGATION_CONTROLLER_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/time/time.h"

namespace browser_core {

// NavigationEntry represents a single navigation (URL visit) in the browser history.
class NavigationEntry {
 public:
  NavigationEntry();
  NavigationEntry(const std::string& url, 
                 const std::string& title,
                 base::Time timestamp);
  ~NavigationEntry();

  // Getters
  const std::string& GetURL() const { return url_; }
  const std::string& GetTitle() const { return title_; }
  base::Time GetTimestamp() const { return timestamp_; }

  // Setters
  void SetURL(const std::string& url) { url_ = url; }
  void SetTitle(const std::string& title) { title_ = title; }
  void SetTimestamp(base::Time timestamp) { timestamp_ = timestamp; }

 private:
  std::string url_;
  std::string title_;
  base::Time timestamp_;
};

// NavigationController manages browser navigation state including
// back/forward history.
class NavigationController {
 public:
  // Navigation events
  using NavigationStartedCallback = 
      base::RepeatingCallback<void(const std::string& url)>;
  using NavigationCompletedCallback = 
      base::RepeatingCallback<void(const std::string& url, bool success)>;
  using HistoryChangedCallback = 
      base::RepeatingCallback<void()>;

  NavigationController();
  virtual ~NavigationController();

  // Disallow copy and assign
  NavigationController(const NavigationController&) = delete;
  NavigationController& operator=(const NavigationController&) = delete;

  // Navigation
  virtual void Navigate(const std::string& url) = 0;
  virtual void GoBack() = 0;
  virtual void GoForward() = 0;
  virtual void Reload() = 0;
  virtual void Stop() = 0;

  // History state
  virtual bool CanGoBack() const = 0;
  virtual bool CanGoForward() const = 0;
  virtual int GetCurrentEntryIndex() const = 0;
  virtual int GetEntryCount() const = 0;
  virtual const NavigationEntry* GetEntryAtIndex(int index) const = 0;
  virtual const NavigationEntry* GetCurrentEntry() const = 0;
  virtual const NavigationEntry* GetPendingEntry() const = 0;
  virtual std::vector<const NavigationEntry*> GetBackwardEntries() const = 0;
  virtual std::vector<const NavigationEntry*> GetForwardEntries() const = 0;

  // Event registration
  virtual void SetNavigationStartedCallback(
      NavigationStartedCallback callback) = 0;
  virtual void SetNavigationCompletedCallback(
      NavigationCompletedCallback callback) = 0;
  virtual void SetHistoryChangedCallback(
      HistoryChangedCallback callback) = 0;
};

}  // namespace browser_core

#endif  // BROWSER_CORE_ENGINE_NAVIGATION_CONTROLLER_H_