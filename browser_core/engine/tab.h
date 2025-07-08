// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_CORE_ENGINE_TAB_H_
#define BROWSER_CORE_ENGINE_TAB_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "browser_core/engine/navigation_controller.h"
#include "browser_core/engine/web_contents.h"

namespace browser_core {

// Tab represents a browser tab with its own navigation state and web contents.
class Tab {
 public:
  // Tab state
  enum class State {
    LOADING,
    COMPLETE,
    ERROR,
    CRASHED
  };

  // Tab events
  using TitleChangedCallback = base::RepeatingCallback<void(const std::string&)>;
  using URLChangedCallback = base::RepeatingCallback<void(const std::string&)>;
  using StateChangedCallback = base::RepeatingCallback<void(State)>;
  using FaviconChangedCallback = base::RepeatingCallback<void(const std::string&)>;

  Tab();
  virtual ~Tab();

  // Disallow copy and assign
  Tab(const Tab&) = delete;
  Tab& operator=(const Tab&) = delete;

  // Basic tab properties
  virtual int GetId() const = 0;
  virtual std::string GetTitle() const = 0;
  virtual std::string GetURL() const = 0;
  virtual State GetState() const = 0;
  virtual std::string GetFaviconURL() const = 0;
  virtual bool IsActive() const = 0;
  virtual void SetActive(bool active) = 0;

  // Navigation
  virtual NavigationController* GetNavigationController() = 0;
  virtual void Navigate(const std::string& url) = 0;
  virtual void GoBack() = 0;
  virtual void GoForward() = 0;
  virtual void Reload() = 0;
  virtual void StopLoading() = 0;

  // Web contents
  virtual WebContents* GetWebContents() = 0;

  // Event registration
  virtual void SetTitleChangedCallback(TitleChangedCallback callback) = 0;
  virtual void SetURLChangedCallback(URLChangedCallback callback) = 0;
  virtual void SetStateChangedCallback(StateChangedCallback callback) = 0;
  virtual void SetFaviconChangedCallback(FaviconChangedCallback callback) = 0;
};

}  // namespace browser_core

#endif  // BROWSER_CORE_ENGINE_TAB_H_