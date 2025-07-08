// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_CORE_ENGINE_WEB_CONTENTS_IMPL_H_
#define BROWSER_CORE_ENGINE_WEB_CONTENTS_IMPL_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "browser_core/engine/web_contents.h"

namespace browser_core {

// Implementation of the WebContents interface
class WebContentsImpl : public WebContents {
 public:
  WebContentsImpl();
  ~WebContentsImpl() override;

  // Content interaction
  void ExecuteJavaScript(const std::string& script, 
                       JavaScriptResultCallback callback) override;
  
  // Content analysis
  void AnalyzeContent(ContentAnalysisCallback callback) override;
  void ExtractMainText(TextExtractedCallback callback) override;
  void ExtractArticle(TextExtractedCallback callback) override;
  
  // DOM interaction
  void ClickElement(const std::string& selector) override;
  void FillForm(const std::string& selector, const std::string& value) override;
  void ScrollTo(int x, int y) override;
  
  // Content capture
  void CaptureVisibleContent(
      base::OnceCallback<void(const std::vector<uint8_t>&)> callback) override;
  
  // Security information
  bool IsSecure() const override;
  std::string GetCertificateInfo() const override;
  
  // Page information
  std::string GetPageSource() const override;
  std::string GetDocumentEncoding() const override;

  // Internal methods for the browser engine
  void SetPageTitle(const std::string& title);
  std::string GetPageTitle() const;
  void SetPageURL(const std::string& url);
  std::string GetPageURL() const;
  void SetFaviconURL(const std::string& favicon_url);
  std::string GetFaviconURL() const;

  // Event callbacks
  using TitleChangedCallback = base::RepeatingCallback<void(const std::string&)>;
  using FaviconChangedCallback = base::RepeatingCallback<void(const std::string&)>;
  
  void SetTitleChangedCallback(TitleChangedCallback callback);
  void SetFaviconChangedCallback(FaviconChangedCallback callback);

 private:
  // Private implementation
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace browser_core

#endif  // BROWSER_CORE_ENGINE_WEB_CONTENTS_IMPL_H_