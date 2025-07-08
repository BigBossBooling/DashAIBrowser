// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_CORE_UI_SUMMARIZATION_UI_H_
#define BROWSER_CORE_UI_SUMMARIZATION_UI_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "browser_core/ai/summarization_service.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/widget/widget.h"
#include "ui/gfx/animation/animation_delegate.h"
#include "ui/gfx/animation/throb_animation.h"

namespace browser_core {
namespace ui {

// Forward declarations
class SynapseButton;
class SummarySidebar;

// SummarizationUI manages the UI components for the AI-Summarization feature.
// It includes the Synapse button in the Omnibox and the summary sidebar.
class SummarizationUI {
 public:
  // UI state
  enum class UIState {
    INACTIVE,     // Feature not available
    AVAILABLE,    // Feature available but not active
    LOADING,      // Summary is being generated
    ACTIVE,       // Summary is displayed
    ERROR         // Error occurred
  };

  // Callback for UI events
  using UIEventCallback = 
      base::RepeatingCallback<void(const std::string& event_type,
                                const std::string& event_data)>;

  SummarizationUI();
  ~SummarizationUI();

  // Disallow copy and assign
  SummarizationUI(const SummarizationUI&) = delete;
  SummarizationUI& operator=(const SummarizationUI&) = delete;

  // Initialize the UI
  bool Initialize(ai::SummarizationService* summarization_service);

  // Show/hide the Synapse button
  void ShowSynapseButton(views::View* parent_view);
  void HideSynapseButton();

  // Show/hide the summary sidebar
  void ShowSummarySidebar(views::Widget* parent_widget);
  void HideSummarySidebar();

  // Toggle the summary sidebar
  void ToggleSummarySidebar(views::Widget* parent_widget);

  // Set the content for summarization
  void SetContent(const std::string& content, const std::string& page_url);

  // Set the UI state
  void SetUIState(UIState state);
  UIState GetUIState() const;

  // Set the summary format
  void SetSummaryFormat(ai::SummarizationService::SummaryFormat format);

  // Set the summary length
  void SetSummaryLength(ai::SummarizationService::SummaryLength length);

  // Set callback for UI events
  void SetEventCallback(UIEventCallback callback);

  // Get a weak pointer to this instance
  base::WeakPtr<SummarizationUI> GetWeakPtr();

 private:
  // SynapseButton class - the pulsating icon in the Omnibox
  class SynapseButton : public views::Button,
                       public gfx::AnimationDelegate {
   public:
    explicit SynapseButton(SummarizationUI* owner);
    ~SynapseButton() override;

    // Initialize the button
    bool Initialize();

    // Start/stop pulsating animation
    void StartPulsating();
    void StopPulsating();

    // Set the button state
    void SetState(UIState state);

    // views::Button overrides
    void OnThemeChanged() override;
    void OnMouseEntered(const ui::MouseEvent& event) override;
    void OnMouseExited(const ui::MouseEvent& event) override;
    void NotifyClick(const ui::Event& event) override;

    // gfx::AnimationDelegate overrides
    void AnimationEnded(const gfx::Animation* animation) override;
    void AnimationProgressed(const gfx::Animation* animation) override;
    void AnimationCanceled(const gfx::Animation* animation) override;

   private:
    // Update button appearance based on state
    void UpdateAppearance();

    // Owner
    SummarizationUI* owner_ = nullptr;

    // Pulsating animation
    std::unique_ptr<gfx::ThrobAnimation> throb_animation_;

    // Animation start time
    base::TimeTicks animation_start_time_;
  };

  // SummarySidebar class - the sidebar that displays the summary
  class SummarySidebar : public views::WidgetDelegate {
   public:
    explicit SummarySidebar(SummarizationUI* owner);
    ~SummarySidebar() override;

    // Initialize the sidebar
    bool Initialize();

    // Show the sidebar
    void Show(views::Widget* parent_widget);

    // Hide the sidebar
    void Hide();

    // Update the sidebar content
    void UpdateContent(const ai::SummarizationService::SummaryResult& result);

