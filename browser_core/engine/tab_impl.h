// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_CORE_ENGINE_TAB_IMPL_H_
#define BROWSER_CORE_ENGINE_TAB_IMPL_H_

#include <memory>
#include <string>

#include "browser_core/engine/tab.h"
#include "browser_core/engine/navigation_controller_impl.h"
#include "browser_core/engine/web_contents_impl.h"

namespace browser_core {

// Implementation of the Tab interface
class TabImpl : public Tab {
 public:
  explicit TabImpl(int id);
  ~TabImpl() override;

  // Tab interface implementation
  int GetId() const override;
  std::string GetTitle() const override;
  std::string GetURL() const override;
  State GetState() const override;
  std::string GetFaviconURL() const override;
  bool IsActive() const override;
  void SetActive(bool active) override;

  NavigationController* GetNavigationController() override;
  void Navigate(const std::string& url) override;
  void GoBack() override;
  void GoForward() override;
  void Reload() override;
  void StopLoading() override;

  WebContents* GetWebContents() override;

  void SetTitleChangedCallback(TitleChangedCallback callback) override;
  void SetURLChangedCallback(URLChangedCallback callback) override;
  void SetStateChangedCallback(StateChangedCallback callback) override;
  void SetFaviconChangedCallback(FaviconChangedCallback callback) override;

 private:
  // Tab properties
  int id_;
  std::string title_;
  State state_;
  std::string favicon_url_;
  bool is_active_;

  // Components
  std::unique_ptr<NavigationControllerImpl> navigation_controller_;
  std::unique_ptr<WebContentsImpl> web_contents_;

  // Callbacks
  TitleChangedCallback title_changed_callback_;
  URLChangedCallback url_changed_callback_;
  StateChangedCallback state_changed_callback_;
  FaviconChangedCallback favicon_changed_callback_;

  // Internal event handlers
  void OnNavigationStarted(const std::string& url);
  void OnNavigationCompleted(const std::string& url, bool success);
  void OnTitleChanged(const std::string& title);
  void OnFaviconChanged(const std::string& favicon_url);
};

}  // namespace browser_core

#endif  // BROWSER_CORE_ENGINE_TAB_IMPL_H_