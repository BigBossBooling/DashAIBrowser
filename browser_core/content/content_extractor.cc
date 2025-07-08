// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser_core/content/content_extractor.h"

#include <algorithm>
#include <regex>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/strings/string_util.h"
#include "base/strings/string_split.h"
#include "base/task/thread_pool.h"

namespace browser_core {
namespace content {

namespace {

// Helper function to remove HTML tags
std::string RemoveHtmlTags(const std::string& html) {
  std::regex tags("<[^>]*>");
  return std::regex_replace(html, tags, "");
}

// Helper function to decode HTML entities
std::string DecodeHtmlEntities(const std::string& html) {
  // Simple implementation for common entities
  std::string result = html;
  
  // Replace common HTML entities
  std::vector<std::pair<std::string, std::string>> entities = {
    {"&amp;", "&"},
    {"&lt;", "<"},
    {"&gt;", ">"},
    {"&quot;", "\""},
    {"&apos;", "'"},
    {"&nbsp;", " "},
  };
  
  for (const auto& entity : entities) {
    size_t pos = 0;
    while ((pos = result.find(entity.first, pos)) != std::string::npos) {
      result.replace(pos, entity.first.length(), entity.second);
      pos += entity.second.length();
    }
  }
  
  return result;
}

// Helper function to normalize whitespace
std::string NormalizeWhitespace(const std::string& text) {
  // Replace multiple whitespace characters with a single space
  std::regex whitespace("\\s+");
  std::string result = std::regex_replace(text, whitespace, " ");
  
  // Trim leading and trailing whitespace
  result = base::TrimString(result, " \t\n\r", base::TRIM_ALL);
  
  return result;
}

// Helper function to extract content between tags
std::string ExtractBetweenTags(const std::string& html, 
                             const std::string& start_tag,
                             const std::string& end_tag) {
  size_t start_pos = html.find(start_tag);
  if (start_pos == std::string::npos)
    return "";
  
  start_pos += start_tag.length();
  size_t end_pos = html.find(end_tag, start_pos);
  if (end_pos == std::string::npos)
    return "";
  
  return html.substr(start_pos, end_pos - start_pos);
}

// Helper function to extract all matches of a regex pattern
std::vector<std::string> ExtractAllMatches(const std::string& text,
                                         const std::regex& pattern) {
  std::vector<std::string> matches;
  std::smatch match;
  std::string::const_iterator search_start(text.cbegin());
  
  while (std::regex_search(search_start, text.cend(), match, pattern)) {
    if (match.size() > 1) {
      matches.push_back(match[1].str());
    } else if (match.size() > 0) {
      matches.push_back(match[0].str());
    }
    search_start = match.suffix().first;
  }
  
  return matches;
}

}  // namespace

ContentExtractor::ContentExtractor()
    : weak_ptr_factory_(this) {}

ContentExtractor::~ContentExtractor() = default;

bool ContentExtractor::Initialize() {
  // No initialization needed for now
  return true;
}

void ContentExtractor::ExtractContent(
    const std::string& page_url,
    const std::string& html_content,
    ExtractionCallback callback) {
  // Run extraction on a background thread
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock()},
      base::BindOnce(&ContentExtractor::ExtractContentSync,
                   base::Unretained(this),
                   page_url,
                   html_content),
      std::move(callback));
}

ContentExtractor::ExtractedContent ContentExtractor::ExtractContentSync(
    const std::string& page_url,
    const std::string& html_content) {
  ExtractedContent content;
  content.success = true;
  
  try {
    // Extract content components
    content.title = ExtractTitle(html_content);
    content.main_text = ExtractMainText(html_content);
    content.author = ExtractAuthor(html_content);
    content.date = ExtractDate(html_content);
    content.content_type = DetectContentType(html_content);
    content.paragraphs = ExtractParagraphs(html_content);
    content.headings = ExtractHeadings(html_content);
    content.images = ExtractImages(html_content);
    content.links = ExtractLinks(html_content);
  } catch (const std::exception& e) {
    content.success = false;
    content.error_message = "Error extracting content: " + std::string(e.what());
  }
  
  return content;
}