    // Set the sidebar state
    void SetState(UIState state);

    // views::WidgetDelegate overrides
    views::View* GetContentsView() override;
    std::u16string GetWindowTitle() const override;
    bool CanResize() const override;
    bool CanMaximize() const override;
    bool CanMinimize() const override;
    bool ShouldShowCloseButton() const override;

   private:
    // Generate HTML content for the sidebar
    std::string GenerateSidebarHTML(
        const ai::SummarizationService::SummaryResult& result);

    // Handle web view events
    void OnWebViewLoadCompleted();
    void OnLinkClicked(const std::string& url);

    // Owner
    SummarizationUI* owner_ = nullptr;

    // UI components
    std::unique_ptr<views::View> contents_view_;
    views::WebView* web_view_ = nullptr;
    views::Widget* sidebar_widget_ = nullptr;

    // Current state
    UIState state_ = UIState::INACTIVE;
  };

  // Handle summarization result
  void OnSummarizationComplete(const ai::SummarizationService::SummaryResult& result);

  // Handle Synapse button click
  void OnSynapseButtonClicked();

  // Trigger summarization
  void TriggerSummarization();

  // Components
  ai::SummarizationService* summarization_service_ = nullptr;
  std::unique_ptr<SynapseButton> synapse_button_;
  std::unique_ptr<SummarySidebar> summary_sidebar_;

  // Current content and state
  std::string content_;
  std::string page_url_;
  UIState ui_state_ = UIState::INACTIVE;
  ai::SummarizationService::SummaryFormat summary_format_ = 
      ai::SummarizationService::SummaryFormat::EXECUTIVE_SUMMARY;
  ai::SummarizationService::SummaryLength summary_length_ = 
      ai::SummarizationService::SummaryLength::MEDIUM;

  // Event callback
  UIEventCallback event_callback_;

  // For weak pointers
  base::WeakPtrFactory<SummarizationUI> weak_ptr_factory_{this};
};

}  // namespace ui
}  // namespace browser_core

#endif  // BROWSER_CORE_UI_SUMMARIZATION_UI_H_// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_CORE_UI_SUMMARIZATION_UI_H_
#define BROWSER_CORE_UI_SUMMARIZATION_UI_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "browser_core/ai/summarization_service.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/widget/widget.h"
#include "ui/gfx/animation/animation_delegate.h"
#include "ui/gfx/animation/throb_animation.h"

namespace browser_core {
namespace ui {

// Forward declarations
class SynapseButton;
class SummarySidebar;

// SummarizationUI manages the UI components for the AI-Summarization feature.
// It includes the Synapse button in the Omnibox and the summary sidebar.
class SummarizationUI {
 public:
  // UI state
  enum class UIState {
    INACTIVE,     // Feature not available
    AVAILABLE,    // Feature available but not active
    LOADING,      // Summary is being generated
    ACTIVE,       // Summary is displayed
    ERROR         // Error occurred
  };

  // Callback for UI events
  using UIEventCallback = 
      base::RepeatingCallback<void(const std::string& event_type,
                                const std::string& event_data)>;

  SummarizationUI();
  ~SummarizationUI();

  // Disallow copy and assign
  SummarizationUI(const SummarizationUI&) = delete;
  SummarizationUI& operator=(const SummarizationUI&) = delete;

  // Initialize the UI
  bool Initialize(ai::SummarizationService* summarization_service);

  // Show/hide the Synapse button
  void ShowSynapseButton(views::View* parent_view);
  void HideSynapseButton();

  // Show/hide the summary sidebar
  void ShowSummarySidebar(views::Widget* parent_widget);
  void HideSummarySidebar();

  // Toggle the summary sidebar
  void ToggleSummarySidebar(views::Widget* parent_widget);

  // Set the content for summarization
  void SetContent(const std::string& content, const std::string& page_url);

  // Set the UI state
  void SetUIState(UIState state);
  UIState GetUIState() const;

  // Set the summary format
  void SetSummaryFormat(ai::SummarizationService::SummaryFormat format);

