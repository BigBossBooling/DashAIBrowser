// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser_core/ui/adaptive_rendering_engine.h"

#include <sstream>
#include <algorithm>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_util.h"
#include "base/values.h"

namespace browser_core {
namespace ui {

namespace {

// Constants for AI prompts
constexpr char kLayoutAnalysisPrompt[] = 
    "Analyze the following web page content and suggest optimizations to improve "
    "readability, reduce cognitive load, and enhance user experience. "
    "Consider the device capabilities, user cognitive profile, and content importance. "
    "\n\nPage content:\n{page_content}\n\n"
    "Device capabilities:\n{device_capabilities}\n\n"
    "User cognitive profile:\n{cognitive_profile}\n\n"
    "Provide optimization suggestions in JSON format with the following fields: "
    "style_modifications (array of objects with selector and css_changes), "
    "content_modifications (array of objects with selector and content_changes), "
    "visibility_modifications (array of objects with selector and is_visible), "
    "custom_css (string), custom_js (string), "
    "estimated_cognitive_load_reduction (float 0.0-1.0), "
    "estimated_performance_improvement (float 0.0-1.0).";

// JavaScript for extracting page content
constexpr char kExtractPageContentScript[] = R"(
  (function() {
    // Extract main content
    const content = {
      title: document.title,
      url: window.location.href,
      headings: [],
      paragraphs: [],
      images: [],
      links: [],
      forms: [],
      layout: {}
    };
    
    // Extract headings
    const headings = document.querySelectorAll('h1, h2, h3, h4, h5, h6');
    for (let i = 0; i < headings.length; i++) {
      const heading = headings[i];
      content.headings.push({
        text: heading.textContent.trim(),
        level: parseInt(heading.tagName.substring(1)),
        position: {
          x: heading.getBoundingClientRect().left,
          y: heading.getBoundingClientRect().top
        }
      });
    }
    
    // Extract paragraphs
    const paragraphs = document.querySelectorAll('p');
    for (let i = 0; i < paragraphs.length; i++) {
      const paragraph = paragraphs[i];
      content.paragraphs.push({
        text: paragraph.textContent.trim(),
        length: paragraph.textContent.trim().length,
        position: {
          x: paragraph.getBoundingClientRect().left,
          y: paragraph.getBoundingClientRect().top
        }
      });
    }
    
    // Extract images
    const images = document.querySelectorAll('img');
    for (let i = 0; i < images.length; i++) {
      const image = images[i];
      content.images.push({
        src: image.src,
        alt: image.alt,
        width: image.width,
        height: image.height,
        position: {
          x: image.getBoundingClientRect().left,
          y: image.getBoundingClientRect().top
        }
      });
    }
    
    // Extract links
    const links = document.querySelectorAll('a');
    for (let i = 0; i < links.length; i++) {
      const link = links[i];
      content.links.push({
        href: link.href,
        text: link.textContent.trim(),
        position: {
          x: link.getBoundingClientRect().left,
          y: link.getBoundingClientRect().top
        }
      });
    }
    
    // Extract forms
    const forms = document.querySelectorAll('form');
    for (let i = 0; i < forms.length; i++) {
      const form = forms[i];
      content.forms.push({
        id: form.id,
        action: form.action,
        method: form.method,
        position: {
          x: form.getBoundingClientRect().left,
          y: form.getBoundingClientRect().top
        }
      });
    }
    
    // Extract layout information
    content.layout = {
      viewport: {
        width: window.innerWidth,
        height: window.innerHeight
      },
      body: {
        width: document.body.scrollWidth,
        height: document.body.scrollHeight
      }
    };
    
    return JSON.stringify(content);
  })();
)";

// JavaScript for applying optimizations
constexpr char kApplyOptimizationsScriptTemplate[] = R"(
  (function() {
    // Apply style modifications
    const styleModifications = $STYLE_MODIFICATIONS;
    for (const mod of styleModifications) {
      const elements = document.querySelectorAll(mod.selector);
      for (const el of elements) {
        Object.assign(el.style, mod.css_changes);
      }
    }
    
    // Apply content modifications
    const contentModifications = $CONTENT_MODIFICATIONS;
    for (const mod of contentModifications) {
      const elements = document.querySelectorAll(mod.selector);
      for (const el of elements) {
        if (mod.content_changes.text) {
          el.textContent = mod.content_changes.text;
        }
        if (mod.content_changes.html) {
          el.innerHTML = mod.content_changes.html;
        }
        if (mod.content_changes.attributes) {
          for (const [attr, value] of Object.entries(mod.content_changes.attributes)) {
            el.setAttribute(attr, value);
          }
        }
      }
    }
    
    // Apply visibility modifications
    const visibilityModifications = $VISIBILITY_MODIFICATIONS;
    for (const mod of visibilityModifications) {
      const elements = document.querySelectorAll(mod.selector);
      for (const el of elements) {
        el.style.display = mod.is_visible ? '' : 'none';
      }
    }
    
    // Apply custom CSS
    const customCSS = `$CUSTOM_CSS`;
    if (customCSS) {
      const styleEl = document.createElement('style');
      styleEl.id = 'dashai-adaptive-styles';
      styleEl.textContent = customCSS;
      document.head.appendChild(styleEl);
    }
    
    // Apply custom JS
    const customJS = `$CUSTOM_JS`;
    if (customJS) {
      try {
        eval(customJS);
      } catch (e) {
        console.error('Error executing custom JS:', e);
      }
    }
    
    return true;
  })();
)";

}  // namespace

