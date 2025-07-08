// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser_core/ai/browser_ai_integration.h"

#include <utility>

#include "base/logging.h"
#include "base/strings/string_util.h"

namespace browser_core {
namespace ai {

BrowserAIIntegration::BrowserAIIntegration() {
  // Enable all features by default
  enabled_features_[FeatureType::PAGE_SUMMARIZATION] = true;
  enabled_features_[FeatureType::CONTENT_ANALYSIS] = true;
  enabled_features_[FeatureType::SMART_SEARCH] = true;
  enabled_features_[FeatureType::QUESTION_ANSWERING] = true;
  enabled_features_[FeatureType::CODE_ASSISTANCE] = true;
  enabled_features_[FeatureType::TRANSLATION] = true;
  enabled_features_[FeatureType::WRITING_ASSISTANCE] = true;
  enabled_features_[FeatureType::RESEARCH_ASSISTANT] = true;
}

BrowserAIIntegration::~BrowserAIIntegration() = default;

bool BrowserAIIntegration::Initialize(
    BrowserEngine* browser_engine,
    asol::core::AIServiceManager* ai_service_manager) {
  
  if (!browser_engine || !ai_service_manager) {
    LOG(ERROR) << "Failed to initialize BrowserAIIntegration: "
               << "browser_engine or ai_service_manager is null";
    return false;
  }
  
  browser_engine_ = browser_engine;
  ai_service_manager_ = ai_service_manager;
  
  LOG(INFO) << "BrowserAIIntegration initialized successfully";
  return true;
}

void BrowserAIIntegration::SummarizePage(
    int tab_id, 
    FeatureResultCallback callback) {
  
  if (!IsFeatureEnabled(FeatureType::PAGE_SUMMARIZATION)) {
    FeatureResult result;
    result.success = false;
    result.error_message = "Page summarization feature is disabled";
    std::move(callback).Run(result);
    return;
  }
  
  // Extract the page content
  ExtractPageContent(
      tab_id,
      base::BindOnce([](BrowserAIIntegration* self,
                      FeatureResultCallback callback,
                      const std::string& content) {
        if (content.empty()) {
          FeatureResult result;
          result.success = false;
          result.error_message = "Failed to extract page content";
          std::move(callback).Run(result);
          return;
        }
        
        // Create AI request parameters
        asol::core::AIServiceManager::AIRequestParams params;
        params.task_type = asol::core::AIServiceManager::TaskType::TEXT_SUMMARIZATION;
        params.input_text = content;
        
        // Process the request
        self->ai_service_manager_->ProcessRequest(
            params,
            base::BindOnce(&BrowserAIIntegration::OnAIResponse,
                          self->weak_ptr_factory_.GetWeakPtr(),
                          std::move(callback)));
      }, base::Unretained(this), std::move(callback)));
}

void BrowserAIIntegration::AnalyzePageContent(
    int tab_id, 
    FeatureResultCallback callback) {
  
  if (!IsFeatureEnabled(FeatureType::CONTENT_ANALYSIS)) {
    FeatureResult result;
    result.success = false;
    result.error_message = "Content analysis feature is disabled";
    std::move(callback).Run(result);
    return;
  }
  
  // Extract the page content
  ExtractPageContent(
      tab_id,
      base::BindOnce([](BrowserAIIntegration* self,
                      FeatureResultCallback callback,
                      const std::string& content) {
        if (content.empty()) {
          FeatureResult result;
          result.success = false;
          result.error_message = "Failed to extract page content";
          std::move(callback).Run(result);
          return;
        }
        
        // Create AI request parameters
        asol::core::AIServiceManager::AIRequestParams params;
        params.task_type = asol::core::AIServiceManager::TaskType::CONTENT_ANALYSIS;
        params.input_text = content;
        
        // Process the request
        self->ai_service_manager_->ProcessRequest(
            params,
            base::BindOnce(&BrowserAIIntegration::OnAIResponse,
                          self->weak_ptr_factory_.GetWeakPtr(),
                          std::move(callback)));
      }, base::Unretained(this), std::move(callback)));
}

void BrowserAIIntegration::PerformSmartSearch(
    const std::string& query, 
    FeatureResultCallback callback) {
  
  if (!IsFeatureEnabled(FeatureType::SMART_SEARCH)) {
    FeatureResult result;
    result.success = false;
    result.error_message = "Smart search feature is disabled";
    std::move(callback).Run(result);
    return;
  }
  
  // Create AI request parameters
  asol::core::AIServiceManager::AIRequestParams params;
  params.task_type = asol::core::AIServiceManager::TaskType::TEXT_GENERATION;
  params.input_text = "Search query: " + query + 
                     "\nProvide a comprehensive answer to this search query.";
  
  // Process the request
  ai_service_manager_->ProcessRequest(
      params,
      base::BindOnce(&BrowserAIIntegration::OnAIResponse,
                    weak_ptr_factory_.GetWeakPtr(),
                    std::move(callback)));
}

void BrowserAIIntegration::AnswerQuestion(
    const std::string& question, 
    int tab_id,
    FeatureResultCallback callback) {
  
  if (!IsFeatureEnabled(FeatureType::QUESTION_ANSWERING)) {
    FeatureResult result;
    result.success = false;
    result.error_message = "Question answering feature is disabled";
    std::move(callback).Run(result);
    return;
  }
  
  // If tab_id is valid, extract page content for context
  if (tab_id >= 0) {
    ExtractPageContent(
        tab_id,
        base::BindOnce([](BrowserAIIntegration* self,
                        const std::string& question,
                        FeatureResultCallback callback,
                        const std::string& content) {
          // Create AI request parameters with page context
          asol::core::AIServiceManager::AIRequestParams params;
          params.task_type = asol::core::AIServiceManager::TaskType::QUESTION_ANSWERING;
          params.input_text = "Question: " + question + 
                             "\n\nContext from current page:\n" + content;
          
          // Process the request
          self->ai_service_manager_->ProcessRequest(
              params,
              base::BindOnce(&BrowserAIIntegration::OnAIResponse,
                            self->weak_ptr_factory_.GetWeakPtr(),
                            std::move(callback)));
        }, base::Unretained(this), question, std::move(callback)));
  } else {
    // No tab context, just answer the question
    asol::core::AIServiceManager::AIRequestParams params;
    params.task_type = asol::core::AIServiceManager::TaskType::QUESTION_ANSWERING;
    params.input_text = "Question: " + question;
    
    // Process the request
    ai_service_manager_->ProcessRequest(
        params,
        base::BindOnce(&BrowserAIIntegration::OnAIResponse,
                      weak_ptr_factory_.GetWeakPtr(),
                      std::move(callback)));
  }
}

void BrowserAIIntegration::ProvideCodeAssistance(
    const std::string& code, 
    const std::string& language,
    FeatureResultCallback callback) {
  
  if (!IsFeatureEnabled(FeatureType::CODE_ASSISTANCE)) {
    FeatureResult result;
    result.success = false;
    result.error_message = "Code assistance feature is disabled";
    std::move(callback).Run(result);
    return;
  }
  
  // Create AI request parameters
  asol::core::AIServiceManager::AIRequestParams params;
  params.task_type = asol::core::AIServiceManager::TaskType::CODE_GENERATION;
  params.input_text = "Code in " + language + ":\n" + code + 
                     "\n\nPlease analyze this code, explain what it does, " +
                     "and suggest any improvements or fixes.";
  
  // Add language as a custom parameter
  params.custom_params["language"] = language;
  
  // Process the request
  ai_service_manager_->ProcessRequest(
      params,
      base::BindOnce(&BrowserAIIntegration::OnAIResponse,
                    weak_ptr_factory_.GetWeakPtr(),
                    std::move(callback)));
}

void BrowserAIIntegration::TranslateContent(
    const std::string& content, 
    const std::string& target_language,
    FeatureResultCallback callback) {
  
  if (!IsFeatureEnabled(FeatureType::TRANSLATION)) {
    FeatureResult result;
    result.success = false;
    result.error_message = "Translation feature is disabled";
    std::move(callback).Run(result);
    return;
  }
  
  // Create AI request parameters
  asol::core::AIServiceManager::AIRequestParams params;
  params.task_type = asol::core::AIServiceManager::TaskType::TRANSLATION;
  params.input_text = content;
  
  // Add target language as a custom parameter
  params.custom_params["target_language"] = target_language;
  
  // Process the request
  ai_service_manager_->ProcessRequest(
      params,
      base::BindOnce(&BrowserAIIntegration::OnAIResponse,
                    weak_ptr_factory_.GetWeakPtr(),
                    std::move(callback)));
}

void BrowserAIIntegration::ProvideWritingAssistance(
    const std::string& text, 
    const std::string& task,
    FeatureResultCallback callback) {
  
  if (!IsFeatureEnabled(FeatureType::WRITING_ASSISTANCE)) {
    FeatureResult result;
    result.success = false;
    result.error_message = "Writing assistance feature is disabled";
    std::move(callback).Run(result);
    return;
  }
  
  // Create AI request parameters
  asol::core::AIServiceManager::AIRequestParams params;
  params.task_type = asol::core::AIServiceManager::TaskType::TEXT_GENERATION;
  
  // Construct the prompt based on the task
  std::string prompt;
  if (task == "improve") {
    prompt = "Please improve the following text while maintaining its meaning:\n\n";
  } else if (task == "proofread") {
    prompt = "Please proofread the following text and correct any errors:\n\n";
  } else if (task == "summarize") {
    prompt = "Please summarize the following text:\n\n";
  } else if (task == "expand") {
    prompt = "Please expand on the following text with more details and examples:\n\n";
  } else {
    prompt = "Please help with the following text:\n\n";
  }
  
  params.input_text = prompt + text;
  
  // Process the request
  ai_service_manager_->ProcessRequest(
      params,
      base::BindOnce(&BrowserAIIntegration::OnAIResponse,
                    weak_ptr_factory_.GetWeakPtr(),
                    std::move(callback)));
}

void BrowserAIIntegration::ResearchTopic(
    const std::string& topic, 
    FeatureResultCallback callback) {
  
  if (!IsFeatureEnabled(FeatureType::RESEARCH_ASSISTANT)) {
    FeatureResult result;
    result.success = false;
    result.error_message = "Research assistant feature is disabled";
    std::move(callback).Run(result);
    return;
  }
  
  // Create AI request parameters
  asol::core::AIServiceManager::AIRequestParams params;
  params.task_type = asol::core::AIServiceManager::TaskType::TEXT_GENERATION;
  params.input_text = "Research topic: " + topic + 
                     "\n\nPlease provide a comprehensive overview of this topic, " +
                     "including key concepts, important facts, and relevant resources " +
                     "for further reading.";
  
  // Process the request
  ai_service_manager_->ProcessRequest(
      params,
      base::BindOnce(&BrowserAIIntegration::OnAIResponse,
                    weak_ptr_factory_.GetWeakPtr(),
                    std::move(callback)));
}

void BrowserAIIntegration::EnableFeature(FeatureType feature, bool enable) {
  enabled_features_[feature] = enable;
  LOG(INFO) << "AI feature " << static_cast<int>(feature) 
            << (enable ? " enabled" : " disabled");
}

bool BrowserAIIntegration::IsFeatureEnabled(FeatureType feature) const {
  auto it = enabled_features_.find(feature);
  if (it != enabled_features_.end()) {
    return it->second;
  }
  return false;
}

base::WeakPtr<BrowserAIIntegration> BrowserAIIntegration::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void BrowserAIIntegration::ExtractPageContent(
    int tab_id,
    base::OnceCallback<void(const std::string&)> callback) {
  
  Tab* tab = browser_engine_->GetTabById(tab_id);
  if (!tab) {
    LOG(ERROR) << "Failed to extract page content: tab not found";
    std::move(callback).Run("");
    return;
  }
  
  WebContents* web_contents = tab->GetWebContents();
  if (!web_contents) {
    LOG(ERROR) << "Failed to extract page content: web contents not found";
    std::move(callback).Run("");
    return;
  }
  
  // Extract the main text from the page
  web_contents->ExtractMainText(std::move(callback));
}

void BrowserAIIntegration::OnAIResponse(
    FeatureResultCallback callback,
    bool success,
    const std::string& response) {
  
  FeatureResult result;
  result.success = success;
  
  if (success) {
    result.result = response;
  } else {
    result.error_message = "AI service error: " + response;
  }
  
  std::move(callback).Run(result);
}

}  // namespace ai
}  // namespace browser_core