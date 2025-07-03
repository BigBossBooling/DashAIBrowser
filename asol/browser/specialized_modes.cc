// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "asol/browser/specialized_modes.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "asol/browser/browser_features.h"
#include "asol/browser/page_context_extractor.h"
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

// JavaScript to detect programming language on the page
const char kDetectProgrammingLanguageScript[] = R"(
  (function() {
    // Check for common indicators of programming languages
    const codeElements = document.querySelectorAll('pre, code');
    let languageHints = [];
    
    // Check code elements for language hints
    for (const element of codeElements) {
      const classes = element.className.split(' ');
      for (const cls of classes) {
        if (cls.startsWith('language-') || cls.startsWith('lang-')) {
          languageHints.push(cls.split('-')[1]);
        }
      }
    }
    
    // Check for common language keywords
    const pageText = document.body.innerText.toLowerCase();
    const languagePatterns = {
      'javascript': /function\s+\w+\s*\(|const\s+\w+\s*=|let\s+\w+\s*=|var\s+\w+\s*=|=>\s*{/g,
      'python': /def\s+\w+\s*\(|import\s+\w+|from\s+\w+\s+import/g,
      'java': /public\s+class|private\s+\w+\(|protected\s+\w+\(|@Override/g,
      'c++': /std::|#include\s*<\w+>|template\s*<|namespace\s+\w+/g,
      'c#': /using\s+System;|public\s+class|namespace\s+\w+|async\s+Task/g,
      'ruby': /def\s+\w+\s*\(|require\s+[\'\"]|module\s+\w+|class\s+\w+\s*</g,
      'php': /<\?php|\$\w+\s*=|function\s+\w+\s*\(|namespace\s+\w+/g,
      'go': /func\s+\w+\s*\(|package\s+\w+|import\s+\(|type\s+\w+\s+struct/g,
      'rust': /fn\s+\w+\s*\(|let\s+mut|impl\s+\w+|use\s+\w+::/g,
      'typescript': /interface\s+\w+|type\s+\w+\s*=|class\s+\w+\s*implements/g,
      'swift': /func\s+\w+\s*\(|var\s+\w+\s*:|let\s+\w+\s*:|class\s+\w+\s*:/g,
      'kotlin': /fun\s+\w+\s*\(|val\s+\w+\s*:|var\s+\w+\s*:|class\s+\w+\s*\(/g,
      'html': /<html|<head|<body|<div|<span|<p>/g,
      'css': /\.[\w-]+\s*{|#[\w-]+\s*{|@media|@keyframes/g,
      'sql': /SELECT\s+\w+\s+FROM|INSERT\s+INTO|UPDATE\s+\w+\s+SET|CREATE\s+TABLE/gi
    };
    
    let languageCounts = {};
    for (const [language, pattern] of Object.entries(languagePatterns)) {
      const matches = pageText.match(pattern);
      if (matches) {
        languageCounts[language] = matches.length;
      }
    }
    
    // Determine the most likely language
    let detectedLanguage = '';
    let maxCount = 0;
    
    // First check explicit hints
    if (languageHints.length > 0) {
      // Count occurrences of each hint
      let hintCounts = {};
      for (const hint of languageHints) {
        hintCounts[hint] = (hintCounts[hint] || 0) + 1;
      }
      
      // Find the most common hint
      for (const [lang, count] of Object.entries(hintCounts)) {
        if (count > maxCount) {
          maxCount = count;
          detectedLanguage = lang;
        }
      }
    }
    
    // If no hints found, use pattern matching
    if (!detectedLanguage) {
      for (const [language, count] of Object.entries(languageCounts)) {
        if (count > maxCount) {
          maxCount = count;
          detectedLanguage = language;
        }
      }
    }
    
    return detectedLanguage;
  })();
)";

// JavaScript to extract code snippets from the page
const char kExtractCodeSnippetsScript[] = R"(
  (function() {
    const codeElements = document.querySelectorAll('pre, code');
    let snippets = [];
    
    for (const element of codeElements) {
      // Get the code content
      const code = element.textContent.trim();
      if (!code || code.length < 10) continue; // Skip very short snippets
      
      // Try to determine the language
      let language = 'unknown';
      const classes = element.className.split(' ');
      for (const cls of classes) {
        if (cls.startsWith('language-') || cls.startsWith('lang-')) {
          language = cls.split('-')[1];
          break;
        }
      }
      
      // Get surrounding context for description
      let description = '';
      let prevElement = element.previousElementSibling;
      if (prevElement && (prevElement.tagName === 'H1' || 
                          prevElement.tagName === 'H2' || 
                          prevElement.tagName === 'H3' || 
                          prevElement.tagName === 'H4' || 
                          prevElement.tagName === 'H5' || 
                          prevElement.tagName === 'H6' || 
                          prevElement.tagName === 'P')) {
        description = prevElement.textContent.trim();
      }
      
      snippets.push({
        language: language,
        code: code,
        description: description
      });
    }
    
    return JSON.stringify(snippets);
  })();
)";

// JavaScript to detect game information on the page
const char kDetectGameScript[] = R"(
  (function() {
    // Common game genres
    const genres = [
      'Action', 'Adventure', 'RPG', 'Strategy', 'Simulation', 'Sports', 
      'Racing', 'Puzzle', 'FPS', 'MMORPG', 'MOBA', 'Battle Royale', 
      'Platformer', 'Survival', 'Horror', 'Stealth', 'Fighting', 
      'Rhythm', 'Sandbox', 'Open World'
    ];
    
    // Common gaming platforms
    const platforms = [
      'PC', 'PlayStation', 'PS5', 'PS4', 'PS3', 'Xbox', 'Xbox Series X', 
      'Xbox One', 'Nintendo Switch', 'Nintendo', 'iOS', 'Android', 
      'Steam', 'Epic Games', 'GOG', 'Stadia', 'GeForce Now'
    ];
    
    // Function to extract text from meta tags
    function getMetaContent(name) {
      const meta = document.querySelector(`meta[name="${name}"], meta[property="${name}"]`);
      return meta ? meta.getAttribute('content') : '';
    }
    
    // Try to get the game title
    let title = document.title;
    
    // Remove common suffixes from title
    title = title.replace(/\s*-\s*(Official Site|Game|Review|Walkthrough|Guide|Tips|Cheats|Wiki).*$/i, '');
    
    // Check for game genre
    let genre = '';
    const pageText = document.body.innerText;
    
    for (const g of genres) {
      const regex = new RegExp(`\\b${g}\\b`, 'i');
      if (regex.test(pageText)) {
        genre = g;
        break;
      }
    }
    
    // Check for platform
    let platform = '';
    for (const p of platforms) {
      const regex = new RegExp(`\\b${p}\\b`, 'i');
      if (regex.test(pageText)) {
        platform = p;
        break;
      }
    }
    
    // Look for tips sections
    let tips = '';
    const tipElements = document.querySelectorAll('h1, h2, h3, h4');
    for (const element of tipElements) {
      if (/tips|tricks|hints|guide/i.test(element.textContent)) {
        let nextElement = element.nextElementSibling;
        while (nextElement && !['H1', 'H2', 'H3', 'H4'].includes(nextElement.tagName)) {
          tips += nextElement.textContent.trim() + ' ';
          nextElement = nextElement.nextElementSibling;
        }
        break;
      }
    }
    
    // Look for strategy sections
    let strategies = '';
    const strategyElements = document.querySelectorAll('h1, h2, h3, h4');
    for (const element of strategyElements) {
      if (/strategy|strategies|walkthrough|how to/i.test(element.textContent)) {
        let nextElement = element.nextElementSibling;
        while (nextElement && !['H1', 'H2', 'H3', 'H4'].includes(nextElement.tagName)) {
          strategies += nextElement.textContent.trim() + ' ';
          nextElement = nextElement.nextElementSibling;
        }
        break;
      }
    }
    
    return JSON.stringify({
      title: title,
      genre: genre,
      platform: platform,
      tips: tips.substring(0, 1000),
      strategies: strategies.substring(0, 1000)
    });
  })();
)";

// Convert a CodeSnippet to a base::Value
base::Value::Dict CodeSnippetToValue(const CodeSnippet& snippet) {
  base::Value::Dict dict;
  dict.Set("language", snippet.language);
  dict.Set("code", snippet.code);
  dict.Set("description", snippet.description);
  dict.Set("source_url", snippet.source_url);
  return dict;
}

// Convert a base::Value to a CodeSnippet
CodeSnippet ValueToCodeSnippet(const base::Value::Dict& value) {
  CodeSnippet snippet;
  snippet.language = value.FindString("language").value_or("");
  snippet.code = value.FindString("code").value_or("");
  snippet.description = value.FindString("description").value_or("");
  snippet.source_url = value.FindString("source_url").value_or("");
  return snippet;
}

// Convert a WorkDocument to a base::Value
base::Value::Dict WorkDocumentToValue(const WorkDocument& document) {
  base::Value::Dict dict;
  dict.Set("title", document.title);
  dict.Set("content", document.content);
  dict.Set("format", document.format);
  dict.Set("url", document.url);
  dict.Set("is_draft", document.is_draft);
  return dict;
}

// Convert a base::Value to a WorkDocument
WorkDocument ValueToWorkDocument(const base::Value::Dict& value) {
  WorkDocument document;
  document.title = value.FindString("title").value_or("");
  document.content = value.FindString("content").value_or("");
  document.format = value.FindString("format").value_or("");
  document.url = value.FindString("url").value_or("");
  document.is_draft = value.FindBool("is_draft").value_or(true);
  return document;
}

// Convert a GameInfo to a base::Value
base::Value::Dict GameInfoToValue(const GameInfo& game_info) {
  base::Value::Dict dict;
  dict.Set("title", game_info.title);
  dict.Set("genre", game_info.genre);
  dict.Set("platform", game_info.platform);
  dict.Set("tips", game_info.tips);
  dict.Set("strategies", game_info.strategies);
  dict.Set("url", game_info.url);
  return dict;
}

// Convert a base::Value to a GameInfo
GameInfo ValueToGameInfo(const base::Value::Dict& value) {
  GameInfo game_info;
  game_info.title = value.FindString("title").value_or("");
  game_info.genre = value.FindString("genre").value_or("");
  game_info.platform = value.FindString("platform").value_or("");
  game_info.tips = value.FindString("tips").value_or("");
  game_info.strategies = value.FindString("strategies").value_or("");
  game_info.url = value.FindString("url").value_or("");
  return game_info;
}

// Get the file path for storing specialized mode data
base::FilePath GetSpecializedModeDataFilePath(content::BrowserContext* context) {
  base::FilePath path;
  if (!base::PathService::Get(base::DIR_USER_DATA, &path)) {
    return base::FilePath();
  }
  
  path = path.AppendASCII("asol_specialized_modes.json");
  return path;
}

// Prompt for generating code documentation
const char kCodeDocumentationPrompt[] = 
    "Generate comprehensive documentation for the following %s code:\n\n"
    "```%s\n%s\n```\n\n"
    "Include function descriptions, parameter explanations, return values, "
    "and any important notes about usage or edge cases.";

// Prompt for explaining code
const char kExplainCodePrompt[] = 
    "Explain the following %s code in detail:\n\n"
    "```%s\n%s\n```\n\n"
    "Break down how it works, what each part does, and the overall purpose.";

// Prompt for optimizing code
const char kOptimizeCodePrompt[] = 
    "Optimize the following %s code for better performance and readability:\n\n"
    "```%s\n%s\n```\n\n"
    "Provide the optimized code and explain the improvements made.";

// Prompt for debugging code
const char kDebugCodePrompt[] = 
    "Debug the following %s code that's producing this error: %s\n\n"
    "```%s\n%s\n```\n\n"
    "Identify the issue, explain the cause, and provide a fixed version of the code.";

// Prompt for generating document content
const char kGenerateDocumentPrompt[] = 
    "Generate a %s document with the title \"%s\" based on the following prompt:\n\n%s";

// Prompt for summarizing a document
const char kSummarizeDocumentPrompt[] = 
    "Summarize the following document content into a concise overview:\n\n%s";

// Prompt for formatting a document
const char kFormatDocumentPrompt[] = 
    "Format the following content as a %s document:\n\n%s";

// Prompt for extracting action items
const char kExtractActionItemsPrompt[] = 
    "Extract all action items, tasks, and to-dos from the following document:\n\n%s\n\n"
    "Present each action item as a separate bullet point.";

// Prompt for getting game tips
const char kGameTipsPrompt[] = 
    "Provide useful tips and tricks for playing %s. Include beginner advice, "
    "controls, and helpful strategies.";

// Prompt for getting game strategies
const char kGameStrategiesPrompt[] = 
    "Provide advanced strategies for %s. Include tactics, optimal builds or loadouts, "
    "and approaches for different play styles.";

// Prompt for generating a game walkthrough
const char kGameWalkthroughPrompt[] = 
    "Create a detailed walkthrough for the %s level/area in %s. Include step-by-step "
    "instructions, locations of important items, and strategies for overcoming challenges.";

// Prompt for optimizing game settings
const char kOptimizeGameSettingsPrompt[] = 
    "Recommend optimal settings for playing %s on %s hardware. Include graphics settings, "
    "control configurations, and any other relevant optimizations for the best experience.";

}  // namespace

// static
WEB_CONTENTS_USER_DATA_KEY_IMPL(SpecializedModesController);

SpecializedModesController::SpecializedModesController(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<SpecializedModesController>(*web_contents) {
  DLOG(INFO) << "SpecializedModesController created for WebContents: " << web_contents;
  
  // Load saved data
  LoadData();
}

SpecializedModesController::~SpecializedModesController() {
  DLOG(INFO) << "SpecializedModesController destroyed";
  
  // Save data before destruction
  SaveData();
}

void SpecializedModesController::SetMode(SpecializedMode mode) {
  // Check if the requested mode is enabled via feature flags
  bool mode_enabled = false;
  
  switch (mode) {
    case SpecializedMode::kDeveloper:
      mode_enabled = base::FeatureList::IsEnabled(kAsolDeveloperMode);
      break;
    case SpecializedMode::kWork:
      mode_enabled = base::FeatureList::IsEnabled(kAsolWorkMode);
      break;
    case SpecializedMode::kGaming:
      mode_enabled = base::FeatureList::IsEnabled(kAsolGamingMode);
      break;
    case SpecializedMode::kNone:
      mode_enabled = true;
      break;
  }
  
  if (!mode_enabled) {
    DLOG(WARNING) << "Attempted to set disabled mode: " << static_cast<int>(mode);
    return;
  }
  
  current_mode_ = mode;
  DLOG(INFO) << "Specialized mode set to: " << GetModeString();
  
  // If we're on a page, perform mode-specific actions
  if (web_contents() && web_contents()->GetLastCommittedURL().is_valid()) {
    switch (current_mode_) {
      case SpecializedMode::kDeveloper:
        if (base::GetFieldTrialParamByFeatureAsBool(
                kAsolDeveloperMode, "auto_detect_code", true)) {
          ExtractCodeSnippets(base::DoNothing());
        }
        break;
      case SpecializedMode::kGaming:
        if (base::GetFieldTrialParamByFeatureAsBool(
                kAsolGamingMode, "auto_detect_games", true)) {
          DetectGame(base::DoNothing());
        }
        break;
      default:
        break;
    }
  }
}

SpecializedMode SpecializedModesController::GetMode() const {
  return current_mode_;
}

std::string SpecializedModesController::GetModeString() const {
  switch (current_mode_) {
    case SpecializedMode::kDeveloper:
      return "Developer";
    case SpecializedMode::kWork:
      return "Work";
    case SpecializedMode::kGaming:
      return "Gaming";
    case SpecializedMode::kNone:
    default:
      return "None";
  }
}

//
// Developer Mode Features
//

void SpecializedModesController::DetectProgrammingLanguage(
    base::OnceCallback<void(const std::string&)> callback) {
  // Track performance of this operation
  util::ScopedPerformanceTracker tracker("SpecializedModesController_DetectProgrammingLanguage");
  
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
      base::UTF8ToUTF16(kDetectProgrammingLanguageScript),
      base::BindOnce([](base::OnceCallback<void(const std::string&)> callback,
                       base::Value result) {
        std::string language;
        if (result.is_string()) {
          language = result.GetString();
        }
        std::move(callback).Run(language);
      },
      std::move(callback)));
}

void SpecializedModesController::ExtractCodeSnippets(CodeSnippetsCallback callback) {
  // Track performance of this operation
  util::ScopedPerformanceTracker tracker("SpecializedModesController_ExtractCodeSnippets");
  
  if (!web_contents()) {
    std::move(callback).Run({});
    return;
  }
  
  content::RenderFrameHost* main_frame = web_contents()->GetPrimaryMainFrame();
  if (!main_frame) {
    std::move(callback).Run({});
    return;
  }
  
  std::string url = web_contents()->GetLastCommittedURL().spec();
  
  main_frame->ExecuteJavaScriptForTests(
      base::UTF8ToUTF16(kExtractCodeSnippetsScript),
      base::BindOnce([](SpecializedModesController* controller,
                       std::string url,
                       CodeSnippetsCallback callback,
                       base::Value result) {
        std::vector<CodeSnippet> snippets;
        
        if (result.is_string()) {
          std::string json_str = result.GetString();
          absl::optional<base::Value> parsed = base::JSONReader::Read(json_str);
          
          if (parsed && parsed->is_list()) {
            for (const auto& item : parsed->GetList()) {
              if (!item.is_dict()) continue;
              
              const auto& dict = item.GetDict();
              CodeSnippet snippet;
              snippet.language = dict.FindString("language").value_or("unknown");
              snippet.code = dict.FindString("code").value_or("");
              snippet.description = dict.FindString("description").value_or("");
              snippet.source_url = url;
              
              if (!snippet.code.empty()) {
                snippets.push_back(snippet);
                
                // Auto-save if in developer mode
                if (controller->GetMode() == SpecializedMode::kDeveloper) {
                  controller->SaveCodeSnippet(snippet);
                }
              }
            }
          }
        }
        
        std::move(callback).Run(snippets);
      },
      this, url, std::move(callback)));
}

void SpecializedModesController::SaveCodeSnippet(const CodeSnippet& snippet) {
  // Check if we already have this snippet
  auto it = std::find_if(
      code_snippets_.begin(), code_snippets_.end(),
      [&snippet](const CodeSnippet& existing) {
        return existing.code == snippet.code && 
               existing.source_url == snippet.source_url;
      });
  
  if (it != code_snippets_.end()) {
    // Update existing snippet
    it->description = snippet.description;
    it->language = snippet.language;
    DLOG(INFO) << "Updated existing code snippet from: " << snippet.source_url;
  } else {
    // Check if we've reached the maximum number of snippets
    int max_snippets = base::GetFieldTrialParamByFeatureAsInt(
        kAsolDeveloperMode, "max_code_snippets", 100);
    
    if (static_cast<int>(code_snippets_.size()) >= max_snippets) {
      // Remove the first snippet (oldest)
      code_snippets_.erase(code_snippets_.begin());
    }
    
    // Add new snippet
    code_snippets_.push_back(snippet);
    DLOG(INFO) << "Saved new code snippet from: " << snippet.source_url;
  }
  
  // Save to disk
  SaveData();
}

void SpecializedModesController::GetSavedCodeSnippets(CodeSnippetsCallback callback) {
  std::move(callback).Run(code_snippets_);
}

void SpecializedModesController::GenerateCodeDocumentation(
    const std::string& code,
    base::OnceCallback<void(const std::string&)> callback) {
  // Track performance of this operation
  util::ScopedPerformanceTracker tracker("SpecializedModesController_GenerateCodeDocumentation");
  
  // Detect the language if not specified
  DetectProgrammingLanguage(base::BindOnce(
      [](SpecializedModesController* controller,
         std::string code,
         base::OnceCallback<void(const std::string&)> callback,
         const std::string& language) {
        std::string lang = language.empty() ? "unknown" : language;
        
        // Prepare the prompt
        std::string prompt = base::StringPrintf(
            kCodeDocumentationPrompt, lang.c_str(), lang.c_str(), code.c_str());
        
        // Get the service manager
        auto* service_manager = core::ServiceManager::GetInstance();
        
        // Process the prompt with the best available adapter for text generation
        service_manager->ProcessTextWithCapabilityAsync(
            "text-generation", prompt,
            base::BindOnce([](base::OnceCallback<void(const std::string&)> callback,
                            const adapters::ModelResponse& response) {
              if (response.success) {
                std::move(callback).Run(response.text);
              } else {
                std::move(callback).Run("Failed to generate documentation: " + 
                                       response.error_message);
              }
            },
            std::move(callback)));
      },
      this, code, std::move(callback)));
}

void SpecializedModesController::ExplainCode(
    const std::string& code,
    base::OnceCallback<void(const std::string&)> callback) {
  // Track performance of this operation
  util::ScopedPerformanceTracker tracker("SpecializedModesController_ExplainCode");
  
  // Detect the language if not specified
  DetectProgrammingLanguage(base::BindOnce(
      [](std::string code,
         base::OnceCallback<void(const std::string&)> callback,
         const std::string& language) {
        std::string lang = language.empty() ? "unknown" : language;
        
        // Prepare the prompt
        std::string prompt = base::StringPrintf(
            kExplainCodePrompt, lang.c_str(), lang.c_str(), code.c_str());
        
        // Get the service manager
        auto* service_manager = core::ServiceManager::GetInstance();
        
        // Process the prompt with the best available adapter for text generation
        service_manager->ProcessTextWithCapabilityAsync(
            "text-generation", prompt,
            base::BindOnce([](base::OnceCallback<void(const std::string&)> callback,
                            const adapters::ModelResponse& response) {
              if (response.success) {
                std::move(callback).Run(response.text);
              } else {
                std::move(callback).Run("Failed to explain code: " + 
                                       response.error_message);
              }
            },
            std::move(callback)));
      },
      code, std::move(callback)));
}

void SpecializedModesController::OptimizeCode(
    const std::string& code,
    base::OnceCallback<void(const std::string&)> callback) {
  // Track performance of this operation
  util::ScopedPerformanceTracker tracker("SpecializedModesController_OptimizeCode");
  
  // Detect the language if not specified
  DetectProgrammingLanguage(base::BindOnce(
      [](std::string code,
         base::OnceCallback<void(const std::string&)> callback,
         const std::string& language) {
        std::string lang = language.empty() ? "unknown" : language;
        
        // Prepare the prompt
        std::string prompt = base::StringPrintf(
            kOptimizeCodePrompt, lang.c_str(), lang.c_str(), code.c_str());
        
        // Get the service manager
        auto* service_manager = core::ServiceManager::GetInstance();
        
        // Process the prompt with the best available adapter for text generation
        service_manager->ProcessTextWithCapabilityAsync(
            "text-generation", prompt,
            base::BindOnce([](base::OnceCallback<void(const std::string&)> callback,
                            const adapters::ModelResponse& response) {
              if (response.success) {
                std::move(callback).Run(response.text);
              } else {
                std::move(callback).Run("Failed to optimize code: " + 
                                       response.error_message);
              }
            },
            std::move(callback)));
      },
      code, std::move(callback)));
}

void SpecializedModesController::DebugCode(
    const std::string& code,
    const std::string& error_message,
    base::OnceCallback<void(const std::string&)> callback) {
  // Track performance of this operation
  util::ScopedPerformanceTracker tracker("SpecializedModesController_DebugCode");
  
  // Detect the language if not specified
  DetectProgrammingLanguage(base::BindOnce(
      [](std::string code,
         std::string error_message,
         base::OnceCallback<void(const std::string&)> callback,
         const std::string& language) {
        std::string lang = language.empty() ? "unknown" : language;
        
        // Prepare the prompt
        std::string prompt = base::StringPrintf(
            kDebugCodePrompt, lang.c_str(), error_message.c_str(), 
            lang.c_str(), code.c_str());
        
        // Get the service manager
        auto* service_manager = core::ServiceManager::GetInstance();
        
        // Process the prompt with the best available adapter for text generation
        service_manager->ProcessTextWithCapabilityAsync(
            "text-generation", prompt,
            base::BindOnce([](base::OnceCallback<void(const std::string&)> callback,
                            const adapters::ModelResponse& response) {
              if (response.success) {
                std::move(callback).Run(response.text);
              } else {
                std::move(callback).Run("Failed to debug code: " + 
                                       response.error_message);
              }
            },
            std::move(callback)));
      },
      code, error_message, std::move(callback)));
}

//
// Work Mode Features
//

void SpecializedModesController::CreateDocument(
    const std::string& title, const std::string& format) {
  // Check if a document with this title already exists
  auto it = std::find_if(
      documents_.begin(), documents_.end(),
      [&title](const WorkDocument& doc) {
        return doc.title == title;
      });
  
  if (it != documents_.end()) {
    DLOG(INFO) << "Document with title already exists: " << title;
    return;
  }
  
  // Check if we've reached the maximum number of documents
  int max_documents = base::GetFieldTrialParamByFeatureAsInt(
      kAsolWorkMode, "max_documents", 50);
  
  if (static_cast<int>(documents_.size()) >= max_documents) {
    // Remove the first document (oldest)
    documents_.erase(documents_.begin());
  }
  
  // Create a new document
  WorkDocument document;
  document.title = title;
  document.format = format;
  document.is_draft = true;
  
  if (web_contents()) {
    document.url = web_contents()->GetLastCommittedURL().spec();
  }
  
  documents_.push_back(document);
  DLOG(INFO) << "Created new document: " << title;
  
  // Save to disk
  SaveData();
}

void SpecializedModesController::GetAllDocuments(WorkDocumentsCallback callback) {
  std::move(callback).Run(documents_);
}

void SpecializedModesController::UpdateDocument(
    const std::string& title, const std::string& content) {
  auto it = std::find_if(
      documents_.begin(), documents_.end(),
      [&title](const WorkDocument& doc) {
        return doc.title == title;
      });
  
  if (it != documents_.end()) {
    it->content = content;
    DLOG(INFO) << "Updated document: " << title;
    
    // Save to disk
    SaveData();
  } else {
    DLOG(WARNING) << "Document not found: " << title;
  }
}

void SpecializedModesController::GenerateDocumentContent(
    const std::string& title,
    const std::string& prompt,
    base::OnceCallback<void(const std::string&)> callback) {
  // Track performance of this operation
  util::ScopedPerformanceTracker tracker("SpecializedModesController_GenerateDocumentContent");
  
  // Find the document format
  std::string format = "text";
  auto it = std::find_if(
      documents_.begin(), documents_.end(),
      [&title](const WorkDocument& doc) {
        return doc.title == title;
      });
  
  if (it != documents_.end()) {
    format = it->format;
  }
  
  // Prepare the prompt
  std::string full_prompt = base::StringPrintf(
      kGenerateDocumentPrompt, format.c_str(), title.c_str(), prompt.c_str());
  
  // Get the service manager
  auto* service_manager = core::ServiceManager::GetInstance();
  
  // Process the prompt with the best available adapter for text generation
  service_manager->ProcessTextWithCapabilityAsync(
      "text-generation", full_prompt,
      base::BindOnce([](SpecializedModesController* controller,
                      std::string title,
                      base::OnceCallback<void(const std::string&)> callback,
                      const adapters::ModelResponse& response) {
        if (response.success) {
          // Update the document with the generated content
          controller->UpdateDocument(title, response.text);
          std::move(callback).Run(response.text);
        } else {
          std::move(callback).Run("Failed to generate document content: " + 
                                 response.error_message);
        }
      },
      this, title, std::move(callback)));
}

void SpecializedModesController::SummarizeDocument(
    const std::string& content,
    base::OnceCallback<void(const std::string&)> callback) {
  // Track performance of this operation
  util::ScopedPerformanceTracker tracker("SpecializedModesController_SummarizeDocument");
  
  // Prepare the prompt
  std::string prompt = base::StringPrintf(kSummarizeDocumentPrompt, content.c_str());
  
  // Get the service manager
  auto* service_manager = core::ServiceManager::GetInstance();
  
  // Process the prompt with the best available adapter for text generation
  service_manager->ProcessTextWithCapabilityAsync(
      "text-generation", prompt,
      base::BindOnce([](base::OnceCallback<void(const std::string&)> callback,
                      const adapters::ModelResponse& response) {
        if (response.success) {
          std::move(callback).Run(response.text);
        } else {
          std::move(callback).Run("Failed to summarize document: " + 
                                 response.error_message);
        }
      },
      std::move(callback)));
}

void SpecializedModesController::FormatDocument(
    const std::string& content,
    const std::string& format,
    base::OnceCallback<void(const std::string&)> callback) {
  // Track performance of this operation
  util::ScopedPerformanceTracker tracker("SpecializedModesController_FormatDocument");
  
  // Prepare the prompt
  std::string prompt = base::StringPrintf(kFormatDocumentPrompt, format.c_str(), content.c_str());
  
  // Get the service manager
  auto* service_manager = core::ServiceManager::GetInstance();
  
  // Process the prompt with the best available adapter for text generation
  service_manager->ProcessTextWithCapabilityAsync(
      "text-generation", prompt,
      base::BindOnce([](base::OnceCallback<void(const std::string&)> callback,
                      const adapters::ModelResponse& response) {
        if (response.success) {
          std::move(callback).Run(response.text);
        } else {
          std::move(callback).Run("Failed to format document: " + 
                                 response.error_message);
        }
      },
      std::move(callback)));
}

void SpecializedModesController::ExtractActionItems(
    const std::string& content,
    base::OnceCallback<void(const std::vector<std::string>&)> callback) {
  // Track performance of this operation
  util::ScopedPerformanceTracker tracker("SpecializedModesController_ExtractActionItems");
  
  // Prepare the prompt
  std::string prompt = base::StringPrintf(kExtractActionItemsPrompt, content.c_str());
  
  // Get the service manager
  auto* service_manager = core::ServiceManager::GetInstance();
  
  // Process the prompt with the best available adapter for text generation
  service_manager->ProcessTextWithCapabilityAsync(
      "text-generation", prompt,
      base::BindOnce([](base::OnceCallback<void(const std::vector<std::string>&)> callback,
                      const adapters::ModelResponse& response) {
        std::vector<std::string> action_items;
        
        if (response.success) {
          // Parse the action items from the response
          std::vector<std::string> lines = base::SplitString(
              response.text, "\n", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
          
          for (const auto& line : lines) {
            // Look for bullet points or numbered lists
            if (line.find("- ") == 0 || line.find("• ") == 0 ||
                (line.size() > 2 && isdigit(line[0]) && line[1] == '.')) {
              // Extract the content after the bullet or number
              size_t start = line.find_first_of(" ") + 1;
              if (start < line.size()) {
                action_items.push_back(line.substr(start));
              }
            } else {
              // If no bullet or number, just add the line as is
              action_items.push_back(line);
            }
          }
        }
        
        std::move(callback).Run(action_items);
      },
      std::move(callback)));
}

//
// Gaming Mode Features
//

void SpecializedModesController::DetectGame(
    base::OnceCallback<void(const GameInfo&)> callback) {
  // Track performance of this operation
  util::ScopedPerformanceTracker tracker("SpecializedModesController_DetectGame");
  
  if (!web_contents()) {
    std::move(callback).Run(GameInfo());
    return;
  }
  
  content::RenderFrameHost* main_frame = web_contents()->GetPrimaryMainFrame();
  if (!main_frame) {
    std::move(callback).Run(GameInfo());
    return;
  }
  
  std::string url = web_contents()->GetLastCommittedURL().spec();
  
  main_frame->ExecuteJavaScriptForTests(
      base::UTF8ToUTF16(kDetectGameScript),
      base::BindOnce([](SpecializedModesController* controller,
                       std::string url,
                       base::OnceCallback<void(const GameInfo&)> callback,
                       base::Value result) {
        GameInfo game_info;
        
        if (result.is_string()) {
          std::string json_str = result.GetString();
          absl::optional<base::Value> parsed = base::JSONReader::Read(json_str);
          
          if (parsed && parsed->is_dict()) {
            const auto& dict = parsed->GetDict();
            game_info.title = dict.FindString("title").value_or("");
            game_info.genre = dict.FindString("genre").value_or("");
            game_info.platform = dict.FindString("platform").value_or("");
            game_info.tips = dict.FindString("tips").value_or("");
            game_info.strategies = dict.FindString("strategies").value_or("");
            game_info.url = url;
            
            // Auto-save if in gaming mode and we have a title
            if (controller->GetMode() == SpecializedMode::kGaming && 
                !game_info.title.empty()) {
              controller->SaveGameInfo(game_info);
            }
          }
        }
        
        std::move(callback).Run(game_info);
      },
      this, url, std::move(callback)));
}

void SpecializedModesController::SaveGameInfo(const GameInfo& game_info) {
  if (game_info.title.empty()) {
    return;
  }
  
  // Check if we already have info for this game
  auto it = std::find_if(
      game_info_.begin(), game_info_.end(),
      [&game_info](const GameInfo& existing) {
        return existing.title == game_info.title;
      });
  
  if (it != game_info_.end()) {
    // Update existing game info
    if (!game_info.genre.empty()) {
      it->genre = game_info.genre;
    }
    if (!game_info.platform.empty()) {
      it->platform = game_info.platform;
    }
    if (!game_info.tips.empty()) {
      it->tips = game_info.tips;
    }
    if (!game_info.strategies.empty()) {
      it->strategies = game_info.strategies;
    }
    it->url = game_info.url;
    
    DLOG(INFO) << "Updated existing game info for: " << game_info.title;
  } else {
    // Check if we've reached the maximum number of game info entries
    int max_game_info = base::GetFieldTrialParamByFeatureAsInt(
        kAsolGamingMode, "max_game_info", 50);
    
    if (static_cast<int>(game_info_.size()) >= max_game_info) {
      // Remove the first entry (oldest)
      game_info_.erase(game_info_.begin());
    }
    
    // Add new game info
    game_info_.push_back(game_info);
    DLOG(INFO) << "Saved new game info for: " << game_info.title;
  }
  
  // Save to disk
  SaveData();
}

void SpecializedModesController::GetAllGameInfo(GameInfoCallback callback) {
  std::move(callback).Run(game_info_);
}

void SpecializedModesController::GetGameTips(
    const std::string& game_title,
    base::OnceCallback<void(const std::string&)> callback) {
  // Track performance of this operation
  util::ScopedPerformanceTracker tracker("SpecializedModesController_GetGameTips");
  
  // Check if we already have tips for this game
  auto it = std::find_if(
      game_info_.begin(), game_info_.end(),
      [&game_title](const GameInfo& info) {
        return info.title == game_title;
      });
  
  if (it != game_info_.end() && !it->tips.empty()) {
    // Use existing tips
    std::move(callback).Run(it->tips);
    return;
  }
  
  // Prepare the prompt
  std::string prompt = base::StringPrintf(kGameTipsPrompt, game_title.c_str());
  
  // Get the service manager
  auto* service_manager = core::ServiceManager::GetInstance();
  
  // Process the prompt with the best available adapter for text generation
  service_manager->ProcessTextWithCapabilityAsync(
      "text-generation", prompt,
      base::BindOnce([](SpecializedModesController* controller,
                      std::string game_title,
                      base::OnceCallback<void(const std::string&)> callback,
                      const adapters::ModelResponse& response) {
        if (response.success) {
          // Update the game info with the new tips
          auto it = std::find_if(
              controller->game_info_.begin(), controller->game_info_.end(),
              [&game_title](const GameInfo& info) {
                return info.title == game_title;
              });
          
          if (it != controller->game_info_.end()) {
            it->tips = response.text;
          } else {
            // Create new game info
            GameInfo new_info;
            new_info.title = game_title;
            new_info.tips = response.text;
            controller->SaveGameInfo(new_info);
          }
          
          std::move(callback).Run(response.text);
        } else {
          std::move(callback).Run("Failed to get game tips: " + 
                                 response.error_message);
        }
      },
      this, game_title, std::move(callback)));
}

void SpecializedModesController::GetGameStrategies(
    const std::string& game_title,
    base::OnceCallback<void(const std::string&)> callback) {
  // Track performance of this operation
  util::ScopedPerformanceTracker tracker("SpecializedModesController_GetGameStrategies");
  
  // Check if we already have strategies for this game
  auto it = std::find_if(
      game_info_.begin(), game_info_.end(),
      [&game_title](const GameInfo& info) {
        return info.title == game_title;
      });
  
  if (it != game_info_.end() && !it->strategies.empty()) {
    // Use existing strategies
    std::move(callback).Run(it->strategies);
    return;
  }
  
  // Prepare the prompt
  std::string prompt = base::StringPrintf(kGameStrategiesPrompt, game_title.c_str());
  
  // Get the service manager
  auto* service_manager = core::ServiceManager::GetInstance();
  
  // Process the prompt with the best available adapter for text generation
  service_manager->ProcessTextWithCapabilityAsync(
      "text-generation", prompt,
      base::BindOnce([](SpecializedModesController* controller,
                      std::string game_title,
                      base::OnceCallback<void(const std::string&)> callback,
                      const adapters::ModelResponse& response) {
        if (response.success) {
          // Update the game info with the new strategies
          auto it = std::find_if(
              controller->game_info_.begin(), controller->game_info_.end(),
              [&game_title](const GameInfo& info) {
                return info.title == game_title;
              });
          
          if (it != controller->game_info_.end()) {
            it->strategies = response.text;
          } else {
            // Create new game info
            GameInfo new_info;
            new_info.title = game_title;
            new_info.strategies = response.text;
            controller->SaveGameInfo(new_info);
          }
          
          std::move(callback).Run(response.text);
        } else {
          std::move(callback).Run("Failed to get game strategies: " + 
                                 response.error_message);
        }
      },
      this, game_title, std::move(callback)));
}

void SpecializedModesController::GenerateGameWalkthrough(
    const std::string& game_title,
    const std::string& level,
    base::OnceCallback<void(const std::string&)> callback) {
  // Track performance of this operation
  util::ScopedPerformanceTracker tracker("SpecializedModesController_GenerateGameWalkthrough");
  
  // Prepare the prompt
  std::string prompt = base::StringPrintf(
      kGameWalkthroughPrompt, level.c_str(), game_title.c_str());
  
  // Get the service manager
  auto* service_manager = core::ServiceManager::GetInstance();
  
  // Process the prompt with the best available adapter for text generation
  service_manager->ProcessTextWithCapabilityAsync(
      "text-generation", prompt,
      base::BindOnce([](base::OnceCallback<void(const std::string&)> callback,
                      const adapters::ModelResponse& response) {
        if (response.success) {
          std::move(callback).Run(response.text);
        } else {
          std::move(callback).Run("Failed to generate game walkthrough: " + 
                                 response.error_message);
        }
      },
      std::move(callback)));
}

void SpecializedModesController::OptimizeGameSettings(
    const std::string& game_title,
    const std::string& hardware,
    base::OnceCallback<void(const std::string&)> callback) {
  // Track performance of this operation
  util::ScopedPerformanceTracker tracker("SpecializedModesController_OptimizeGameSettings");
  
  // Prepare the prompt
  std::string prompt = base::StringPrintf(
      kOptimizeGameSettingsPrompt, game_title.c_str(), hardware.c_str());
  
  // Get the service manager
  auto* service_manager = core::ServiceManager::GetInstance();
  
  // Process the prompt with the best available adapter for text generation
  service_manager->ProcessTextWithCapabilityAsync(
      "text-generation", prompt,
      base::BindOnce([](base::OnceCallback<void(const std::string&)> callback,
                      const adapters::ModelResponse& response) {
        if (response.success) {
          std::move(callback).Run(response.text);
        } else {
          std::move(callback).Run("Failed to optimize game settings: " + 
                                 response.error_message);
        }
      },
      std::move(callback)));
}

void SpecializedModesController::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInPrimaryMainFrame() ||
      !navigation_handle->HasCommitted() ||
      navigation_handle->IsSameDocument()) {
    return;
  }
  
  // Perform mode-specific actions based on the current mode
  switch (current_mode_) {
    case SpecializedMode::kDeveloper:
      if (base::GetFieldTrialParamByFeatureAsBool(
              kAsolDeveloperMode, "auto_detect_code", true)) {
        ExtractCodeSnippets(base::DoNothing());
      }
      break;
    case SpecializedMode::kGaming:
      if (base::GetFieldTrialParamByFeatureAsBool(
              kAsolGamingMode, "auto_detect_games", true)) {
        DetectGame(base::DoNothing());
      }
      break;
    default:
      break;
  }
}

void SpecializedModesController::WebContentsDestroyed() {
  // Save data before web contents is destroyed
  SaveData();
}

void SpecializedModesController::SaveData() {
  if (!web_contents()) {
    return;
  }
  
  content::BrowserContext* browser_context = web_contents()->GetBrowserContext();
  if (!browser_context) {
    return;
  }
  
  base::FilePath path = GetDataFilePath();
  if (path.empty()) {
    DLOG(ERROR) << "Failed to get specialized mode data file path";
    return;
  }
  
  // Convert data to JSON
  base::Value::Dict root;
  root.Set("current_mode", static_cast<int>(current_mode_));
  
  // Save code snippets
  base::Value::List snippets_list;
  for (const auto& snippet : code_snippets_) {
    snippets_list.Append(CodeSnippetToValue(snippet));
  }
  root.Set("code_snippets", std::move(snippets_list));
  
  // Save documents
  base::Value::List documents_list;
  for (const auto& document : documents_) {
    documents_list.Append(WorkDocumentToValue(document));
  }
  root.Set("documents", std::move(documents_list));
  
  // Save game info
  base::Value::List game_info_list;
  for (const auto& info : game_info_) {
    game_info_list.Append(GameInfoToValue(info));
  }
  root.Set("game_info", std::move(game_info_list));
  
  std::string json_string;
  if (!base::JSONWriter::WriteWithOptions(
          root, base::JSONWriter::OPTIONS_PRETTY_PRINT, &json_string)) {
    DLOG(ERROR) << "Failed to serialize specialized mode data to JSON";
    return;
  }
  
  // Write to file
  if (base::WriteFile(path, json_string.data(), json_string.size()) == -1) {
    DLOG(ERROR) << "Failed to write specialized mode data to file: " << path.value();
  }
}

void SpecializedModesController::LoadData() {
  if (!web_contents()) {
    return;
  }
  
  content::BrowserContext* browser_context = web_contents()->GetBrowserContext();
  if (!browser_context) {
    return;
  }
  
  base::FilePath path = GetDataFilePath();
  if (path.empty()) {
    DLOG(ERROR) << "Failed to get specialized mode data file path";
    return;
  }
  
  // Check if the file exists
  if (!base::PathExists(path)) {
    DLOG(INFO) << "Specialized mode data file does not exist: " << path.value();
    return;
  }
  
  // Read the file
  std::string json_string;
  if (!base::ReadFileToString(path, &json_string)) {
    DLOG(ERROR) << "Failed to read specialized mode data from file: " << path.value();
    return;
  }
  
  // Parse the JSON
  absl::optional<base::Value> value = base::JSONReader::Read(json_string);
  if (!value || !value->is_dict()) {
    DLOG(ERROR) << "Failed to parse specialized mode data JSON";
    return;
  }
  
  const base::Value::Dict& root = value->GetDict();
  
  // Get the current mode
  if (auto mode = root.FindInt("current_mode")) {
    current_mode_ = static_cast<SpecializedMode>(*mode);
  }
  
  // Get code snippets
  if (const base::Value::List* snippets_list = root.FindList("code_snippets")) {
    code_snippets_.clear();
    for (const auto& snippet_value : *snippets_list) {
      if (!snippet_value.is_dict()) continue;
      code_snippets_.push_back(ValueToCodeSnippet(snippet_value.GetDict()));
    }
  }
  
  // Get documents
  if (const base::Value::List* documents_list = root.FindList("documents")) {
    documents_.clear();
    for (const auto& document_value : *documents_list) {
      if (!document_value.is_dict()) continue;
      documents_.push_back(ValueToWorkDocument(document_value.GetDict()));
    }
  }
  
  // Get game info
  if (const base::Value::List* game_info_list = root.FindList("game_info")) {
    game_info_.clear();
    for (const auto& info_value : *game_info_list) {
      if (!info_value.is_dict()) continue;
      game_info_.push_back(ValueToGameInfo(info_value.GetDict()));
    }
  }
  
  DLOG(INFO) << "Loaded specialized mode data: " 
             << code_snippets_.size() << " code snippets, "
             << documents_.size() << " documents, "
             << game_info_.size() << " game info entries";
}

base::FilePath SpecializedModesController::GetDataFilePath() const {
  if (!web_contents()) {
    return base::FilePath();
  }
  
  content::BrowserContext* browser_context = web_contents()->GetBrowserContext();
  if (!browser_context) {
    return base::FilePath();
  }
  
  return GetSpecializedModeDataFilePath(browser_context);
}

}  // namespace browser
}  // namespace asol