AdaptiveRenderingEngine::AdaptiveRenderingEngine() {
  // Initialize default cognitive profile
  cognitive_profile_.reading_speed = 250.0f;  // Average reading speed
  cognitive_profile_.attention_span = 5.0f;   // 5 minutes
  cognitive_profile_.complexity_tolerance = 0.5f;
  cognitive_profile_.preferred_content_type = "mixed";
  cognitive_profile_.preferred_learning_style = "visual";
  cognitive_profile_.prefers_visual_content = true;
  cognitive_profile_.prefers_reduced_motion = false;
  cognitive_profile_.prefers_reduced_data = false;
}

AdaptiveRenderingEngine::~AdaptiveRenderingEngine() = default;

bool AdaptiveRenderingEngine::Initialize(
    asol::core::AIServiceManager* ai_service_manager,
    asol::core::ContextManager* context_manager) {
  if (!ai_service_manager || !context_manager) {
    return false;
  }

  ai_service_manager_ = ai_service_manager;
  context_manager_ = context_manager;
  
  // Load user cognitive profile from context manager
  context_manager_->GetUserContext(
      base::BindOnce([](
          AdaptiveRenderingEngine* self,
          const asol::core::ContextManager::UserContext& user_context) {
        // Update cognitive profile based on user context
        if (user_context.preferences.contains("reading_speed")) {
          self->cognitive_profile_.reading_speed = 
              std::stof(user_context.preferences.at("reading_speed"));
        }
        
        if (user_context.preferences.contains("attention_span")) {
          self->cognitive_profile_.attention_span = 
              std::stof(user_context.preferences.at("attention_span"));
        }
        
        if (user_context.preferences.contains("complexity_tolerance")) {
          self->cognitive_profile_.complexity_tolerance = 
              std::stof(user_context.preferences.at("complexity_tolerance"));
        }
        
        if (user_context.preferences.contains("preferred_content_type")) {
          self->cognitive_profile_.preferred_content_type = 
              user_context.preferences.at("preferred_content_type");
        }
        
        if (user_context.preferences.contains("preferred_learning_style")) {
          self->cognitive_profile_.preferred_learning_style = 
              user_context.preferences.at("preferred_learning_style");
        }
        
        if (user_context.preferences.contains("prefers_visual_content")) {
          self->cognitive_profile_.prefers_visual_content = 
              user_context.preferences.at("prefers_visual_content") == "true";
        }
        
        if (user_context.preferences.contains("prefers_reduced_motion")) {
          self->cognitive_profile_.prefers_reduced_motion = 
              user_context.preferences.at("prefers_reduced_motion") == "true";
        }
        
        if (user_context.preferences.contains("prefers_reduced_data")) {
          self->cognitive_profile_.prefers_reduced_data = 
              user_context.preferences.at("prefers_reduced_data") == "true";
        }
        
        // Update topic expertise based on user interests
        for (const auto& interest : user_context.interests) {
          self->cognitive_profile_.topic_expertise[interest] = 0.7f;  // Default expertise level
        }
      }, this));
  
  return true;
}

