// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser_core/engine/browser_engine.h"

#include <algorithm>
#include <utility>

#include "base/logging.h"
#include "browser_core/engine/tab_impl.h"

namespace browser_core {

// Private implementation of BrowserEngine
class BrowserEngine::Impl {
 public:
  Impl() : next_tab_id_(1), active_tab_id_(-1) {
    home_page_ = "https://www.dashaibrowser.com";
    user_agent_ = "DashAIBrowser/1.0";
    download_directory_ = "/downloads";
    javascript_enabled_ = true;
    cookies_enabled_ = true;
    popups_blocked_ = true;
  }

  ~Impl() {
    tabs_.clear();
  }

  bool Initialize() {
    LOG(INFO) << "Initializing BrowserEngine";
    // Initialize browser components
    return true;
  }

  std::unique_ptr<Tab> CreateTab() {
    int tab_id = next_tab_id_++;
    auto tab = std::make_unique<TabImpl>(tab_id);
    
    Tab* tab_ptr = tab.get();
    tabs_[tab_id] = tab_ptr;
    
    if (active_tab_id_ == -1) {
      active_tab_id_ = tab_id;
      tab_ptr->SetActive(true);
    }
    
    LOG(INFO) << "Created tab with ID: " << tab_id;
    return tab;
  }

  bool CloseTab(int tab_id) {
    auto it = tabs_.find(tab_id);
    if (it == tabs_.end()) {
      LOG(ERROR) << "Attempted to close non-existent tab: " << tab_id;
      return false;
    }
    
    tabs_.erase(it);
    LOG(INFO) << "Closed tab with ID: " << tab_id;
    
    // If we closed the active tab, activate another one if available
    if (tab_id == active_tab_id_) {
      active_tab_id_ = -1;
      if (!tabs_.empty()) {
        active_tab_id_ = tabs_.begin()->first;
        tabs_[active_tab_id_]->SetActive(true);
      }
    }
    
    return true;
  }

  Tab* GetTabById(int tab_id) {
    auto it = tabs_.find(tab_id);
    if (it == tabs_.end()) {
      return nullptr;
    }
    return it->second;
  }

  std::vector<Tab*> GetAllTabs() {
    std::vector<Tab*> result;
    result.reserve(tabs_.size());
    
    for (const auto& pair : tabs_) {
      result.push_back(pair.second);
    }
    
    return result;
  }

  Tab* GetActiveTab() {
    if (active_tab_id_ == -1) {
      return nullptr;
    }
    return GetTabById(active_tab_id_);
  }

  void SetActiveTab(int tab_id) {
    // Deactivate current active tab
    if (active_tab_id_ != -1) {
      Tab* current_active = GetTabById(active_tab_id_);
      if (current_active) {
        current_active->SetActive(false);
      }
    }
    
    // Activate new tab
    Tab* new_active = GetTabById(tab_id);
    if (new_active) {
      new_active->SetActive(true);
      active_tab_id_ = tab_id;
      LOG(INFO) << "Set active tab to: " << tab_id;
    } else {
      LOG(ERROR) << "Attempted to activate non-existent tab: " << tab_id;
    }
  }

  void Navigate(int tab_id, const std::string& url) {
    Tab* tab = GetTabById(tab_id);
    if (tab) {
      tab->Navigate(url);
      LOG(INFO) << "Navigating tab " << tab_id << " to: " << url;
    } else {
      LOG(ERROR) << "Attempted to navigate non-existent tab: " << tab_id;
    }
  }

  void GoBack(int tab_id) {
    Tab* tab = GetTabById(tab_id);
    if (tab) {
      tab->GoBack();
    }
  }

  void GoForward(int tab_id) {
    Tab* tab = GetTabById(tab_id);
    if (tab) {
      tab->GoForward();
    }
  }

  void Reload(int tab_id) {
    Tab* tab = GetTabById(tab_id);
    if (tab) {
      tab->Reload();
    }
  }

  void StopLoading(int tab_id) {
    Tab* tab = GetTabById(tab_id);
    if (tab) {
      tab->StopLoading();
    }
  }

  void AddBookmark(const std::string& url, const std::string& title) {
    bookmarks_[url] = title;
    LOG(INFO) << "Added bookmark: " << title << " (" << url << ")";
  }

  void RemoveBookmark(const std::string& url) {
    bookmarks_.erase(url);
    LOG(INFO) << "Removed bookmark: " << url;
  }

  bool IsBookmarked(const std::string& url) {
    return bookmarks_.find(url) != bookmarks_.end();
  }

  std::vector<std::pair<std::string, std::string>> GetBookmarks() {
    std::vector<std::pair<std::string, std::string>> result;
    result.reserve(bookmarks_.size());
    
    for (const auto& pair : bookmarks_) {
      result.emplace_back(pair.first, pair.second);
    }
    
    return result;
  }

  std::vector<std::pair<std::string, std::string>> GetHistory(int max_items) {
    std::vector<std::pair<std::string, std::string>> result;
    
    // In a real implementation, this would query a history database
    // For now, return a placeholder
    result.reserve(std::min(max_items, static_cast<int>(history_.size())));
    
    int count = 0;
    for (const auto& pair : history_) {
      if (count >= max_items) {
        break;
      }
      result.emplace_back(pair.first, pair.second);
      count++;
    }
    
    return result;
  }

  void ClearHistory() {
    history_.clear();
    LOG(INFO) << "Cleared browsing history";
  }

