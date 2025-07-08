// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_CORE_BROWSER_AI_INTEGRATION_H_
#define BROWSER_CORE_BROWSER_AI_INTEGRATION_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "browser_core/browser_features.h"
#include "browser_core/browser_content_handler.h"
#include "asol/adapters/adapter_factory.h"
#include "asol/core/ai_service_manager.h"
#include "asol/core/multi_adapter_manager.h"
#include "asol/core/privacy_proxy.h"
#include "asol/core/context_manager.h"
#include "browser_core/ui/ai_settings_page.h"
#include "browser_core/ui/predictive_omnibox.h"
#include "browser_core/ui/memory_palace.h"
#include "browser_core/ui/adaptive_rendering_engine.h"
#include "browser_core/ui/semantic_search.h"
#include "browser_core/ai/smart_suggestions.h"
#include "browser_core/ai/content_understanding.h"
#include "browser_core/ai/multimedia_understanding.h"
#include "browser_core/engine/browser_engine.h"

namespace browser_core {

// BrowserAIIntegration is the main integration point for AI features in the browser.
// It initializes and coordinates all AI components and provides a unified interface
// for the browser to interact with AI features.
class BrowserAIIntegration {
 public:
  BrowserAIIntegration();
  ~BrowserAIIntegration();

  // Disallow copy and assign
  BrowserAIIntegration(const BrowserAIIntegration&) = delete;
  BrowserAIIntegration& operator=(const BrowserAIIntegration&) = delete;

  // Initialize the AI integration
  bool Initialize(asol::core::AIServiceManager* ai_service_manager,
                asol::core::PrivacyProxy* privacy_proxy);

  // Initialize with browser engine and context manager
  bool InitializeWithEngine(BrowserEngine* browser_engine,
                          asol::core::AIServiceManager* ai_service_manager,
                          asol::core::PrivacyProxy* privacy_proxy,
                          asol::core::ContextManager* context_manager);

  // Get the browser features
  BrowserFeatures* GetBrowserFeatures();

  // Get the browser content handler
  BrowserContentHandler* GetBrowserContentHandler();
  
  // Get the multi-adapter manager
  asol::core::MultiAdapterManager* GetMultiAdapterManager();
  
  // Get the AI settings page
  ui::AISettingsPage* GetAISettingsPage();
  
  // Get the predictive omnibox
  ui::PredictiveOmnibox* GetPredictiveOmnibox();
  
  // Get the memory palace
  ui::MemoryPalace* GetMemoryPalace();
  
  // Get the adaptive rendering engine
  ui::AdaptiveRenderingEngine* GetAdaptiveRenderingEngine();
  
  // Get the semantic search
  ui::SemanticSearch* GetSemanticSearch();
  
  // Get the multimedia understanding
  ai::MultimediaUnderstanding* GetMultimediaUnderstanding();
  
  // Show the AI settings page
  void ShowAISettingsPage();

  // Browser event handlers
  void OnPageLoaded(const std::string& page_url,
                  const std::string& html_content,
                  views::View* toolbar_view,
                  views::Widget* browser_widget);
  void OnPageUnloaded(const std::string& page_url);
  void OnBrowserClosed();

  // Get a weak pointer to this instance
  base::WeakPtr<BrowserAIIntegration> GetWeakPtr();

 private:
  // Initialize the multi-adapter manager
  void InitializeMultiAdapterManager();

  // Components
  std::unique_ptr<BrowserFeatures> browser_features_;
  std::unique_ptr<BrowserContentHandler> browser_content_handler_;
  std::unique_ptr<asol::core::MultiAdapterManager> multi_adapter_manager_;
  std::unique_ptr<ui::AISettingsPage> ai_settings_page_;
  std::unique_ptr<ui::PredictiveOmnibox> predictive_omnibox_;
  std::unique_ptr<ui::MemoryPalace> memory_palace_;
  std::unique_ptr<ai::SmartSuggestions> smart_suggestions_;
  std::unique_ptr<ai::ContentUnderstanding> content_understanding_;

  // External components (not owned)
  BrowserEngine* browser_engine_ = nullptr;
  asol::core::ContextManager* context_manager_ = nullptr;

  // For weak pointers
  base::WeakPtrFactory<BrowserAIIntegration> weak_ptr_factory_{this};
};

}  // namespace browser_core

#endif  // BROWSER_CORE_BROWSER_AI_INTEGRATION_H_