void AdaptiveRenderingEngine::AnalyzeLayout(
    WebContents* web_contents,
    const DeviceCapabilities& device_capabilities,
    LayoutAnalysisCallback callback) {
  if (!is_enabled_ || !web_contents) {
    LayoutOptimizations empty_result;
    empty_result.success = false;
    empty_result.error_message = "Adaptive rendering is disabled or web contents is null";
    std::move(callback).Run(empty_result);
    return;
  }

  // Extract page content
  ExtractLayoutElements(web_contents, 
      base::BindOnce([](
          AdaptiveRenderingEngine* self,
          WebContents* web_contents,
          const DeviceCapabilities& device_capabilities,
          LayoutAnalysisCallback callback,
          const std::string& page_content) {
        // Generate optimizations
        self->GenerateOptimizations(web_contents, 
                                  page_content, 
                                  device_capabilities, 
                                  std::move(callback));
      }, this, web_contents, device_capabilities, std::move(callback)));
}

void AdaptiveRenderingEngine::ExtractLayoutElements(
    WebContents* web_contents,
    base::OnceCallback<void(const std::string&)> callback) {
  // Execute JavaScript to extract page content
  web_contents->ExecuteJavaScript(
      kExtractPageContentScript,
      base::BindOnce([](
          base::OnceCallback<void(const std::string&)> callback,
          const WebContents::JavaScriptResult& result) {
        if (!result.success) {
          std::move(callback).Run("{}");
          return;
        }
        
        std::move(callback).Run(result.result);
      }, std::move(callback)));
}

void AdaptiveRenderingEngine::GenerateOptimizations(
    WebContents* web_contents,
    const std::string& page_content,
    const DeviceCapabilities& device_capabilities,
    LayoutAnalysisCallback callback) {
  // Generate AI prompt for layout analysis
  std::string prompt = GenerateLayoutAnalysisPrompt(page_content, 
                                                  device_capabilities, 
                                                  cognitive_profile_);
  
  // Request AI analysis
  ai_service_manager_->GetTextAdapter()->GenerateText(
      prompt,
      base::BindOnce([](
          AdaptiveRenderingEngine* self,
          LayoutAnalysisCallback callback,
          const asol::core::TextAdapter::GenerateTextResult& result) {
        if (!result.success) {
          LayoutOptimizations error_result;
          error_result.success = false;
          error_result.error_message = "Failed to generate AI analysis: " + result.error_message;
          std::move(callback).Run(error_result);
          return;
        }
        
        // Parse AI response
        LayoutOptimizations optimizations = self->ParseAIResponse(result.text);
        std::move(callback).Run(optimizations);
      }, this, std::move(callback)));
}

