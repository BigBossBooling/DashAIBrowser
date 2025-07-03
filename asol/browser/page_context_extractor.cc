// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "asol/browser/page_context_extractor.h"

#include <string>

#include "asol/browser/browser_features.h"
#include "asol/util/performance_tracker.h"
#include "base/feature_list.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

namespace asol {
namespace browser {

namespace {

// JavaScript to extract selected text
const char kExtractSelectedTextScript[] = R"(
  (function() {
    return window.getSelection().toString();
  })();
)";

// JavaScript to extract visible content
const char kExtractVisibleContentScript[] = R"(
  (function() {
    function isElementInViewport(el) {
      const rect = el.getBoundingClientRect();
      return (
        rect.top >= 0 &&
        rect.left >= 0 &&
        rect.bottom <= window.innerHeight &&
        rect.right <= window.innerWidth
      );
    }

    function getVisibleText() {
      const elements = document.querySelectorAll('p, h1, h2, h3, h4, h5, h6, li, td, th, div, span');
      let visibleText = '';
      
      for (const element of elements) {
        if (isElementInViewport(element) && element.textContent.trim()) {
          visibleText += element.textContent.trim() + '\n';
        }
      }
      
      return visibleText;
    }
    
    return getVisibleText();
  })();
)";

// JavaScript to extract full page content
const char kExtractFullPageContentScript[] = R"(
  (function() {
    function getPageText() {
      // Get the page title
      let text = document.title + '\n\n';
      
      // Get meta description if available
      const metaDescription = document.querySelector('meta[name="description"]');
      if (metaDescription) {
        text += metaDescription.getAttribute('content') + '\n\n';
      }
      
      // Get main content
      const elements = document.querySelectorAll('p, h1, h2, h3, h4, h5, h6, li, td, th');
      for (const element of elements) {
        if (element.textContent.trim()) {
          text += element.textContent.trim() + '\n';
        }
      }
      
      return text;
    }
    
    return getPageText();
  })();
)";

// JavaScript to extract content from a specific element
const char kExtractElementContentScript[] = R"(
  (function(selector) {
    const element = document.querySelector(selector);
    if (!element) {
      return '';
    }
    return element.textContent.trim();
  })('%s');
)";

}  // namespace

PageContextExtractor::PageContextExtractor(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {
  DLOG(INFO) << "PageContextExtractor created for WebContents: " << web_contents;
}

PageContextExtractor::~PageContextExtractor() {
  DLOG(INFO) << "PageContextExtractor destroyed";
}

void PageContextExtractor::ExtractContext(ContextCallback callback) {
  // Track performance of this operation
  util::ScopedPerformanceTracker tracker("PageContextExtractor_ExtractContext");
  
  // Check if the feature is enabled
  if (!base::FeatureList::IsEnabled(kAsolPageContextExtraction)) {
    std::move(callback).Run("");
    return;
  }
  
  // By default, extract visible content
  ExtractVisibleContent(std::move(callback));
}

void PageContextExtractor::ExtractSelectedText(ContextCallback callback) {
  // Track performance of this operation
  util::ScopedPerformanceTracker tracker("PageContextExtractor_ExtractSelectedText");
  
  // Check if the feature is enabled
  if (!base::FeatureList::IsEnabled(kAsolPageContextExtraction)) {
    std::move(callback).Run("");
    return;
  }
  
  ExecuteJavaScript(kExtractSelectedTextScript,
                   base::BindOnce(&PageContextExtractor::HandleJavaScriptResult,
                                 weak_ptr_factory_.GetWeakPtr(),
                                 std::move(callback)));
}

void PageContextExtractor::ExtractVisibleContent(ContextCallback callback) {
  // Track performance of this operation
  util::ScopedPerformanceTracker tracker("PageContextExtractor_ExtractVisibleContent");
  
  // Check if the feature is enabled
  if (!base::FeatureList::IsEnabled(kAsolPageContextExtraction)) {
    std::move(callback).Run("");
    return;
  }
  
  ExecuteJavaScript(kExtractVisibleContentScript,
                   base::BindOnce(&PageContextExtractor::HandleJavaScriptResult,
                                 weak_ptr_factory_.GetWeakPtr(),
                                 std::move(callback)));
}

void PageContextExtractor::ExtractFullPageContent(ContextCallback callback) {
  // Track performance of this operation
  util::ScopedPerformanceTracker tracker("PageContextExtractor_ExtractFullPageContent");
  
  // Check if the feature is enabled
  if (!base::FeatureList::IsEnabled(kAsolPageContextExtraction)) {
    std::move(callback).Run("");
    return;
  }
  
  ExecuteJavaScript(kExtractFullPageContentScript,
                   base::BindOnce(&PageContextExtractor::HandleJavaScriptResult,
                                 weak_ptr_factory_.GetWeakPtr(),
                                 std::move(callback)));
}

void PageContextExtractor::ExtractElementContent(
    const std::string& selector,
    ContextCallback callback) {
  // Track performance of this operation
  util::ScopedPerformanceTracker tracker("PageContextExtractor_ExtractElementContent");
  
  // Check if the feature is enabled
  if (!base::FeatureList::IsEnabled(kAsolPageContextExtraction)) {
    std::move(callback).Run("");
    return;
  }
  
  // Format the script with the selector
  std::string script = base::StringPrintf(kExtractElementContentScript, selector.c_str());
  
  ExecuteJavaScript(script,
                   base::BindOnce(&PageContextExtractor::HandleJavaScriptResult,
                                 weak_ptr_factory_.GetWeakPtr(),
                                 std::move(callback)));
}

void PageContextExtractor::WebContentsDestroyed() {
  // Nothing to do here
}

void PageContextExtractor::PrimaryPageChanged(content::Page& page) {
  // Nothing to do here
}

void PageContextExtractor::ExecuteJavaScript(
    const std::string& script,
    base::OnceCallback<void(const std::string&)> callback) {
  if (!web_contents()) {
    std::move(callback).Run("");
    return;
  }
  
  content::RenderFrameHost* main_frame = web_contents()->GetPrimaryMainFrame();
  if (!main_frame) {
    std::move(callback).Run("");
    return;
  }
  
  main_frame->ExecuteJavaScriptForTests(
      base::UTF8ToUTF16(script),
      base::BindOnce([](base::OnceCallback<void(const std::string&)> callback,
                       base::Value result) {
        std::string text;
        if (result.is_string()) {
          text = result.GetString();
        } else {
          // Try to convert the result to a string
          std::string json;
          base::JSONWriter::Write(result, &json);
          text = json;
        }
        
        // Limit the context length
        int max_length = base::GetFieldTrialParamByFeatureAsInt(
            kAsolPageContextExtraction,
            "max_context_length",
            5000);
        
        if (static_cast<int>(text.length()) > max_length) {
          text = text.substr(0, max_length) + "...";
        }
        
        std::move(callback).Run(text);
      },
      std::move(callback)));
}

void PageContextExtractor::HandleJavaScriptResult(
    ContextCallback callback,
    const std::string& result) {
  // Process the result if needed
  std::string processed_result = result;
  
  // Remove excessive whitespace
  base::TrimWhitespaceASCII(processed_result, base::TRIM_ALL, &processed_result);
  
  // Call the callback with the processed result
  std::move(callback).Run(processed_result);
}

}  // namespace browser
}  // namespace asol