ContentExtractor::ContentType ContentExtractor::DetectContentType(
    const std::string& html_content) {
  // Simple content type detection based on keywords and structure
  
  // Check for article indicators
  std::regex article_pattern("<article|class=[\"']article|article-|post-|blog-");
  if (std::regex_search(html_content, article_pattern)) {
    return ContentType::ARTICLE;
  }
  
  // Check for product indicators
  std::regex product_pattern("product-|price|add to cart|buy now");
  if (std::regex_search(html_content, product_pattern)) {
    return ContentType::PRODUCT;
  }
  
  // Check for documentation indicators
  std::regex doc_pattern("documentation|api-|reference-|manual-|guide-");
  if (std::regex_search(html_content, doc_pattern)) {
    return ContentType::DOCUMENTATION;
  }
  
  // Check for forum indicators
  std::regex forum_pattern("forum-|thread-|post-|comment-|discussion-");
  if (std::regex_search(html_content, forum_pattern)) {
    return ContentType::FORUM;
  }
  
  // Check for social media indicators
  std::regex social_pattern("social-|tweet-|status-|feed-|profile-");
  if (std::regex_search(html_content, social_pattern)) {
    return ContentType::SOCIAL;
  }
  
  // Count different content indicators
  int article_count = 0;
  int product_count = 0;
  int doc_count = 0;
  int forum_count = 0;
  int social_count = 0;
  
  // Count headings
  std::regex h_pattern("<h[1-6][^>]*>(.*?)</h[1-6]>");
  article_count += ExtractAllMatches(html_content, h_pattern).size() / 2;
  
  // Count paragraphs
  std::regex p_pattern("<p[^>]*>(.*?)</p>");
  article_count += ExtractAllMatches(html_content, p_pattern).size() / 3;
  
  // Count product indicators
  std::regex price_pattern("price|cost|\\$|€|£");
  product_count += ExtractAllMatches(html_content, price_pattern).size() / 2;
  
  // Count documentation indicators
  std::regex code_pattern("<code|<pre|function|method|class|example");
  doc_count += ExtractAllMatches(html_content, code_pattern).size() / 2;
  
  // Count forum indicators
  std::regex comment_pattern("comment|reply|post|thread");
  forum_count += ExtractAllMatches(html_content, comment_pattern).size() / 2;
  
  // Count social indicators
  std::regex share_pattern("share|like|follow|tweet|status");
  social_count += ExtractAllMatches(html_content, share_pattern).size() / 2;
  
  // Determine the dominant content type
  int max_count = std::max({article_count, product_count, doc_count, 
                          forum_count, social_count});
  
  if (max_count == 0) {
    return ContentType::UNKNOWN;
  }
  
  // Check if there are multiple dominant types
  int dominant_types = 0;
  if (article_count == max_count) dominant_types++;
  if (product_count == max_count) dominant_types++;
  if (doc_count == max_count) dominant_types++;
  if (forum_count == max_count) dominant_types++;
  if (social_count == max_count) dominant_types++;
  
  if (dominant_types > 1) {
    return ContentType::MIXED;
  }
  
  // Return the dominant type
  if (article_count == max_count) return ContentType::ARTICLE;
  if (product_count == max_count) return ContentType::PRODUCT;
  if (doc_count == max_count) return ContentType::DOCUMENTATION;
  if (forum_count == max_count) return ContentType::FORUM;
  if (social_count == max_count) return ContentType::SOCIAL;
  
  return ContentType::UNKNOWN;
}

std::string ContentExtractor::CleanContent(const std::string& html_content) {
  // Remove HTML tags
  std::string clean_text = RemoveHtmlTags(html_content);
  
  // Decode HTML entities
  clean_text = DecodeHtmlEntities(clean_text);
  
  // Normalize whitespace
  clean_text = NormalizeWhitespace(clean_text);
  
  return clean_text;
}

base::WeakPtr<ContentExtractor> ContentExtractor::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

std::string ContentExtractor::ExtractMainText(const std::string& html_content) {
  // Try to extract content from article tag
  std::string article_content = 
      ExtractBetweenTags(html_content, "<article", "</article>");
  if (!article_content.empty()) {
    return CleanContent(article_content);
  }
  
  // Try to extract content from main tag
  std::string main_content = 
      ExtractBetweenTags(html_content, "<main", "</main>");
  if (!main_content.empty()) {
    return CleanContent(main_content);
  }
  
  // Try to extract content from div with content-related class
  std::regex content_div("<div[^>]*class=[\"'][^\"']*content[^\"']*[\"'][^>]*>(.*?)</div>");
  std::smatch match;
  if (std::regex_search(html_content, match, content_div) && match.size() > 1) {
    return CleanContent(match[1].str());
  }
  
  // Extract all paragraphs
  std::regex p_pattern("<p[^>]*>(.*?)</p>");
  std::vector<std::string> paragraphs = ExtractAllMatches(html_content, p_pattern);
  
  // Combine paragraphs
  std::string combined_text;
  for (const auto& p : paragraphs) {
    std::string clean_p = CleanContent(p);
    if (!clean_p.empty()) {
      if (!combined_text.empty()) {
        combined_text += "\n\n";
      }
      combined_text += clean_p;
    }
  }
  
  return combined_text;
}

