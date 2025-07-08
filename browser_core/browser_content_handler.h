// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_CORE_BROWSER_CONTENT_HANDLER_H_
#define BROWSER_CORE_BROWSER_CONTENT_HANDLER_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "browser_core/browser_features.h"
#include "browser_core/content/content_extractor.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace browser_core {

// BrowserContentHandler manages content extraction and processing for browser pages.
// It coordinates between the content extraction, AI features, and browser UI.
class BrowserContentHandler {
 public:
  // Content processing result
  struct ProcessingResult {
    std::string page_url;
    std::string page_title;
    std::string main_content;
    content::ContentExtractor::ContentType content_type;
    bool is_summarizable;
    bool is_searchable;
    bool is_analyzable;
  };

  // Callback for content processing
  using ProcessingCallback = 
      base::OnceCallback<void(const ProcessingResult& result)>;

  BrowserContentHandler();
  ~BrowserContentHandler();

  // Disallow copy and assign
  BrowserContentHandler(const BrowserContentHandler&) = delete;
  BrowserContentHandler& operator=(const BrowserContentHandler&) = delete;

  // Initialize the handler
  bool Initialize(BrowserFeatures* browser_features);

  // Process a page
  void ProcessPage(const std::string& page_url,
                 const std::string& html_content,
                 views::View* toolbar_view,
                 views::Widget* browser_widget,
                 ProcessingCallback callback);

  // Process a page synchronously
  ProcessingResult ProcessPageSync(const std::string& page_url,
                                 const std::string& html_content);

  // Notify the handler of page navigation events
  void OnPageLoaded(const std::string& page_url,
                  const std::string& html_content,
                  views::View* toolbar_view,
                  views::Widget* browser_widget);
  void OnPageUnloaded(const std::string& page_url);
  void OnBrowserClosed();

  // Get a weak pointer to this instance
  base::WeakPtr<BrowserContentHandler> GetWeakPtr();

 private:
  // Handle content extraction result
  void OnContentExtracted(const std::string& page_url,
                        views::View* toolbar_view,
                        views::Widget* browser_widget,
                        ProcessingCallback callback,
                        const content::ContentExtractor::ExtractedContent& content);

  // Components
  BrowserFeatures* browser_features_ = nullptr;
  std::unique_ptr<content::ContentExtractor> content_extractor_;

  // Cache of processed pages
  std::unordered_map<std::string, ProcessingResult> page_cache_;

  // For weak pointers
  base::WeakPtrFactory<BrowserContentHandler> weak_ptr_factory_{this};
};

}  // namespace browser_core

#endif  // BROWSER_CORE_BROWSER_CONTENT_HANDLER_H_