  // Set the summary length
  void SetSummaryLength(ai::SummarizationService::SummaryLength length);

  // Set callback for UI events
  void SetEventCallback(UIEventCallback callback);

  // Get a weak pointer to this instance
  base::WeakPtr<SummarizationUI> GetWeakPtr();

 private:
  // SynapseButton class - the pulsating icon in the Omnibox
  class SynapseButton : public views::Button,
                       public gfx::AnimationDelegate {
   public:
    explicit SynapseButton(SummarizationUI* owner);
    ~SynapseButton() override;

    // Initialize the button
    bool Initialize();

    // Start/stop pulsating animation
    void StartPulsating();
    void StopPulsating();

    // Set the button state
    void SetState(UIState state);

    // views::Button overrides
    void OnThemeChanged() override;
    void OnMouseEntered(const ui::MouseEvent& event) override;
    void OnMouseExited(const ui::MouseEvent& event) override;
    void NotifyClick(const ui::Event& event) override;

    // gfx::AnimationDelegate overrides
    void AnimationEnded(const gfx::Animation* animation) override;
    void AnimationProgressed(const gfx::Animation* animation) override;
    void AnimationCanceled(const gfx::Animation* animation) override;

   private:
    // Update button appearance based on state
    void UpdateAppearance();

    // Owner
    SummarizationUI* owner_ = nullptr;

    // Pulsating animation
    std::unique_ptr<gfx::ThrobAnimation> throb_animation_;

    // Animation start time
    base::TimeTicks animation_start_time_;
  };

  // SummarySidebar class - the sidebar that displays the summary
  class SummarySidebar : public views::WidgetDelegate {
   public:
    explicit SummarySidebar(SummarizationUI* owner);
    ~SummarySidebar() override;

    // Initialize the sidebar
    bool Initialize();

    // Show the sidebar
    void Show(views::Widget* parent_widget);

    // Hide the sidebar
    void Hide();

    // Update the sidebar content
    void UpdateContent(const ai::SummarizationService::SummaryResult& result);

    // Set the sidebar state
    void SetState(UIState state);

    // views::WidgetDelegate overrides
    views::View* GetContentsView() override;
    std::u16string GetWindowTitle() const override;
    bool CanResize() const override;
    bool CanMaximize() const override;
    bool CanMinimize() const override;
    bool ShouldShowCloseButton() const override;

   private:
    // Generate HTML content for the sidebar
    std::string GenerateSidebarHTML(
        const ai::SummarizationService::SummaryResult& result);

    // Handle web view events
    void OnWebViewLoadCompleted();
    void OnLinkClicked(const std::string& url);

    // Owner
    SummarizationUI* owner_ = nullptr;

    // UI components
    std::unique_ptr<views::View> contents_view_;
    views::WebView* web_view_ = nullptr;
    views::Widget* sidebar_widget_ = nullptr;

    // Current state
    UIState state_ = UIState::INACTIVE;
  };

  // Handle summarization result
  void OnSummarizationComplete(const ai::SummarizationService::SummaryResult& result);

  // Handle Synapse button click
  void OnSynapseButtonClicked();

  // Trigger summarization
  void TriggerSummarization();

  // Components
  ai::SummarizationService* summarization_service_ = nullptr;
  std::unique_ptr<SynapseButton> synapse_button_;
  std::unique_ptr<SummarySidebar> summary_sidebar_;

  // Current content and state
  std::string content_;
  std::string page_url_;
  UIState ui_state_ = UIState::INACTIVE;
  ai::SummarizationService::SummaryFormat summary_format_ = 
      ai::SummarizationService::SummaryFormat::EXECUTIVE_SUMMARY;
  ai::SummarizationService::SummaryLength summary_length_ = 
      ai::SummarizationService::SummaryLength::MEDIUM;

  // Event callback
  UIEventCallback event_callback_;

  // For weak pointers
  base::WeakPtrFactory<SummarizationUI> weak_ptr_factory_{this};
};

}  // namespace ui
}  // namespace browser_core

#endif  // BROWSER_CORE_UI_SUMMARIZATION_UI_H_