// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASOL_BROWSER_PAGE_CONTEXT_EXTRACTOR_H_
#define ASOL_BROWSER_PAGE_CONTEXT_EXTRACTOR_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {
class WebContents;
class RenderFrameHost;
}  // namespace content

namespace asol {
namespace browser {

// PageContextExtractor extracts context from the current page to provide
// more relevant AI responses. It observes web contents and extracts text
// content from the page when requested.
class PageContextExtractor : public content::WebContentsObserver {
 public:
  // Callback for receiving extracted context
  using ContextCallback = base::OnceCallback<void(const std::string&)>;

  // Create a new context extractor for the given web contents
  explicit PageContextExtractor(content::WebContents* web_contents);
  ~PageContextExtractor() override;

  // Disallow copy and assign
  PageContextExtractor(const PageContextExtractor&) = delete;
  PageContextExtractor& operator=(const PageContextExtractor&) = delete;

  // Extract context from the current page
  // The callback will be called with the extracted context
  void ExtractContext(ContextCallback callback);

  // Extract context from the selected text on the page
  // The callback will be called with the extracted context
  void ExtractSelectedText(ContextCallback callback);

  // Extract context from the visible viewport
  // The callback will be called with the extracted context
  void ExtractVisibleContent(ContextCallback callback);

  // Extract context from the entire page
  // The callback will be called with the extracted context
  void ExtractFullPageContent(ContextCallback callback);

  // Extract context from the given element
  // The callback will be called with the extracted context
  void ExtractElementContent(const std::string& selector, ContextCallback callback);

 private:
  // WebContentsObserver implementation
  void WebContentsDestroyed() override;
  void PrimaryPageChanged(content::Page& page) override;

  // Execute a JavaScript function in the page and get the result
  void ExecuteJavaScript(const std::string& script, 
                        base::OnceCallback<void(const std::string&)> callback);

  // Handle the result of a JavaScript execution
  void HandleJavaScriptResult(ContextCallback callback,
                             const std::string& result);

  // For generating weak pointers to this
  base::WeakPtrFactory<PageContextExtractor> weak_ptr_factory_{this};
};

}  // namespace browser
}  // namespace asol

#endif  // ASOL_BROWSER_PAGE_CONTEXT_EXTRACTOR_H_