std::string AdaptiveRenderingEngine::GenerateLayoutAnalysisPrompt(
    const std::string& page_content,
    const DeviceCapabilities& device_capabilities,
    const CognitiveProfile& cognitive_profile) {
  std::string prompt = kLayoutAnalysisPrompt;
  
  // Format page content section (truncate if too long)
  std::string truncated_content = page_content;
  if (truncated_content.length() > 5000) {
    truncated_content = truncated_content.substr(0, 5000) + "... [content truncated]";
  }
  base::ReplaceSubstringsAfterOffset(&prompt, 0, "{page_content}", truncated_content);
  
  // Format device capabilities section
  std::stringstream device_stream;
  device_stream << "Screen size: " << device_capabilities.screen_width << "x" 
               << device_capabilities.screen_height << "\n";
  device_stream << "Pixel ratio: " << device_capabilities.pixel_ratio << "\n";
  device_stream << "Device type: " << (device_capabilities.is_mobile ? "Mobile" : 
                                     (device_capabilities.is_tablet ? "Tablet" : "Desktop")) << "\n";
  device_stream << "Touch enabled: " << (device_capabilities.is_touch_enabled ? "Yes" : "No") << "\n";
  device_stream << "Browser: " << device_capabilities.browser_name << " " 
               << device_capabilities.browser_version << "\n";
  device_stream << "OS: " << device_capabilities.os_name << " " 
               << device_capabilities.os_version << "\n";
  base::ReplaceSubstringsAfterOffset(&prompt, 0, "{device_capabilities}", device_stream.str());
  
  // Format cognitive profile section
  std::stringstream cognitive_stream;
  cognitive_stream << "Reading speed: " << cognitive_profile.reading_speed << " words per minute\n";
  cognitive_stream << "Attention span: " << cognitive_profile.attention_span << " minutes\n";
  cognitive_stream << "Complexity tolerance: " << cognitive_profile.complexity_tolerance << "\n";
  cognitive_stream << "Preferred content type: " << cognitive_profile.preferred_content_type << "\n";
  cognitive_stream << "Preferred learning style: " << cognitive_profile.preferred_learning_style << "\n";
  cognitive_stream << "Prefers visual content: " << (cognitive_profile.prefers_visual_content ? "Yes" : "No") << "\n";
  cognitive_stream << "Prefers reduced motion: " << (cognitive_profile.prefers_reduced_motion ? "Yes" : "No") << "\n";
  cognitive_stream << "Prefers reduced data: " << (cognitive_profile.prefers_reduced_data ? "Yes" : "No") << "\n";
  
  cognitive_stream << "Topic expertise:\n";
  for (const auto& [topic, expertise] : cognitive_profile.topic_expertise) {
    cognitive_stream << "  - " << topic << ": " << expertise << "\n";
  }
  base::ReplaceSubstringsAfterOffset(&prompt, 0, "{cognitive_profile}", cognitive_stream.str());
  
  return prompt;
}

AdaptiveRenderingEngine::LayoutOptimizations AdaptiveRenderingEngine::ParseAIResponse(
    const std::string& response) {
  LayoutOptimizations optimizations;
  optimizations.success = false;
  
  // Parse the JSON response
  absl::optional<base::Value> json = base::JSONReader::Read(response);
  if (!json || !json->is_dict()) {
    optimizations.error_message = "Failed to parse AI response as JSON";
    return optimizations;
  }
  
  const base::Value::Dict& dict = json->GetDict();
  
  // Parse style modifications
  const base::Value::List* style_mods = dict.FindList("style_modifications");
  if (style_mods) {
    for (const auto& mod : *style_mods) {
      if (!mod.is_dict()) continue;
      
      const base::Value::Dict& mod_dict = mod.GetDict();
      std::string selector = mod_dict.FindString("selector").value_or("");
      std::string css_changes = mod_dict.FindString("css_changes").value_or("");
      
      if (!selector.empty() && !css_changes.empty()) {
        optimizations.style_modifications.emplace_back(selector, css_changes);
      }
    }
  }
  
  // Parse content modifications
  const base::Value::List* content_mods = dict.FindList("content_modifications");
  if (content_mods) {
    for (const auto& mod : *content_mods) {
      if (!mod.is_dict()) continue;
      
      const base::Value::Dict& mod_dict = mod.GetDict();
      std::string selector = mod_dict.FindString("selector").value_or("");
      std::string content_changes = mod_dict.FindString("content_changes").value_or("");
      
      if (!selector.empty() && !content_changes.empty()) {
        optimizations.content_modifications.emplace_back(selector, content_changes);
      }
    }
  }
  
  // Parse visibility modifications
  const base::Value::List* visibility_mods = dict.FindList("visibility_modifications");
  if (visibility_mods) {
    for (const auto& mod : *visibility_mods) {
      if (!mod.is_dict()) continue;
      
      const base::Value::Dict& mod_dict = mod.GetDict();
      std::string selector = mod_dict.FindString("selector").value_or("");
      bool is_visible = mod_dict.FindBool("is_visible").value_or(true);
      
      if (!selector.empty()) {
        optimizations.visibility_modifications.emplace_back(selector, is_visible);
      }
    }
  }
  
  // Parse custom CSS and JS
  optimizations.custom_css = dict.FindString("custom_css").value_or("");
  optimizations.custom_js = dict.FindString("custom_js").value_or("");
  
  // Parse estimated improvements
  optimizations.estimated_cognitive_load_reduction = 
      dict.FindDouble("estimated_cognitive_load_reduction").value_or(0.0);
  optimizations.estimated_performance_improvement = 
      dict.FindDouble("estimated_performance_improvement").value_or(0.0);
  
  optimizations.success = true;
  return optimizations;
}

