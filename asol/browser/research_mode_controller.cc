// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "asol/browser/research_mode_controller.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "asol/browser/browser_features.h"
#include "asol/core/service_manager.h"
#include "asol/util/performance_tracker.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/guid.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "base/values.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"

namespace asol {
namespace browser {

namespace {

// Feature for research mode
const base::Feature kAsolResearchMode{"AsolResearchMode",
                                     base::FEATURE_ENABLED_BY_DEFAULT};

// Parameter for auto-adding pages to research session
const base::FeatureParam<bool> kAsolAutoAddPagesToResearchParam{
    &kAsolResearchMode, "auto_add_pages", false};

// Parameter for auto-generating key points
const base::FeatureParam<bool> kAsolAutoGenerateKeyPointsParam{
    &kAsolResearchMode, "auto_generate_key_points", false};

// Parameter for max pages per session
const base::FeatureParam<int> kAsolMaxPagesPerSessionParam{
    &kAsolResearchMode, "max_pages_per_session", 100};

// Convert a ResearchSession to a base::Value
base::Value::Dict ResearchSessionToValue(const ResearchSession& session) {
  base::Value::Dict session_dict;
  session_dict.Set("id", session.id);
  session_dict.Set("name", session.name);
  session_dict.Set("topic", session.topic);
  session_dict.Set("created", session.created.ToDoubleT());
  session_dict.Set("last_updated", session.last_updated.ToDoubleT());
  
  base::Value::List pages_list;
  for (const auto& page : session.pages) {
    base::Value::Dict page_dict;
    page_dict.Set("url", page.url);
    page_dict.Set("title", page.title);
    page_dict.Set("content", page.content);
    page_dict.Set("timestamp", page.timestamp.ToDoubleT());
    page_dict.Set("is_processed", page.is_processed);
    
    base::Value::List key_points_list;
    for (const auto& point : page.key_points) {
      key_points_list.Append(point);
    }
    page_dict.Set("key_points", std::move(key_points_list));
    
    pages_list.Append(std::move(page_dict));
  }
  session_dict.Set("pages", std::move(pages_list));
  
  return session_dict;
}

// Convert a base::Value to a ResearchSession
ResearchSession ValueToResearchSession(const base::Value::Dict& value) {
  ResearchSession session;
  session.id = value.FindString("id").value_or("");
  session.name = value.FindString("name").value_or("");
  session.topic = value.FindString("topic").value_or("");
  
  if (const auto* created = value.FindDouble("created")) {
    session.created = base::Time::FromDoubleT(*created);
  } else {
    session.created = base::Time::Now();
  }
  
  if (const auto* last_updated = value.FindDouble("last_updated")) {
    session.last_updated = base::Time::FromDoubleT(*last_updated);
  } else {
    session.last_updated = base::Time::Now();
  }
  
  if (const auto* pages_list = value.FindList("pages")) {
    for (const auto& page_value : *pages_list) {
      if (!page_value.is_dict()) continue;
      
      const auto& page_dict = page_value.GetDict();
      ResearchPageData page;
      page.url = page_dict.FindString("url").value_or("");
      page.title = page_dict.FindString("title").value_or("");
      page.content = page_dict.FindString("content").value_or("");
      
      if (const auto* timestamp = page_dict.FindDouble("timestamp")) {
        page.timestamp = base::Time::FromDoubleT(*timestamp);
      } else {
        page.timestamp = base::Time::Now();
      }
      
      page.is_processed = page_dict.FindBool("is_processed").value_or(false);
      
      if (const auto* key_points_list = page_dict.FindList("key_points")) {
        for (const auto& point_value : *key_points_list) {
          if (point_value.is_string()) {
            page.key_points.push_back(point_value.GetString());
          }
        }
      }
      
      session.pages.push_back(std::move(page));
    }
  }
  
  return session;
}

// Get the file path for storing research sessions
base::FilePath GetResearchSessionsFilePath(content::BrowserContext* context) {
  base::FilePath path;
  if (!base::PathService::Get(base::DIR_USER_DATA, &path)) {
    return base::FilePath();
  }
  
  path = path.AppendASCII("asol_research_sessions.json");
  return path;
}

// Prompt for generating key points from content
const char kKeyPointsPrompt[] = 
    "Extract 3-5 key points from the following content. "
    "Focus on the most important information and present each point "
    "as a concise bullet point:\n\n%s";

}  // namespace

// static
WEB_CONTENTS_USER_DATA_KEY_IMPL(ResearchModeController);

ResearchModeController::ResearchModeController(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<ResearchModeController>(*web_contents) {
  DLOG(INFO) << "ResearchModeController created for WebContents: " << web_contents;
  
  // Create the context extractor
  context_extractor_ = std::make_unique<PageContextExtractor>(web_contents);
  
  // Load existing research sessions
  LoadSessions();
  
  // Create a default session if none exists
  if (sessions_.empty()) {
    CreateResearchSession("Default Research", "General Research");
  } else {
    // Use the most recently updated session as the current session
    auto latest_it = std::max_element(
        sessions_.begin(), sessions_.end(),
        [](const ResearchSession& a, const ResearchSession& b) {
          return a.last_updated < b.last_updated;
        });
    
    if (latest_it != sessions_.end()) {
      current_session_id_ = latest_it->id;
    }
  }
}

ResearchModeController::~ResearchModeController() {
  DLOG(INFO) << "ResearchModeController destroyed";
  
  // Save research sessions before destruction
  SaveSessions();
}

void ResearchModeController::SetResearchModeEnabled(bool enabled) {
  research_mode_enabled_ = enabled;
  DLOG(INFO) << "Research mode " << (enabled ? "enabled" : "disabled");
}

bool ResearchModeController::IsResearchModeEnabled() const {
  return research_mode_enabled_;
}

void ResearchModeController::CreateResearchSession(
    const std::string& name, const std::string& topic) {
  ResearchSession session;
  session.id = GenerateSessionId();
  session.name = name;
  session.topic = topic;
  session.created = base::Time::Now();
  session.last_updated = session.created;
  
  sessions_.push_back(std::move(session));
  current_session_id_ = sessions_.back().id;
  
  DLOG(INFO) << "Created research session: " << name << " (ID: " << current_session_id_ << ")";
  
  // Save the updated sessions
  SaveSessions();
}

const ResearchSession* ResearchModeController::GetCurrentSession() const {
  auto it = std::find_if(
      sessions_.begin(), sessions_.end(),
      [this](const ResearchSession& session) {
        return session.id == current_session_id_;
      });
  
  if (it != sessions_.end()) {
    return &(*it);
  }
  
  return nullptr;
}

void ResearchModeController::GetAllSessions(ResearchSessionsCallback callback) {
  std::move(callback).Run(sessions_);
}

void ResearchModeController::SwitchSession(const std::string& session_id) {
  auto it = std::find_if(
      sessions_.begin(), sessions_.end(),
      [&session_id](const ResearchSession& session) {
        return session.id == session_id;
      });
  
  if (it != sessions_.end()) {
    current_session_id_ = session_id;
    DLOG(INFO) << "Switched to research session: " << it->name << " (ID: " << session_id << ")";
  } else {
    DLOG(WARNING) << "Research session not found: " << session_id;
  }
}

void ResearchModeController::AddCurrentPageToSession() {
  if (!web_contents()) {
    return;
  }
  
  content::NavigationEntry* entry = 
      web_contents()->GetController().GetLastCommittedEntry();
  if (!entry) {
    return;
  }
  
  std::string url = entry->GetURL().spec();
  std::string title = base::UTF16ToUTF8(entry->GetTitle());
  
  ProcessPage(url, title, base::BindOnce(
      &ResearchModeController::HandleExtractedContext,
      weak_ptr_factory_.GetWeakPtr(),
      url, title));
}

void ResearchModeController::AddPageToSession(
    const std::string& url, const std::string& title, const std::string& content) {
  if (!IsResearchModeEnabled() || current_session_id_.empty()) {
    return;
  }
  
  auto it = std::find_if(
      sessions_.begin(), sessions_.end(),
      [this](const ResearchSession& session) {
        return session.id == current_session_id_;
      });
  
  if (it == sessions_.end()) {
    DLOG(WARNING) << "Current research session not found";
    return;
  }
  
  // Check if the page already exists in the session
  auto page_it = std::find_if(
      it->pages.begin(), it->pages.end(),
      [&url](const ResearchPageData& page) {
        return page.url == url;
      });
  
  if (page_it != it->pages.end()) {
    // Update the existing page
    page_it->title = title;
    page_it->content = content;
    page_it->timestamp = base::Time::Now();
    DLOG(INFO) << "Updated page in research session: " << title;
  } else {
    // Add a new page
    ResearchPageData page;
    page.url = url;
    page.title = title;
    page.content = content;
    page.timestamp = base::Time::Now();
    
    // Check if we've reached the maximum number of pages
    int max_pages = base::GetFieldTrialParamByFeatureAsInt(
        kAsolResearchMode, "max_pages_per_session", 100);
    
    if (static_cast<int>(it->pages.size()) >= max_pages) {
      // Remove the oldest page
      auto oldest_it = std::min_element(
          it->pages.begin(), it->pages.end(),
          [](const ResearchPageData& a, const ResearchPageData& b) {
            return a.timestamp < b.timestamp;
          });
      
      if (oldest_it != it->pages.end()) {
        DLOG(INFO) << "Removing oldest page to make room: " << oldest_it->title;
        it->pages.erase(oldest_it);
      }
    }
    
    it->pages.push_back(std::move(page));
    DLOG(INFO) << "Added page to research session: " << title;
  }
  
  // Update the session's last updated time
  it->last_updated = base::Time::Now();
  
  // Save the updated sessions
  SaveSessions();
  
  // Auto-generate key points if enabled
  if (base::GetFieldTrialParamByFeatureAsBool(
          kAsolResearchMode, "auto_generate_key_points", false)) {
    GenerateKeyPoints(url, base::DoNothing());
  }
}

void ResearchModeController::RemovePageFromSession(const std::string& url) {
  if (current_session_id_.empty()) {
    return;
  }
  
  auto it = std::find_if(
      sessions_.begin(), sessions_.end(),
      [this](const ResearchSession& session) {
        return session.id == current_session_id_;
      });
  
  if (it == sessions_.end()) {
    DLOG(WARNING) << "Current research session not found";
    return;
  }
  
  // Find and remove the page
  auto page_it = std::find_if(
      it->pages.begin(), it->pages.end(),
      [&url](const ResearchPageData& page) {
        return page.url == url;
      });
  
  if (page_it != it->pages.end()) {
    DLOG(INFO) << "Removing page from research session: " << page_it->title;
    it->pages.erase(page_it);
    
    // Update the session's last updated time
    it->last_updated = base::Time::Now();
    
    // Save the updated sessions
    SaveSessions();
  }
}

void ResearchModeController::GenerateSessionSummary(ResearchDataCallback callback) {
  // Track performance of this operation
  util::ScopedPerformanceTracker tracker("ResearchModeController_GenerateSessionSummary");
  
  if (current_session_id_.empty()) {
    DLOG(WARNING) << "No current research session";
    std::move(callback).Run(ResearchSession());
    return;
  }
  
  auto it = std::find_if(
      sessions_.begin(), sessions_.end(),
      [this](const ResearchSession& session) {
        return session.id == current_session_id_;
      });
  
  if (it == sessions_.end()) {
    DLOG(WARNING) << "Current research session not found";
    std::move(callback).Run(ResearchSession());
    return;
  }
  
  // Create a copy of the session to return
  ResearchSession session_copy = *it;
  
  // Get the service manager
  auto* service_manager = core::ServiceManager::GetInstance();
  
  // Prepare the prompt for generating a summary
  std::string prompt = "Generate a comprehensive summary of the following research materials:\n\n";
  
  for (const auto& page : session_copy.pages) {
    prompt += "Title: " + page.title + "\n";
    prompt += "URL: " + page.url + "\n";
    prompt += "Key Points:\n";
    
    for (const auto& point : page.key_points) {
      prompt += "- " + point + "\n";
    }
    
    prompt += "\n";
  }
  
  prompt += "\nProvide a well-structured summary that synthesizes the key information from all sources.";
  
  // Process the prompt with the best available adapter for text generation
  service_manager->ProcessTextWithCapabilityAsync(
      "text-generation", prompt,
      base::BindOnce([](ResearchDataCallback callback,
                      ResearchSession session_copy,
                      const adapters::ModelResponse& response) {
        if (response.success) {
          // Create a summary page
          ResearchPageData summary_page;
          summary_page.url = "summary://" + session_copy.id;
          summary_page.title = "Summary of " + session_copy.name;
          summary_page.content = response.text;
          summary_page.timestamp = base::Time::Now();
          summary_page.is_processed = true;
          
          // Add the summary page to the beginning of the session
          session_copy.pages.insert(session_copy.pages.begin(), std::move(summary_page));
        }
        
        std::move(callback).Run(session_copy);
      },
      std::move(callback), std::move(session_copy)));
}

void ResearchModeController::GenerateKeyPoints(
    const std::string& url,
    base::OnceCallback<void(const std::vector<std::string>&)> callback) {
  // Track performance of this operation
  util::ScopedPerformanceTracker tracker("ResearchModeController_GenerateKeyPoints");
  
  if (current_session_id_.empty()) {
    DLOG(WARNING) << "No current research session";
    std::move(callback).Run({});
    return;
  }
  
  auto session_it = std::find_if(
      sessions_.begin(), sessions_.end(),
      [this](const ResearchSession& session) {
        return session.id == current_session_id_;
      });
  
  if (session_it == sessions_.end()) {
    DLOG(WARNING) << "Current research session not found";
    std::move(callback).Run({});
    return;
  }
  
  // Find the page
  auto page_it = std::find_if(
      session_it->pages.begin(), session_it->pages.end(),
      [&url](const ResearchPageData& page) {
        return page.url == url;
      });
  
  if (page_it == session_it->pages.end()) {
    DLOG(WARNING) << "Page not found in research session: " << url;
    std::move(callback).Run({});
    return;
  }
  
  // Get the service manager
  auto* service_manager = core::ServiceManager::GetInstance();
  
  // Prepare the prompt for generating key points
  std::string prompt = base::StringPrintf(
      kKeyPointsPrompt, page_it->content.c_str());
  
  // Process the prompt with the best available adapter for text generation
  service_manager->ProcessTextWithCapabilityAsync(
      "text-generation", prompt,
      base::BindOnce([](ResearchModeController* controller,
                      std::string session_id,
                      std::string url,
                      base::OnceCallback<void(const std::vector<std::string>&)> callback,
                      const adapters::ModelResponse& response) {
        std::vector<std::string> key_points;
        
        if (response.success) {
          // Parse the key points from the response
          std::vector<std::string> lines = base::SplitString(
              response.text, "\n", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
          
          for (const auto& line : lines) {
            // Look for bullet points or numbered lists
            if (line.find("- ") == 0 || line.find("• ") == 0 ||
                (line.size() > 2 && isdigit(line[0]) && line[1] == '.')) {
              // Extract the content after the bullet or number
              size_t start = line.find_first_of(" ") + 1;
              if (start < line.size()) {
                key_points.push_back(line.substr(start));
              }
            } else {
              // If no bullet or number, just add the line as is
              key_points.push_back(line);
            }
          }
        }
        
        // Update the page with the key points
        auto session_it = std::find_if(
            controller->sessions_.begin(), controller->sessions_.end(),
            [&session_id](const ResearchSession& session) {
              return session.id == session_id;
            });
        
        if (session_it != controller->sessions_.end()) {
          auto page_it = std::find_if(
              session_it->pages.begin(), session_it->pages.end(),
              [&url](const ResearchPageData& page) {
                return page.url == url;
              });
          
          if (page_it != session_it->pages.end()) {
            page_it->key_points = key_points;
            page_it->is_processed = true;
            
            // Update the session's last updated time
            session_it->last_updated = base::Time::Now();
            
            // Save the updated sessions
            controller->SaveSessions();
          }
        }
        
        std::move(callback).Run(key_points);
      },
      this, current_session_id_, url, std::move(callback)));
}

void ResearchModeController::ExportSessionToDocument(
    const std::string& format,
    base::OnceCallback<void(const std::string&)> callback) {
  // Track performance of this operation
  util::ScopedPerformanceTracker tracker("ResearchModeController_ExportSessionToDocument");
  
  if (current_session_id_.empty()) {
    DLOG(WARNING) << "No current research session";
    std::move(callback).Run("");
    return;
  }
  
  auto it = std::find_if(
      sessions_.begin(), sessions_.end(),
      [this](const ResearchSession& session) {
        return session.id == current_session_id_;
      });
  
  if (it == sessions_.end()) {
    DLOG(WARNING) << "Current research session not found";
    std::move(callback).Run("");
    return;
  }
  
  // Get the service manager
  auto* service_manager = core::ServiceManager::GetInstance();
  
  // Prepare the prompt for generating a document
  std::string prompt = "Generate a " + format + " document summarizing the following research:\n\n";
  prompt += "Research Topic: " + it->topic + "\n\n";
  
  for (const auto& page : it->pages) {
    prompt += "Source: " + page.title + " (" + page.url + ")\n";
    prompt += "Key Points:\n";
    
    for (const auto& point : page.key_points) {
      prompt += "- " + point + "\n";
    }
    
    prompt += "\n";
  }
  
  prompt += "\nCreate a well-structured document that synthesizes this research into a cohesive " + 
            format + " document. Include proper formatting, headings, and citations.";
  
  // Process the prompt with the best available adapter for text generation
  service_manager->ProcessTextWithCapabilityAsync(
      "text-generation", prompt,
      base::BindOnce([](base::OnceCallback<void(const std::string&)> callback,
                      const adapters::ModelResponse& response) {
        if (response.success) {
          std::move(callback).Run(response.text);
        } else {
          std::move(callback).Run("");
        }
      },
      std::move(callback)));
}

void ResearchModeController::SearchSession(
    const std::string& query,
    base::OnceCallback<void(const std::vector<ResearchPageData>&)> callback) {
  // Track performance of this operation
  util::ScopedPerformanceTracker tracker("ResearchModeController_SearchSession");
  
  if (current_session_id_.empty()) {
    DLOG(WARNING) << "No current research session";
    std::move(callback).Run({});
    return;
  }
  
  auto it = std::find_if(
      sessions_.begin(), sessions_.end(),
      [this](const ResearchSession& session) {
        return session.id == current_session_id_;
      });
  
  if (it == sessions_.end()) {
    DLOG(WARNING) << "Current research session not found";
    std::move(callback).Run({});
    return;
  }
  
  // Convert query to lowercase for case-insensitive search
  std::string query_lower = base::ToLowerASCII(query);
  
  // Search for pages that match the query
  std::vector<ResearchPageData> matching_pages;
  
  for (const auto& page : it->pages) {
    // Check if the query appears in the title, content, or key points
    std::string title_lower = base::ToLowerASCII(page.title);
    std::string content_lower = base::ToLowerASCII(page.content);
    
    if (title_lower.find(query_lower) != std::string::npos ||
        content_lower.find(query_lower) != std::string::npos) {
      matching_pages.push_back(page);
      continue;
    }
    
    // Check key points
    for (const auto& point : page.key_points) {
      std::string point_lower = base::ToLowerASCII(point);
      if (point_lower.find(query_lower) != std::string::npos) {
        matching_pages.push_back(page);
        break;
      }
    }
  }
  
  std::move(callback).Run(matching_pages);
}

void ResearchModeController::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInPrimaryMainFrame() ||
      !navigation_handle->HasCommitted() ||
      navigation_handle->IsSameDocument()) {
    return;
  }
  
  // Check if research mode is enabled and auto-add pages is enabled
  if (IsResearchModeEnabled() && base::GetFieldTrialParamByFeatureAsBool(
          kAsolResearchMode, "auto_add_pages", false)) {
    // Add the page to the current research session
    std::string url = navigation_handle->GetURL().spec();
    std::string title = base::UTF16ToUTF8(web_contents()->GetTitle());
    
    ProcessPage(url, title, base::BindOnce(
        &ResearchModeController::HandleExtractedContext,
        weak_ptr_factory_.GetWeakPtr(),
        url, title));
  }
}

void ResearchModeController::WebContentsDestroyed() {
  // Save research sessions before web contents is destroyed
  SaveSessions();
}

void ResearchModeController::TitleWasSet(content::NavigationEntry* entry) {
  if (!entry || !IsResearchModeEnabled() || !base::GetFieldTrialParamByFeatureAsBool(
          kAsolResearchMode, "auto_add_pages", false)) {
    return;
  }
  
  // Update the title of the current page in the research session
  std::string url = entry->GetURL().spec();
  std::string title = base::UTF16ToUTF8(entry->GetTitle());
  
  if (current_session_id_.empty()) {
    return;
  }
  
  auto session_it = std::find_if(
      sessions_.begin(), sessions_.end(),
      [this](const ResearchSession& session) {
        return session.id == current_session_id_;
      });
  
  if (session_it == sessions_.end()) {
    return;
  }
  
  // Find the page
  auto page_it = std::find_if(
      session_it->pages.begin(), session_it->pages.end(),
      [&url](const ResearchPageData& page) {
        return page.url == url;
      });
  
  if (page_it != session_it->pages.end()) {
    // Update the title
    page_it->title = title;
    
    // Update the session's last updated time
    session_it->last_updated = base::Time::Now();
    
    // Save the updated sessions
    SaveSessions();
  }
}

void ResearchModeController::ProcessPage(
    const std::string& url, 
    const std::string& title,
    PageContextExtractor::ContextCallback callback) {
  if (!context_extractor_) {
    std::move(callback).Run("");
    return;
  }
  
  // Extract the full page content
  context_extractor_->ExtractFullPageContent(std::move(callback));
}

void ResearchModeController::HandleExtractedContext(
    const std::string& url,
    const std::string& title,
    const std::string& context) {
  // Add the page to the current research session
  AddPageToSession(url, title, context);
}

std::string ResearchModeController::GenerateSessionId() const {
  return base::GenerateGUID();
}

void ResearchModeController::SaveSessions() {
  if (!web_contents()) {
    return;
  }
  
  content::BrowserContext* browser_context = web_contents()->GetBrowserContext();
  if (!browser_context) {
    return;
  }
  
  base::FilePath path = GetResearchSessionsFilePath(browser_context);
  if (path.empty()) {
    DLOG(ERROR) << "Failed to get research sessions file path";
    return;
  }
  
  // Convert sessions to JSON
  base::Value::List sessions_list;
  for (const auto& session : sessions_) {
    sessions_list.Append(ResearchSessionToValue(session));
  }
  
  base::Value::Dict root;
  root.Set("sessions", std::move(sessions_list));
  root.Set("current_session_id", current_session_id_);
  
  std::string json_string;
  if (!base::JSONWriter::WriteWithOptions(
          root, base::JSONWriter::OPTIONS_PRETTY_PRINT, &json_string)) {
    DLOG(ERROR) << "Failed to serialize research sessions to JSON";
    return;
  }
  
  // Write to file
  if (base::WriteFile(path, json_string.data(), json_string.size()) == -1) {
    DLOG(ERROR) << "Failed to write research sessions to file: " << path.value();
  }
}

void ResearchModeController::LoadSessions() {
  if (!web_contents()) {
    return;
  }
  
  content::BrowserContext* browser_context = web_contents()->GetBrowserContext();
  if (!browser_context) {
    return;
  }
  
  base::FilePath path = GetResearchSessionsFilePath(browser_context);
  if (path.empty()) {
    DLOG(ERROR) << "Failed to get research sessions file path";
    return;
  }
  
  // Check if the file exists
  if (!base::PathExists(path)) {
    DLOG(INFO) << "Research sessions file does not exist: " << path.value();
    return;
  }
  
  // Read the file
  std::string json_string;
  if (!base::ReadFileToString(path, &json_string)) {
    DLOG(ERROR) << "Failed to read research sessions from file: " << path.value();
    return;
  }
  
  // Parse the JSON
  absl::optional<base::Value> value = base::JSONReader::Read(json_string);
  if (!value || !value->is_dict()) {
    DLOG(ERROR) << "Failed to parse research sessions JSON";
    return;
  }
  
  const base::Value::Dict& root = value->GetDict();
  
  // Get the current session ID
  current_session_id_ = root.FindString("current_session_id").value_or("");
  
  // Get the sessions
  const base::Value::List* sessions_list = root.FindList("sessions");
  if (!sessions_list) {
    DLOG(ERROR) << "No sessions found in research sessions JSON";
    return;
  }
  
  sessions_.clear();
  for (const auto& session_value : *sessions_list) {
    if (!session_value.is_dict()) continue;
    
    sessions_.push_back(ValueToResearchSession(session_value.GetDict()));
  }
  
  DLOG(INFO) << "Loaded " << sessions_.size() << " research sessions";
}

}  // namespace browser
}  // namespace asol