  void RemoveFromHistory(const std::string& url) {
    history_.erase(url);
    LOG(INFO) << "Removed from history: " << url;
  }

  void SetHomePage(const std::string& url) {
    home_page_ = url;
  }

  std::string GetHomePage() const {
    return home_page_;
  }

  void SetUserAgent(const std::string& user_agent) {
    user_agent_ = user_agent;
  }

  std::string GetUserAgent() const {
    return user_agent_;
  }

  void SetDownloadDirectory(const std::string& directory) {
    download_directory_ = directory;
  }

  std::string GetDownloadDirectory() const {
    return download_directory_;
  }

  void SetJavaScriptEnabled(bool enabled) {
    javascript_enabled_ = enabled;
  }

  bool IsJavaScriptEnabled() const {
    return javascript_enabled_;
  }

  void SetCookiesEnabled(bool enabled) {
    cookies_enabled_ = enabled;
  }

  bool AreCookiesEnabled() const {
    return cookies_enabled_;
  }

  void SetPopupsBlocked(bool blocked) {
    popups_blocked_ = blocked;
  }

  bool ArePopupsBlocked() const {
    return popups_blocked_;
  }

 private:
  int next_tab_id_;
  int active_tab_id_;
  std::map<int, Tab*> tabs_;
  std::map<std::string, std::string> bookmarks_;
  std::map<std::string, std::string> history_;
  
  // Settings
  std::string home_page_;
  std::string user_agent_;
  std::string download_directory_;
  bool javascript_enabled_;
  bool cookies_enabled_;
  bool popups_blocked_;
};

// BrowserEngine implementation

BrowserEngine::BrowserEngine() : impl_(std::make_unique<Impl>()) {}

BrowserEngine::~BrowserEngine() = default;

bool BrowserEngine::Initialize() {
  return impl_->Initialize();
}

std::unique_ptr<Tab> BrowserEngine::CreateTab() {
  return impl_->CreateTab();
}

bool BrowserEngine::CloseTab(int tab_id) {
  return impl_->CloseTab(tab_id);
}

Tab* BrowserEngine::GetTabById(int tab_id) {
  return impl_->GetTabById(tab_id);
}

std::vector<Tab*> BrowserEngine::GetAllTabs() {
  return impl_->GetAllTabs();
}

Tab* BrowserEngine::GetActiveTab() {
  return impl_->GetActiveTab();
}

void BrowserEngine::SetActiveTab(int tab_id) {
  impl_->SetActiveTab(tab_id);
}

void BrowserEngine::Navigate(int tab_id, const std::string& url) {
  impl_->Navigate(tab_id, url);
}

void BrowserEngine::GoBack(int tab_id) {
  impl_->GoBack(tab_id);
}

void BrowserEngine::GoForward(int tab_id) {
  impl_->GoForward(tab_id);
}

void BrowserEngine::Reload(int tab_id) {
  impl_->Reload(tab_id);
}

void BrowserEngine::StopLoading(int tab_id) {
  impl_->StopLoading(tab_id);
}

void BrowserEngine::AddBookmark(const std::string& url, const std::string& title) {
  impl_->AddBookmark(url, title);
}

void BrowserEngine::RemoveBookmark(const std::string& url) {
  impl_->RemoveBookmark(url);
}

bool BrowserEngine::IsBookmarked(const std::string& url) {
  return impl_->IsBookmarked(url);
}

std::vector<std::pair<std::string, std::string>> BrowserEngine::GetBookmarks() {
  return impl_->GetBookmarks();
}

std::vector<std::pair<std::string, std::string>> BrowserEngine::GetHistory(int max_items) {
  return impl_->GetHistory(max_items);
}

void BrowserEngine::ClearHistory() {
  impl_->ClearHistory();
}

void BrowserEngine::RemoveFromHistory(const std::string& url) {
  impl_->RemoveFromHistory(url);
}

void BrowserEngine::SetHomePage(const std::string& url) {
  impl_->SetHomePage(url);
}

std::string BrowserEngine::GetHomePage() const {
  return impl_->GetHomePage();
}

void BrowserEngine::SetUserAgent(const std::string& user_agent) {
  impl_->SetUserAgent(user_agent);
}

std::string BrowserEngine::GetUserAgent() const {
  return impl_->GetUserAgent();
}

void BrowserEngine::SetDownloadDirectory(const std::string& directory) {
  impl_->SetDownloadDirectory(directory);
}

std::string BrowserEngine::GetDownloadDirectory() const {
  return impl_->GetDownloadDirectory();
}

void BrowserEngine::SetJavaScriptEnabled(bool enabled) {
  impl_->SetJavaScriptEnabled(enabled);
}

bool BrowserEngine::IsJavaScriptEnabled() const {
  return impl_->IsJavaScriptEnabled();
}

void BrowserEngine::SetCookiesEnabled(bool enabled) {
  impl_->SetCookiesEnabled(enabled);
}

bool BrowserEngine::AreCookiesEnabled() const {
  return impl_->AreCookiesEnabled();
}

void BrowserEngine::SetPopupsBlocked(bool blocked) {
  impl_->SetPopupsBlocked(blocked);
}

bool BrowserEngine::ArePopupsBlocked() const {
  return impl_->ArePopupsBlocked();
}

base::WeakPtr<BrowserEngine> BrowserEngine::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

}  // namespace browser_core