void AdaptiveRenderingEngine::ApplyOptimizations(
    WebContents* web_contents,
    const LayoutOptimizations& optimizations) {
  if (!is_enabled_ || !web_contents || !optimizations.success) {
    return;
  }
  
  // Convert optimizations to JSON for JavaScript
  base::Value::List style_mods_list;
  for (const auto& [selector, css_changes] : optimizations.style_modifications) {
    base::Value::Dict mod;
    mod.Set("selector", selector);
    mod.Set("css_changes", css_changes);
    style_mods_list.Append(std::move(mod));
  }
  std::string style_mods_json;
  base::JSONWriter::Write(style_mods_list, &style_mods_json);
  
  base::Value::List content_mods_list;
  for (const auto& [selector, content_changes] : optimizations.content_modifications) {
    base::Value::Dict mod;
    mod.Set("selector", selector);
    mod.Set("content_changes", content_changes);
    content_mods_list.Append(std::move(mod));
  }
  std::string content_mods_json;
  base::JSONWriter::Write(content_mods_list, &content_mods_json);
  
  base::Value::List visibility_mods_list;
  for (const auto& [selector, is_visible] : optimizations.visibility_modifications) {
    base::Value::Dict mod;
    mod.Set("selector", selector);
    mod.Set("is_visible", is_visible);
    visibility_mods_list.Append(std::move(mod));
  }
  std::string visibility_mods_json;
  base::JSONWriter::Write(visibility_mods_list, &visibility_mods_json);
  
  // Create the JavaScript to apply optimizations
  std::string script = kApplyOptimizationsScriptTemplate;
  base::ReplaceSubstringsAfterOffset(&script, 0, "$STYLE_MODIFICATIONS", style_mods_json);
  base::ReplaceSubstringsAfterOffset(&script, 0, "$CONTENT_MODIFICATIONS", content_mods_json);
  base::ReplaceSubstringsAfterOffset(&script, 0, "$VISIBILITY_MODIFICATIONS", visibility_mods_json);
  base::ReplaceSubstringsAfterOffset(&script, 0, "$CUSTOM_CSS", optimizations.custom_css);
  base::ReplaceSubstringsAfterOffset(&script, 0, "$CUSTOM_JS", optimizations.custom_js);
  
  // Execute the script
  web_contents->ExecuteJavaScript(script, base::DoNothing());
}

void AdaptiveRenderingEngine::UpdateCognitiveProfile(const CognitiveProfile& profile) {
  cognitive_profile_ = profile;
  
  // Update user preferences in context manager
  std::unordered_map<std::string, std::string> preferences;
  preferences["reading_speed"] = std::to_string(profile.reading_speed);
  preferences["attention_span"] = std::to_string(profile.attention_span);
  preferences["complexity_tolerance"] = std::to_string(profile.complexity_tolerance);
  preferences["preferred_content_type"] = profile.preferred_content_type;
  preferences["preferred_learning_style"] = profile.preferred_learning_style;
  preferences["prefers_visual_content"] = profile.prefers_visual_content ? "true" : "false";
  preferences["prefers_reduced_motion"] = profile.prefers_reduced_motion ? "true" : "false";
  preferences["prefers_reduced_data"] = profile.prefers_reduced_data ? "true" : "false";
  
  context_manager_->UpdateUserPreferences(preferences, base::DoNothing());
}

const AdaptiveRenderingEngine::CognitiveProfile& AdaptiveRenderingEngine::GetCognitiveProfile() const {
  return cognitive_profile_;
}

void AdaptiveRenderingEngine::Enable(bool enable) {
  is_enabled_ = enable;
}

bool AdaptiveRenderingEngine::IsEnabled() const {
  return is_enabled_;
}

base::WeakPtr<AdaptiveRenderingEngine> AdaptiveRenderingEngine::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

}  // namespace ui
}  // namespace browser_core