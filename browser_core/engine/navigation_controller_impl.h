// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_CORE_ENGINE_NAVIGATION_CONTROLLER_IMPL_H_
#define BROWSER_CORE_ENGINE_NAVIGATION_CONTROLLER_IMPL_H_

#include <memory>
#include <string>
#include <vector>

#include "browser_core/engine/navigation_controller.h"

namespace browser_core {

// Implementation of the NavigationController interface
class NavigationControllerImpl : public NavigationController {
 public:
  NavigationControllerImpl();
  ~NavigationControllerImpl() override;

  // Navigation
  void Navigate(const std::string& url) override;
  void GoBack() override;
  void GoForward() override;
  void Reload() override;
  void Stop() override;

  // History state
  bool CanGoBack() const override;
  bool CanGoForward() const override;
  int GetCurrentEntryIndex() const override;
  int GetEntryCount() const override;
  const NavigationEntry* GetEntryAtIndex(int index) const override;
  const NavigationEntry* GetCurrentEntry() const override;
  const NavigationEntry* GetPendingEntry() const override;
  std::vector<const NavigationEntry*> GetBackwardEntries() const override;
  std::vector<const NavigationEntry*> GetForwardEntries() const override;

  // Event registration
  void SetNavigationStartedCallback(
      NavigationStartedCallback callback) override;
  void SetNavigationCompletedCallback(
      NavigationCompletedCallback callback) override;
  void SetHistoryChangedCallback(
      HistoryChangedCallback callback) override;

 private:
  // Add a new entry to the navigation history
  void AddEntry(std::unique_ptr<NavigationEntry> entry);
  
  // Notify observers of history changes
  void NotifyHistoryChanged();

  // Navigation state
  std::vector<std::unique_ptr<NavigationEntry>> entries_;
  int current_index_;
  std::unique_ptr<NavigationEntry> pending_entry_;
  bool is_loading_;

  // Callbacks
  NavigationStartedCallback navigation_started_callback_;
  NavigationCompletedCallback navigation_completed_callback_;
  HistoryChangedCallback history_changed_callback_;
};

}  // namespace browser_core

#endif  // BROWSER_CORE_ENGINE_NAVIGATION_CONTROLLER_IMPL_H_