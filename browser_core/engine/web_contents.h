// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_CORE_ENGINE_WEB_CONTENTS_H_
#define BROWSER_CORE_ENGINE_WEB_CONTENTS_H_

#include <string>
#include <vector>

#include "base/callback.h"

namespace browser_core {

// WebContents represents the content of a web page and provides methods
// to interact with it.
class WebContents {
 public:
  // Content analysis result
  struct ContentAnalysis {
    std::string main_text;
    std::string title;
    std::vector<std::string> headings;
    std::vector<std::string> links;
    std::vector<std::string> images;
    std::string language;
    bool is_article;
    std::string author;
    std::string published_date;
  };

  // JavaScript execution result
  struct JavaScriptResult {
    bool success;
    std::string result;
    std::string error;
  };

  // Callback types
  using ContentAnalysisCallback = 
      base::OnceCallback<void(const ContentAnalysis&)>;
  using JavaScriptResultCallback = 
      base::OnceCallback<void(const JavaScriptResult&)>;
  using TextExtractedCallback = 
      base::OnceCallback<void(const std::string&)>;

  WebContents();
  virtual ~WebContents();

  // Disallow copy and assign
  WebContents(const WebContents&) = delete;
  WebContents& operator=(const WebContents&) = delete;

  // Content interaction
  virtual void ExecuteJavaScript(const std::string& script, 
                               JavaScriptResultCallback callback) = 0;
  
  // Content analysis
  virtual void AnalyzeContent(ContentAnalysisCallback callback) = 0;
  virtual void ExtractMainText(TextExtractedCallback callback) = 0;
  virtual void ExtractArticle(TextExtractedCallback callback) = 0;
  
  // DOM interaction
  virtual void ClickElement(const std::string& selector) = 0;
  virtual void FillForm(const std::string& selector, const std::string& value) = 0;
  virtual void ScrollTo(int x, int y) = 0;
  
  // Content capture
  virtual void CaptureVisibleContent(
      base::OnceCallback<void(const std::vector<uint8_t>&)> callback) = 0;
  
  // Security information
  virtual bool IsSecure() const = 0;
  virtual std::string GetCertificateInfo() const = 0;
  
  // Page information
  virtual std::string GetPageSource() const = 0;
  virtual std::string GetDocumentEncoding() const = 0;
};

}  // namespace browser_core

#endif  // BROWSER_CORE_ENGINE_WEB_CONTENTS_H_