std::string ContentExtractor::ExtractTitle(const std::string& html_content) {
  // Try to extract title from title tag
  std::string title = 
      ExtractBetweenTags(html_content, "<title", "</title>");
  if (!title.empty()) {
    return CleanContent(title);
  }
  
  // Try to extract title from h1 tag
  std::regex h1_pattern("<h1[^>]*>(.*?)</h1>");
  std::smatch match;
  if (std::regex_search(html_content, match, h1_pattern) && match.size() > 1) {
    return CleanContent(match[1].str());
  }
  
  return "";
}

std::string ContentExtractor::ExtractAuthor(const std::string& html_content) {
  // Try to extract author from meta tag
  std::regex author_meta("<meta[^>]*name=[\"']author[\"'][^>]*content=[\"']([^\"']*)[\"'][^>]*>");
  std::smatch match;
  if (std::regex_search(html_content, match, author_meta) && match.size() > 1) {
    return match[1].str();
  }
  
  // Try to extract author from author-related elements
  std::regex author_elem("<[^>]*class=[\"'][^\"']*author[^\"']*[\"'][^>]*>(.*?)</");
  if (std::regex_search(html_content, match, author_elem) && match.size() > 1) {
    return CleanContent(match[1].str());
  }
  
  return "";
}

std::string ContentExtractor::ExtractDate(const std::string& html_content) {
  // Try to extract date from meta tag
  std::regex date_meta("<meta[^>]*name=[\"']date[\"'][^>]*content=[\"']([^\"']*)[\"'][^>]*>");
  std::smatch match;
  if (std::regex_search(html_content, match, date_meta) && match.size() > 1) {
    return match[1].str();
  }
  
  // Try to extract date from time tag
  std::regex time_tag("<time[^>]*datetime=[\"']([^\"']*)[\"'][^>]*>");
  if (std::regex_search(html_content, match, time_tag) && match.size() > 1) {
    return match[1].str();
  }
  
  // Try to extract date from date-related elements
  std::regex date_elem("<[^>]*class=[\"'][^\"']*date[^\"']*[\"'][^>]*>(.*?)</");
  if (std::regex_search(html_content, match, date_elem) && match.size() > 1) {
    return CleanContent(match[1].str());
  }
  
  return "";
}

std::vector<std::string> ContentExtractor::ExtractParagraphs(
    const std::string& html_content) {
  // Extract all paragraphs
  std::regex p_pattern("<p[^>]*>(.*?)</p>");
  std::vector<std::string> paragraphs = ExtractAllMatches(html_content, p_pattern);
  
  // Clean paragraphs
  std::vector<std::string> clean_paragraphs;
  for (const auto& p : paragraphs) {
    std::string clean_p = CleanContent(p);
    if (!clean_p.empty()) {
      clean_paragraphs.push_back(clean_p);
    }
  }
  
  return clean_paragraphs;
}

std::vector<std::string> ContentExtractor::ExtractHeadings(
    const std::string& html_content) {
  // Extract all headings
  std::vector<std::string> headings;
  
  for (int i = 1; i <= 6; i++) {
    std::regex h_pattern("<h" + std::to_string(i) + "[^>]*>(.*?)</h" + 
                       std::to_string(i) + ">");
    std::vector<std::string> h_matches = ExtractAllMatches(html_content, h_pattern);
    
    for (const auto& h : h_matches) {
      std::string clean_h = CleanContent(h);
      if (!clean_h.empty()) {
        headings.push_back(clean_h);
      }
    }
  }
  
  return headings;
}

std::vector<std::string> ContentExtractor::ExtractImages(
    const std::string& html_content) {
  // Extract all image URLs
  std::regex img_pattern("<img[^>]*src=[\"']([^\"']*)[\"'][^>]*>");
  return ExtractAllMatches(html_content, img_pattern);
}

std::vector<std::string> ContentExtractor::ExtractLinks(
    const std::string& html_content) {
  // Extract all link URLs
  std::regex link_pattern("<a[^>]*href=[\"']([^\"']*)[\"'][^>]*>");
  return ExtractAllMatches(html_content, link_pattern);
}

}  // namespace content
}  // namespace browser_core// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser_core/content/content_extractor.h"

