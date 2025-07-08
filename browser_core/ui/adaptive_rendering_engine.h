// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_CORE_UI_ADAPTIVE_RENDERING_ENGINE_H_
#define BROWSER_CORE_UI_ADAPTIVE_RENDERING_ENGINE_H_

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "browser_core/engine/web_contents.h"
#include "asol/core/ai_service_manager.h"
#include "asol/core/context_manager.h"

namespace browser_core {
namespace ui {

// AdaptiveRenderingEngine provides AI-powered rendering optimizations
// that dynamically adjust content presentation based on device capabilities,
// user preferences, and cognitive load considerations.
class AdaptiveRenderingEngine {
 public:
  // Device capability information
  struct DeviceCapabilities {
    int screen_width;
    int screen_height;
    float pixel_ratio;
    bool is_mobile;
    bool is_tablet;
    bool is_touch_enabled;
    std::string browser_name;
    std::string browser_version;
    std::string os_name;
    std::string os_version;
  };

  // User cognitive profile
  struct CognitiveProfile {
    float reading_speed;       // Words per minute
    float attention_span;      // Estimated attention span in minutes
    float complexity_tolerance; // 0.0-1.0 tolerance for complex content
    std::string preferred_content_type;
    std::string preferred_learning_style;
    bool prefers_visual_content;
    bool prefers_reduced_motion;
    bool prefers_reduced_data;
    std::unordered_map<std::string, float> topic_expertise;
  };

  // Layout optimization suggestions
  struct LayoutOptimizations {
    bool success;
    std::string error_message;
    std::vector<std::pair<std::string, std::string>> style_modifications;
    std::vector<std::pair<std::string, std::string>> content_modifications;
    std::vector<std::pair<std::string, bool>> visibility_modifications;
    std::string custom_css;
    std::string custom_js;
    float estimated_cognitive_load_reduction;
    float estimated_performance_improvement;
  };

  // Callback for layout analysis
  using LayoutAnalysisCallback = 
      base::OnceCallback<void(const LayoutOptimizations&)>;

  AdaptiveRenderingEngine();
  ~AdaptiveRenderingEngine();

  // Disallow copy and assign
  AdaptiveRenderingEngine(const AdaptiveRenderingEngine&) = delete;
  AdaptiveRenderingEngine& operator=(const AdaptiveRenderingEngine&) = delete;

  // Initialize the engine
  bool Initialize(asol::core::AIServiceManager* ai_service_manager,
                asol::core::ContextManager* context_manager);

  // Analyze page layout and suggest optimizations
  void AnalyzeLayout(WebContents* web_contents,
                   const DeviceCapabilities& device_capabilities,
                   LayoutAnalysisCallback callback);

  // Apply optimizations to the rendered page
  void ApplyOptimizations(WebContents* web_contents,
                        const LayoutOptimizations& optimizations);

  // Update user cognitive profile
  void UpdateCognitiveProfile(const CognitiveProfile& profile);

  // Get current cognitive profile
  const CognitiveProfile& GetCognitiveProfile() const;

  // Enable/disable adaptive rendering
  void Enable(bool enable);
  bool IsEnabled() const;

  // Get a weak pointer to this instance
  base::WeakPtr<AdaptiveRenderingEngine> GetWeakPtr();

 private:
  // Helper methods
  void ExtractLayoutElements(WebContents* web_contents,
                           base::OnceCallback<void(const std::string&)> callback);

  void GenerateOptimizations(WebContents* web_contents,
                           const std::string& page_content,
                           const DeviceCapabilities& device_capabilities,
                           LayoutAnalysisCallback callback);

  std::string GenerateLayoutAnalysisPrompt(const std::string& page_content,
                                         const DeviceCapabilities& device_capabilities,
                                         const CognitiveProfile& cognitive_profile);

  LayoutOptimizations ParseAIResponse(const std::string& response);

  // Components
  asol::core::AIServiceManager* ai_service_manager_ = nullptr;
  asol::core::ContextManager* context_manager_ = nullptr;

  // State
  bool is_enabled_ = true;
  CognitiveProfile cognitive_profile_;

  // For weak pointers
  base::WeakPtrFactory<AdaptiveRenderingEngine> weak_ptr_factory_{this};
};

}  // namespace ui
}  // namespace browser_core

#endif  // BROWSER_CORE_UI_ADAPTIVE_RENDERING_ENGINE_H_