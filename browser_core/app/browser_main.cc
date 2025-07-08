// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser_core/app/browser_main.h"

#include <utility>

#include "base/logging.h"
#include "base/strings/string_util.h"
#include "asol/adapters/gemini/gemini_service_provider.h"

namespace browser_core {
namespace app {

BrowserMain::BrowserMain() = default;

BrowserMain::~BrowserMain() = default;

bool BrowserMain::Initialize(const StartupParams& params) {
  LOG(INFO) << "Initializing DashAIBrowser...";
  
  // Initialize components in the correct order
  if (!InitializeBrowserEngine()) {
    LOG(ERROR) << "Failed to initialize browser engine";
    return false;
  }
  
  if (params.enable_ai_features && !InitializeAIComponents()) {
    LOG(ERROR) << "Failed to initialize AI components";
    return false;
  }
  
  if (params.enable_advanced_security && !InitializeSecurityComponents()) {
    LOG(ERROR) << "Failed to initialize security components";
    return false;
  }
  
  // Register AI providers
  if (params.enable_ai_features) {
    RegisterAIProviders();
  }
  
  // Navigate to the initial URL if provided
  if (!params.initial_url.empty()) {
    auto tab = browser_engine_->CreateTab();
    int tab_id = tab->GetId();
    browser_engine_->Navigate(tab_id, params.initial_url);
  }
  
  LOG(INFO) << "DashAIBrowser initialized successfully";
  return true;
}

int BrowserMain::Run() {
  LOG(INFO) << "Running DashAIBrowser main loop";
  
  // In a real implementation, this would run the main event loop
  // For now, we'll just return success
  return 0;
}

void BrowserMain::Shutdown() {
  LOG(INFO) << "Shutting down DashAIBrowser...";
  
  // Shutdown components in the reverse order of initialization
  content_filter_.reset();
  zero_knowledge_sync_.reset();
  ai_phishing_detector_.reset();
  research_assistant_.reset();
  smart_suggestions_.reset();
  voice_command_system_.reset();
  content_understanding_.reset();
  browser_ai_integration_.reset();
  security_manager_.reset();
  local_ai_processor_.reset();
  multi_model_orchestrator_.reset();
  ai_service_manager_.reset();
  browser_engine_.reset();
  
  LOG(INFO) << "DashAIBrowser shutdown complete";
}

BrowserEngine* BrowserMain::GetBrowserEngine() {
  return browser_engine_.get();
}

asol::core::AIServiceManager* BrowserMain::GetAIServiceManager() {
  return ai_service_manager_.get();
}

security::SecurityManager* BrowserMain::GetSecurityManager() {
  return security_manager_.get();
}

ai::BrowserAIIntegration* BrowserMain::GetBrowserAIIntegration() {
  return browser_ai_integration_.get();
}

ai::ContentUnderstanding* BrowserMain::GetContentUnderstanding() {
  return content_understanding_.get();
}

ai::VoiceCommandSystem* BrowserMain::GetVoiceCommandSystem() {
  return voice_command_system_.get();
}

ai::SmartSuggestions* BrowserMain::GetSmartSuggestions() {
  return smart_suggestions_.get();
}

ai::ResearchAssistant* BrowserMain::GetResearchAssistant() {
  return research_assistant_.get();
}

security::AIPhishingDetector* BrowserMain::GetAIPhishingDetector() {
  return ai_phishing_detector_.get();
}

security::ZeroKnowledgeSync* BrowserMain::GetZeroKnowledgeSync() {
  return zero_knowledge_sync_.get();
}

security::ContentFilter* BrowserMain::GetContentFilter() {
  return content_filter_.get();
}

asol::core::MultiModelOrchestrator* BrowserMain::GetMultiModelOrchestrator() {
  return multi_model_orchestrator_.get();
}

asol::core::LocalAIProcessor* BrowserMain::GetLocalAIProcessor() {
  return local_ai_processor_.get();
}

base::WeakPtr<BrowserMain> BrowserMain::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

bool BrowserMain::InitializeBrowserEngine() {
  browser_engine_ = std::make_unique<BrowserEngine>();
  return browser_engine_->Initialize();
}

bool BrowserMain::InitializeAIComponents() {
  // Initialize AI service manager
  ai_service_manager_ = std::make_unique<asol::core::AIServiceManager>();
  if (!ai_service_manager_->Initialize()) {
    LOG(ERROR) << "Failed to initialize AI service manager";
    return false;
  }
  
  // Initialize multi-model orchestrator
  multi_model_orchestrator_ = std::make_unique<asol::core::MultiModelOrchestrator>();
  if (!multi_model_orchestrator_->Initialize(ai_service_manager_.get())) {
    LOG(ERROR) << "Failed to initialize multi-model orchestrator";
    return false;
  }
  
  // Initialize local AI processor
  local_ai_processor_ = std::make_unique<asol::core::LocalAIProcessor>();
  if (!local_ai_processor_->Initialize()) {
    LOG(ERROR) << "Failed to initialize local AI processor";
    return false;
  }
  
  // Initialize browser AI integration
  browser_ai_integration_ = std::make_unique<ai::BrowserAIIntegration>();
  if (!browser_ai_integration_->Initialize(browser_engine_.get(), ai_service_manager_.get())) {
    LOG(ERROR) << "Failed to initialize browser AI integration";
    return false;
  }
  
  // Initialize content understanding
  content_understanding_ = std::make_unique<ai::ContentUnderstanding>();
  if (!content_understanding_->Initialize(ai_service_manager_.get())) {
    LOG(ERROR) << "Failed to initialize content understanding";
    return false;
  }
  
  // Initialize voice command system
  voice_command_system_ = std::make_unique<ai::VoiceCommandSystem>();
  if (!voice_command_system_->Initialize(browser_engine_.get(), ai_service_manager_.get())) {
    LOG(ERROR) << "Failed to initialize voice command system";
    return false;
  }
  
  // Initialize smart suggestions
  smart_suggestions_ = std::make_unique<ai::SmartSuggestions>();
  if (!smart_suggestions_->Initialize(browser_engine_.get(), ai_service_manager_.get(), content_understanding_.get())) {
    LOG(ERROR) << "Failed to initialize smart suggestions";
    return false;
  }
  
  // Initialize research assistant
  research_assistant_ = std::make_unique<ai::ResearchAssistant>();
  if (!research_assistant_->Initialize(browser_engine_.get(), ai_service_manager_.get(), content_understanding_.get())) {
    LOG(ERROR) << "Failed to initialize research assistant";
    return false;
  }
  
  return true;
}

bool BrowserMain::InitializeSecurityComponents() {
  // Initialize security manager
  security_manager_ = std::make_unique<security::SecurityManager>();
  if (!security_manager_->Initialize()) {
    LOG(ERROR) << "Failed to initialize security manager";
    return false;
  }
  
  // Initialize AI phishing detector
  ai_phishing_detector_ = std::make_unique<security::AIPhishingDetector>();
  if (!ai_phishing_detector_->Initialize(ai_service_manager_.get())) {
    LOG(ERROR) << "Failed to initialize AI phishing detector";
    return false;
  }
  
  // Initialize zero-knowledge sync
  zero_knowledge_sync_ = std::make_unique<security::ZeroKnowledgeSync>();
  if (!zero_knowledge_sync_->Initialize()) {
    LOG(ERROR) << "Failed to initialize zero-knowledge sync";
    return false;
  }
  
  // Initialize content filter
  content_filter_ = std::make_unique<security::ContentFilter>();
  if (!content_filter_->Initialize(ai_service_manager_.get())) {
    LOG(ERROR) << "Failed to initialize content filter";
    return false;
  }
  
  return true;
}

void BrowserMain::ParseCommandLine(const base::CommandLine& command_line, StartupParams* params) {
  if (command_line.HasSwitch("incognito")) {
    params->start_incognito = true;
  }
  
  if (command_line.HasSwitch("maximized")) {
    params->start_maximized = true;
  }
  
  if (command_line.HasSwitch("url")) {
    params->initial_url = command_line.GetSwitchValueASCII("url");
  }
  
  if (command_line.HasSwitch("user-data-dir")) {
    params->user_data_dir = command_line.GetSwitchValueASCII("user-data-dir");
  }
  
  if (command_line.HasSwitch("disable-logging")) {
    params->enable_logging = false;
  }
  
  if (command_line.HasSwitch("log-level")) {
    params->log_level = std::stoi(command_line.GetSwitchValueASCII("log-level"));
  }
  
  if (command_line.HasSwitch("disable-ai")) {
    params->enable_ai_features = false;
  }
  
  if (command_line.HasSwitch("disable-voice")) {
    params->enable_voice_commands = false;
  }
  
  if (command_line.HasSwitch("disable-research")) {
    params->enable_research_assistant = false;
  }
  
  if (command_line.HasSwitch("disable-advanced-security")) {
    params->enable_advanced_security = false;
  }
}

void BrowserMain::RegisterAIProviders() {
  // Register Gemini provider
  auto gemini_provider = std::make_unique<asol::adapters::gemini::GeminiServiceProvider>();
  ai_service_manager_->RegisterProvider(std::move(gemini_provider));
  
  // Register local AI processor as a provider
  ai_service_manager_->RegisterProvider(
      std::unique_ptr<asol::core::AIServiceProvider>(local_ai_processor_.get()));
  
  // Set default providers for different task types
  ai_service_manager_->SetDefaultProviderForTask(
      asol::core::AIServiceManager::TaskType::TEXT_GENERATION, "gemini");
  
  ai_service_manager_->SetDefaultProviderForTask(
      asol::core::AIServiceManager::TaskType::TEXT_SUMMARIZATION, "gemini");
  
  ai_service_manager_->SetDefaultProviderForTask(
      asol::core::AIServiceManager::TaskType::CONTENT_ANALYSIS, "gemini");
  
  ai_service_manager_->SetDefaultProviderForTask(
      asol::core::AIServiceManager::TaskType::QUESTION_ANSWERING, "gemini");
  
  ai_service_manager_->SetDefaultProviderForTask(
      asol::core::AIServiceManager::TaskType::CODE_GENERATION, "gemini");
  
  ai_service_manager_->SetDefaultProviderForTask(
      asol::core::AIServiceManager::TaskType::TRANSLATION, "gemini");
}

}  // namespace app
}  // namespace browser_core