#include <algorithm>
#include <regex>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/strings/string_util.h"
#include "base/strings/string_split.h"
#include "base/task/thread_pool.h"

namespace browser_core {
namespace content {

namespace {

// Helper function to remove HTML tags
std::string RemoveHtmlTags(const std::string& html) {
  std::regex tags("<[^>]*>");
  return std::regex_replace(html, tags, "");
}

// Helper function to decode HTML entities
std::string DecodeHtmlEntities(const std::string& html) {
  // Simple implementation for common entities
  std::string result = html;
  
  // Replace common HTML entities
  std::vector<std::pair<std::string, std::string>> entities = {
    {"&amp;", "&"},
    {"&lt;", "<"},
    {"&gt;", ">"},
    {"&quot;", "\""},
    {"&apos;", "'"},
    {"&nbsp;", " "},
  };
  
  for (const auto& entity : entities) {
    size_t pos = 0;
    while ((pos = result.find(entity.first, pos)) != std::string::npos) {
      result.replace(pos, entity.first.length(), entity.second);
      pos += entity.second.length();
    }
  }
  
  return result;
}

// Helper function to normalize whitespace
std::string NormalizeWhitespace(const std::string& text) {
  // Replace multiple whitespace characters with a single space
  std::regex whitespace("\\s+");
  std::string result = std::regex_replace(text, whitespace, " ");
  
  // Trim leading and trailing whitespace
  result = base::TrimString(result, " \t\n\r", base::TRIM_ALL);
  
  return result;
}

// Helper function to extract content between tags
std::string ExtractBetweenTags(const std::string& html, 
                             const std::string& start_tag,
                             const std::string& end_tag) {
  size_t start_pos = html.find(start_tag);
  if (start_pos == std::string::npos)
    return "";
  
  start_pos += start_tag.length();
  size_t end_pos = html.find(end_tag, start_pos);
  if (end_pos == std::string::npos)
    return "";
  
  return html.substr(start_pos, end_pos - start_pos);
}

// Helper function to extract all matches of a regex pattern
std::vector<std::string> ExtractAllMatches(const std::string& text,
                                         const std::regex& pattern) {
  std::vector<std::string> matches;
  std::smatch match;
  std::string::const_iterator search_start(text.cbegin());
  
  while (std::regex_search(search_start, text.cend(), match, pattern)) {
    if (match.size() > 1) {
      matches.push_back(match[1].str());
    } else if (match.size() > 0) {
      matches.push_back(match[0].str());
    }
    search_start = match.suffix().first;
  }
  
  return matches;
}

}  // namespace

ContentExtractor::ContentExtractor()
    : weak_ptr_factory_(this) {}

ContentExtractor::~ContentExtractor() = default;

bool ContentExtractor::Initialize() {
  // No initialization needed for now
  return true;
}

void ContentExtractor::ExtractContent(
    const std::string& page_url,
    const std::string& html_content,
    ExtractionCallback callback) {
  // Run extraction on a background thread
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock()},
      base::BindOnce(&ContentExtractor::ExtractContentSync,
                   base::Unretained(this),
                   page_url,
                   html_content),
      std::move(callback));
}

ContentExtractor::ExtractedContent ContentExtractor::ExtractContentSync(
    const std::string& page_url,
    const std::string& html_content) {
  ExtractedContent content;
  content.success = true;
  
  try {
    // Extract content components
    content.title = ExtractTitle(html_content);
    content.main_text = ExtractMainText(html_content);
    content.author = ExtractAuthor(html_content);
    content.date = ExtractDate(html_content);
    content.content_type = DetectContentType(html_content);
    content.paragraphs = ExtractParagraphs(html_content);
    content.headings = ExtractHeadings(html_content);
    content.images = ExtractImages(html_content);
    content.links = ExtractLinks(html_content);
  } catch (const std::exception& e) {
    content.success = false;
    content.error_message = "Error extracting content: " + std::string(e.what());
  }
  
  return content;
}

