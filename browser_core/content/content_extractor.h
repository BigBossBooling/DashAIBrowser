// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_CORE_CONTENT_CONTENT_EXTRACTOR_H_
#define BROWSER_CORE_CONTENT_CONTENT_EXTRACTOR_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"

namespace browser_core {
namespace content {

// ContentExtractor extracts and processes content from web pages.
// It provides clean, structured content for AI features like summarization.
class ContentExtractor {
 public:
  // Content type
  enum class ContentType {
    ARTICLE,      // News article, blog post, etc.
    PRODUCT,      // Product page
    DOCUMENTATION, // Technical documentation
    FORUM,        // Forum or discussion
    SOCIAL,       // Social media content
    MIXED,        // Mixed content
    UNKNOWN       // Unknown content type
  };

  // Extracted content
  struct ExtractedContent {
    std::string title;
    std::string main_text;
    std::string author;
    std::string date;
    ContentType content_type;
    std::vector<std::string> paragraphs;
    std::vector<std::string> headings;
    std::vector<std::string> images;
    std::vector<std::string> links;
    bool success;
    std::string error_message;
  };

  // Callback for content extraction
  using ExtractionCallback = 
      base::OnceCallback<void(const ExtractedContent& content)>;

  ContentExtractor();
  ~ContentExtractor();

  // Disallow copy and assign
  ContentExtractor(const ContentExtractor&) = delete;
  ContentExtractor& operator=(const ContentExtractor&) = delete;

  // Initialize the extractor
  bool Initialize();

  // Extract content from a web page
  void ExtractContent(const std::string& page_url,
                    const std::string& html_content,
                    ExtractionCallback callback);

  // Extract content synchronously
  ExtractedContent ExtractContentSync(const std::string& page_url,
                                    const std::string& html_content);

  // Detect content type
  ContentType DetectContentType(const std::string& html_content);

  // Clean and normalize content
  std::string CleanContent(const std::string& html_content);

  // Get a weak pointer to this instance
  base::WeakPtr<ContentExtractor> GetWeakPtr();

 private:
  // Extract main text content
  std::string ExtractMainText(const std::string& html_content);

  // Extract title
  std::string ExtractTitle(const std::string& html_content);

  // Extract author
  std::string ExtractAuthor(const std::string& html_content);

  // Extract date
  std::string ExtractDate(const std::string& html_content);

  // Extract paragraphs
  std::vector<std::string> ExtractParagraphs(const std::string& html_content);

  // Extract headings
  std::vector<std::string> ExtractHeadings(const std::string& html_content);

  // Extract images
  std::vector<std::string> ExtractImages(const std::string& html_content);

  // Extract links
  std::vector<std::string> ExtractLinks(const std::string& html_content);

  // For weak pointers
  base::WeakPtrFactory<ContentExtractor> weak_ptr_factory_{this};
};

}  // namespace content
}  // namespace browser_core

#endif  // BROWSER_CORE_CONTENT_CONTENT_EXTRACTOR_H_// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_CORE_CONTENT_CONTENT_EXTRACTOR_H_
#define BROWSER_CORE_CONTENT_CONTENT_EXTRACTOR_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"

namespace browser_core {
namespace content {

// ContentExtractor extracts and processes content from web pages.
// It provides clean, structured content for AI features like summarization.
class ContentExtractor {
 public:
  // Content type
  enum class ContentType {
    ARTICLE,      // News article, blog post, etc.
    PRODUCT,      // Product page
    DOCUMENTATION, // Technical documentation
    FORUM,        // Forum or discussion
    SOCIAL,       // Social media content
    MIXED,        // Mixed content
    UNKNOWN       // Unknown content type
  };

  // Extracted content
  struct ExtractedContent {
    std::string title;
    std::string main_text;
    std::string author;
    std::string date;
    ContentType content_type;
    std::vector<std::string> paragraphs;
    std::vector<std::string> headings;
    std::vector<std::string> images;
    std::vector<std::string> links;
    bool success;
    std::string error_message;
  };

  // Callback for content extraction
  using ExtractionCallback = 
      base::OnceCallback<void(const ExtractedContent& content)>;

  ContentExtractor();
  ~ContentExtractor();

  // Disallow copy and assign
  ContentExtractor(const ContentExtractor&) = delete;
  ContentExtractor& operator=(const ContentExtractor&) = delete;

  // Initialize the extractor
  bool Initialize();

  // Extract content from a web page
  void ExtractContent(const std::string& page_url,
                    const std::string& html_content,
                    ExtractionCallback callback);

  // Extract content synchronously
  ExtractedContent ExtractContentSync(const std::string& page_url,
                                    const std::string& html_content);

  // Detect content type
  ContentType DetectContentType(const std::string& html_content);

  // Clean and normalize content
  std::string CleanContent(const std::string& html_content);

  // Get a weak pointer to this instance
  base::WeakPtr<ContentExtractor> GetWeakPtr();

 private:
  // Extract main text content
  std::string ExtractMainText(const std::string& html_content);

  // Extract title
  std::string ExtractTitle(const std::string& html_content);

  // Extract author
  std::string ExtractAuthor(const std::string& html_content);

  // Extract date
  std::string ExtractDate(const std::string& html_content);

  // Extract paragraphs
  std::vector<std::string> ExtractParagraphs(const std::string& html_content);

  // Extract headings
  std::vector<std::string> ExtractHeadings(const std::string& html_content);

  // Extract images
  std::vector<std::string> ExtractImages(const std::string& html_content);

  // Extract links
  std::vector<std::string> ExtractLinks(const std::string& html_content);

  // For weak pointers
  base::WeakPtrFactory<ContentExtractor> weak_ptr_factory_{this};
};

}  // namespace content
}  // namespace browser_core

#endif  // BROWSER_CORE_CONTENT_CONTENT_EXTRACTOR_H_