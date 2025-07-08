// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser_core/browser_ai_integration.h"

#include "base/bind.h"
#include "base/logging.h"

namespace browser_core {

BrowserAIIntegration::BrowserAIIntegration()
    : weak_ptr_factory_(this) {}

BrowserAIIntegration::~BrowserAIIntegration() = default;

bool BrowserAIIntegration::Initialize(
    asol::core::AIServiceManager* ai_service_manager,
    asol::core::PrivacyProxy* privacy_proxy) {
  // Initialize the multi-adapter manager
  InitializeMultiAdapterManager();
  
  // Initialize browser features
  browser_features_ = std::make_unique<BrowserFeatures>();
  if (!browser_features_->Initialize(ai_service_manager, privacy_proxy)) {
    return false;
  }
  
  // Initialize browser content handler
  browser_content_handler_ = std::make_unique<BrowserContentHandler>();
  if (!browser_content_handler_->Initialize(browser_features_.get())) {
    return false;
  }
  
  // Initialize AI settings page
  ai_settings_page_ = std::make_unique<ui::AISettingsPage>(multi_adapter_manager_.get());
  ai_settings_page_->Initialize();
  
  LOG(INFO) << "BrowserAIIntegration initialized with multiple AI adapters.";
  
  return true;
}

bool BrowserAIIntegration::InitializeWithEngine(
    BrowserEngine* browser_engine,
    asol::core::AIServiceManager* ai_service_manager,
    asol::core::PrivacyProxy* privacy_proxy,
    asol::core::ContextManager* context_manager) {
  if (!browser_engine || !ai_service_manager || !privacy_proxy || !context_manager) {
    LOG(ERROR) << "Failed to initialize BrowserAIIntegration: missing required components";
    return false;
  }
  
  // Store external components
  browser_engine_ = browser_engine;
  context_manager_ = context_manager;
  
  // Initialize the multi-adapter manager
  InitializeMultiAdapterManager();
  
  // Initialize browser features
  browser_features_ = std::make_unique<BrowserFeatures>();
  if (!browser_features_->Initialize(ai_service_manager, privacy_proxy)) {
    LOG(ERROR) << "Failed to initialize browser features";
    return false;
  }
  
  // Initialize browser content handler
  browser_content_handler_ = std::make_unique<BrowserContentHandler>();
  if (!browser_content_handler_->Initialize(browser_features_.get())) {
    LOG(ERROR) << "Failed to initialize browser content handler";
    return false;
  }
  
  // Initialize AI settings page
  ai_settings_page_ = std::make_unique<ui::AISettingsPage>(multi_adapter_manager_.get());
  ai_settings_page_->Initialize();
  
  // Initialize content understanding
  content_understanding_ = std::make_unique<ai::ContentUnderstanding>();
  if (!content_understanding_->Initialize(browser_engine_, ai_service_manager)) {
    LOG(ERROR) << "Failed to initialize content understanding";
    return false;
  }
  
  // Initialize smart suggestions
  smart_suggestions_ = std::make_unique<ai::SmartSuggestions>();
  if (!smart_suggestions_->Initialize(browser_engine_, ai_service_manager, content_understanding_.get())) {
    LOG(ERROR) << "Failed to initialize smart suggestions";
    return false;
  }
  
  // Initialize predictive omnibox
  predictive_omnibox_ = std::make_unique<ui::PredictiveOmnibox>();
  if (!predictive_omnibox_->Initialize(
          browser_engine_, ai_service_manager, context_manager_,
          smart_suggestions_.get(), content_understanding_.get())) {
    LOG(ERROR) << "Failed to initialize predictive omnibox";
    return false;
  }
  
  // Initialize memory palace
  memory_palace_ = std::make_unique<ui::MemoryPalace>();
  if (!memory_palace_->Initialize(
          browser_engine_, ai_service_manager, context_manager_,
          content_understanding_.get())) {
    LOG(ERROR) << "Failed to initialize memory palace";
    return false;
  }
  
  // Initialize contextual manager
  contextual_manager_ = std::make_unique<ui::ContextualManager>();
  if (!contextual_manager_->Initialize(
          browser_engine_, ai_service_manager, context_manager_,
          content_understanding_.get())) {
    LOG(ERROR) << "Failed to initialize contextual manager";
    return false;
  }
  
  LOG(INFO) << "BrowserAIIntegration initialized with browser engine and AI components";
  
  return true;
}

BrowserFeatures* BrowserAIIntegration::GetBrowserFeatures() {
  return browser_features_.get();
}

BrowserContentHandler* BrowserAIIntegration::GetBrowserContentHandler() {
  return browser_content_handler_.get();
}

void BrowserAIIntegration::OnPageLoaded(
    const std::string& page_url,
    const std::string& html_content,
    views::View* toolbar_view,
    views::Widget* browser_widget) {
  // Forward to browser content handler
  browser_content_handler_->OnPageLoaded(
      page_url, html_content, toolbar_view, browser_widget);
      
  // Extract title from content (in a real implementation, this would be more robust)
  std::string title = "Untitled Page";
    size_t title_start = html_content.find("<title>");
  size_t title_end = html_content.find("</title>");
  if (title_start != std::string::npos && title_end != std::string::npos) {
    title_start += 7;  // Length of "<title>"
    if (title_start < title_end) {
      title = html_content.substr(title_start, title_end - title_start);
    }
  }
  
  // Record page visit in memory palace
  if (memory_palace_ && !page_url.empty()) {
    memory_palace_->RecordPageVisit(page_url, title, html_content);
  }
  
  // Update contextual manager
  if (contextual_manager_ && !page_url.empty()) {
    contextual_manager_->UpdateContext(page_url, title, html_content);
  }
}

void BrowserAIIntegration::OnPageUnloaded(const std::string& page_url) {
  // Forward to browser content handler
  browser_content_handler_->OnPageUnloaded(page_url);
}

void BrowserAIIntegration::OnBrowserClosed() {
  // Forward to browser content handler
  browser_content_handler_->OnBrowserClosed();
}

asol::core::MultiAdapterManager* BrowserAIIntegration::GetMultiAdapterManager() {
  return multi_adapter_manager_.get();
}

ui::AISettingsPage* BrowserAIIntegration::GetAISettingsPage() {
  return ai_settings_page_.get();
}

ui::PredictiveOmnibox* BrowserAIIntegration::GetPredictiveOmnibox() {
  return predictive_omnibox_.get();
}

ui::MemoryPalace* BrowserAIIntegration::GetMemoryPalace() {
  return memory_palace_.get();
}

ui::ContextualManager* BrowserAIIntegration::GetContextualManager() {
  return contextual_manager_.get();
}

ui::AdaptiveRenderingEngine* BrowserAIIntegration::GetAdaptiveRenderingEngine() {
  return nullptr;  // Not implemented yet
}

ui::SemanticSearch* BrowserAIIntegration::GetSemanticSearch() {
  return nullptr;  // Not implemented yet
}

ai::MultimediaUnderstanding* BrowserAIIntegration::GetMultimediaUnderstanding() {
  return nullptr;  // Not implemented yet
}

void BrowserAIIntegration::ShowAISettingsPage() {
  if (ai_settings_page_) {
    ai_settings_page_->Show();
  }
}

void BrowserAIIntegration::InitializeMultiAdapterManager() {
  // Create configuration for the adapters
  std::unordered_map<std::string, std::string> config;
  
  // In a real application, these would be loaded from secure storage
  // For now, we'll use placeholder values
  config["gemini_api_key"] = "GEMINI_API_KEY";
  config["openai_api_key"] = "OPENAI_API_KEY";
  config["copilot_api_key"] = "COPILOT_API_KEY";
  config["claude_api_key"] = "CLAUDE_API_KEY";
  
  // Set default provider
  config["default_provider"] = "gemini";
  
  // Create the multi-adapter manager
  multi_adapter_manager_ = asol::adapters::AdapterFactory::CreateMultiAdapterManager(config);
  
  // Log available providers
  LOG(INFO) << "Initialized AI adapters:";
  for (const auto& provider_id : multi_adapter_manager_->GetRegisteredProviderIds()) {
    auto provider = multi_adapter_manager_->GetProvider(provider_id);
    LOG(INFO) << "- " << provider->GetProviderName() 
              << " (ID: " << provider_id << ")";
  }
  
  LOG(INFO) << "Active provider: " << multi_adapter_manager_->GetActiveProviderId();
}

base::WeakPtr<BrowserAIIntegration> BrowserAIIntegration::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

}  // namespace browser_core