ContentExtractor::ContentType ContentExtractor::DetectContentType(
    const std::string& html_content) {
  // Simple content type detection based on keywords and structure
  
  // Check for article indicators
  std::regex article_pattern("<article|class=[\"']article|article-|post-|blog-");
  if (std::regex_search(html_content, article_pattern)) {
    return ContentType::ARTICLE;
  }
  
  // Check for product indicators
  std::regex product_pattern("product-|price|add to cart|buy now");
  if (std::regex_search(html_content, product_pattern)) {
    return ContentType::PRODUCT;
  }
  
  // Check for documentation indicators
  std::regex doc_pattern("documentation|api-|reference-|manual-|guide-");
  if (std::regex_search(html_content, doc_pattern)) {
    return ContentType::DOCUMENTATION;
  }
  
  // Check for forum indicators
  std::regex forum_pattern("forum-|thread-|post-|comment-|discussion-");
  if (std::regex_search(html_content, forum_pattern)) {
    return ContentType::FORUM;
  }
  
  // Check for social media indicators
  std::regex social_pattern("social-|tweet-|status-|feed-|profile-");
  if (std::regex_search(html_content, social_pattern)) {
    return ContentType::SOCIAL;
  }
  
  // Count different content indicators
  int article_count = 0;
  int product_count = 0;
  int doc_count = 0;
  int forum_count = 0;
  int social_count = 0;
  
  // Count headings
  std::regex h_pattern("<h[1-6][^>]*>(.*?)</h[1-6]>");
  article_count += ExtractAllMatches(html_content, h_pattern).size() / 2;
  
  // Count paragraphs
  std::regex p_pattern("<p[^>]*>(.*?)</p>");
  article_count += ExtractAllMatches(html_content, p_pattern).size() / 3;
  
  // Count product indicators
  std::regex price_pattern("price|cost|\\$|€|£");
  product_count += ExtractAllMatches(html_content, price_pattern).size() / 2;
  
  // Count documentation indicators
  std::regex code_pattern("<code|<pre|function|method|class|example");
  doc_count += ExtractAllMatches(html_content, code_pattern).size() / 2;
  
  // Count forum indicators
  std::regex comment_pattern("comment|reply|post|thread");
  forum_count += ExtractAllMatches(html_content, comment_pattern).size() / 2;
  
  // Count social indicators
  std::regex share_pattern("share|like|follow|tweet|status");
  social_count += ExtractAllMatches(html_content, share_pattern).size() / 2;
  
  // Determine the dominant content type
  int max_count = std::max({article_count, product_count, doc_count, 
                          forum_count, social_count});
  
  if (max_count == 0) {
    return ContentType::UNKNOWN;
  }
  
  // Check if there are multiple dominant types
  int dominant_types = 0;
  if (article_count == max_count) dominant_types++;
  if (product_count == max_count) dominant_types++;
  if (doc_count == max_count) dominant_types++;
  if (forum_count == max_count) dominant_types++;
  if (social_count == max_count) dominant_types++;
  
  if (dominant_types > 1) {
    return ContentType::MIXED;
  }
  
  // Return the dominant type
  if (article_count == max_count) return ContentType::ARTICLE;
  if (product_count == max_count) return ContentType::PRODUCT;
  if (doc_count == max_count) return ContentType::DOCUMENTATION;
  if (forum_count == max_count) return ContentType::FORUM;
  if (social_count == max_count) return ContentType::SOCIAL;
  
  return ContentType::UNKNOWN;
}

std::string ContentExtractor::CleanContent(const std::string& html_content) {
  // Remove HTML tags
  std::string clean_text = RemoveHtmlTags(html_content);
  
  // Decode HTML entities
  clean_text = DecodeHtmlEntities(clean_text);
  
  // Normalize whitespace
  clean_text = NormalizeWhitespace(clean_text);
  
  return clean_text;
}

base::WeakPtr<ContentExtractor> ContentExtractor::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

std::string ContentExtractor::ExtractMainText(const std::string& html_content) {
  // Try to extract content from article tag
  std::string article_content = 
      ExtractBetweenTags(html_content, "<article", "</article>");
  if (!article_content.empty()) {
    return CleanContent(article_content);
  }
  
  // Try to extract content from main tag
  std::string main_content = 
      ExtractBetweenTags(html_content, "<main", "</main>");
  if (!main_content.empty()) {
    return CleanContent(main_content);
  }
  
  // Try to extract content from div with content-related class
  std::regex content_div("<div[^>]*class=[\"'][^\"']*content[^\"']*[\"'][^>]*>(.*?)</div>");
  std::smatch match;
  if (std::regex_search(html_content, match, content_div) && match.size() > 1) {
    return CleanContent(match[1].str());
  }
  
  // Extract all paragraphs
  std::regex p_pattern("<p[^>]*>(.*?)</p>");
  std::vector<std::string> paragraphs = ExtractAllMatches(html_content, p_pattern);
  
  // Combine paragraphs
  std::string combined_text;
  for (const auto& p : paragraphs) {
    std::string clean_p = CleanContent(p);
    if (!clean_p.empty()) {
      if (!combined_text.empty()) {
        combined_text += "\n\n";
      }
      combined_text += clean_p;
    }
  }
  
  return combined_text;
}

std::string ContentExtractor::ExtractTitle(const std::string& html_content) {
  // Try to extract title from title tag
  std::string title = 
      ExtractBetweenTags(html_content, "<title", "</title>");
  if (!title.empty()) {
    return CleanContent(title);
  }
  
  // Try to extract title from h1 tag
  std::regex h1_pattern("<h1[^>]*>(.*?)</h1>");
  std::smatch match;
  if (std::regex_search(html_content, match, h1_pattern) && match.size() > 1) {
    return CleanContent(match[1].str());
  }
  
  return "";
}

std::string ContentExtractor::ExtractAuthor(const std::string& html_content) {
  // Try to extract author from meta tag
  std::regex author_meta("<meta[^>]*name=[\"']author[\"'][^>]*content=[\"']([^\"']*)[\"'][^>]*>");
  std::smatch match;
  if (std::regex_search(html_content, match, author_meta) && match.size() > 1) {
    return match[1].str();
  }
  
  // Try to extract author from author-related elements
  std::regex author_elem("<[^>]*class=[\"'][^\"']*author[^\"']*[\"'][^>]*>(.*?)</");
  if (std::regex_search(html_content, match, author_elem) && match.size() > 1) {
    return CleanContent(match[1].str());
  }
  
  return "";
}

std::string ContentExtractor::ExtractDate(const std::string& html_content) {
  // Try to extract date from meta tag
  std::regex date_meta("<meta[^>]*name=[\"']date[\"'][^>]*content=[\"']([^\"']*)[\"'][^>]*>");
  std::smatch match;
  if (std::regex_search(html_content, match, date_meta) && match.size() > 1) {
    return match[1].str();
  }
  
  // Try to extract date from time tag
  std::regex time_tag("<time[^>]*datetime=[\"']([^\"']*)[\"'][^>]*>");
  if (std::regex_search(html_content, match, time_tag) && match.size() > 1) {
    return match[1].str();
  }
  
  // Try to extract date from date-related elements
  std::regex date_elem("<[^>]*class=[\"'][^\"']*date[^\"']*[\"'][^>]*>(.*?)</");
  if (std::regex_search(html_content, match, date_elem) && match.size() > 1) {
    return CleanContent(match[1].str());
  }
  
  return "";
}

std::vector<std::string> ContentExtractor::ExtractParagraphs(
    const std::string& html_content) {
  // Extract all paragraphs
  std::regex p_pattern("<p[^>]*>(.*?)</p>");
  std::vector<std::string> paragraphs = ExtractAllMatches(html_content, p_pattern);
  
  // Clean paragraphs
  std::vector<std::string> clean_paragraphs;
  for (const auto& p : paragraphs) {
    std::string clean_p = CleanContent(p);
    if (!clean_p.empty()) {
      clean_paragraphs.push_back(clean_p);
    }
  }
  
  return clean_paragraphs;
}

std::vector<std::string> ContentExtractor::ExtractHeadings(
    const std::string& html_content) {
  // Extract all headings
  std::vector<std::string> headings;
  
  for (int i = 1; i <= 6; i++) {
    std::regex h_pattern("<h" + std::to_string(i) + "[^>]*>(.*?)</h" + 
                       std::to_string(i) + ">");
    std::vector<std::string> h_matches = ExtractAllMatches(html_content, h_pattern);
    
    for (const auto& h : h_matches) {
      std::string clean_h = CleanContent(h);
      if (!clean_h.empty()) {
        headings.push_back(clean_h);
      }
    }
  }
  
  return headings;
}

std::vector<std::string> ContentExtractor::ExtractImages(
    const std::string& html_content) {
  // Extract all image URLs
  std::regex img_pattern("<img[^>]*src=[\"']([^\"']*)[\"'][^>]*>");
  return ExtractAllMatches(html_content, img_pattern);
}

std::vector<std::string> ContentExtractor::ExtractLinks(
    const std::string& html_content) {
  // Extract all link URLs
  std::regex link_pattern("<a[^>]*href=[\"']([^\"']*)[\"'][^>]*>");
  return ExtractAllMatches(html_content, link_pattern);
}

}  // namespace content
}  // namespace browser_core