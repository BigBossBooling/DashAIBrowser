// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser_core/ai/summarization/synapse_button.h"

#include "base/bind.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/image/image_skia_operations.h"

namespace browser_core {
namespace ai {
namespace summarization {

namespace {

// Animation parameters
constexpr int kAnimationDurationMs = 2000;
constexpr float kAnimationThrobScale = 1.2f;
constexpr int kAnimationCycleCount = 3;

// Button size
constexpr int kButtonSize = 20;

// Helper function to create a colored icon
gfx::ImageSkia CreateColoredIcon(const gfx::VectorIcon& icon,
                               SkColor color,
                               int size) {
  return gfx::CreateVectorIcon(icon, size, color);
}

// Helper function to create a pulsating icon
gfx::ImageSkia CreatePulsatingIcon(const gfx::ImageSkia& base_icon,
                                 float scale,
                                 float alpha) {
  // Scale the icon
  gfx::ImageSkia scaled_icon = gfx::ImageSkiaOperations::CreateResizedImage(
      base_icon, skia::ImageOperations::RESIZE_BEST,
      gfx::Size(base_icon.width() * scale, base_icon.height() * scale));
  
  // Apply alpha
  return gfx::ImageSkiaOperations::CreateTransparentImage(scaled_icon, alpha);
}

}  // namespace

SynapseButton::SynapseButton(const std::string& page_url)
    : views::ImageButton(base::BindRepeating(&SynapseButton::OnButtonClicked,
                                           base::Unretained(this))),
      page_url_(page_url),
      weak_ptr_factory_(this) {
  // Set up the throb animation
  throb_animation_ = std::make_unique<gfx::ThrobAnimation>(this);
  throb_animation_->SetThrobDuration(kAnimationDurationMs);
  throb_animation_->SetTweenType(gfx::Tween::EASE_IN_OUT);
}

SynapseButton::~SynapseButton() = default;

bool SynapseButton::Initialize() {
  // Load button images
  LoadButtonImages();
  
  // Set initial appearance
  SetState(ButtonState::INACTIVE);
  
  // Set button size
  SetPreferredSize(gfx::Size(kButtonSize, kButtonSize));
  
  // Set tooltip
  SetTooltipText(u"Summarize this page");
  
  return true;
}

void SynapseButton::SetState(ButtonState state) {
  if (state_ == state)
    return;
  
  ButtonState old_state = state_;
  state_ = state;
  
  // Handle state transition
  switch (state_) {
    case ButtonState::INACTIVE:
      StopPulsating();
      SetVisible(false);
      break;
    case ButtonState::AVAILABLE:
      StopPulsating();
      SetVisible(true);
      break;
    case ButtonState::PULSATING:
      StartPulsating();
      SetVisible(true);
      break;
    case ButtonState::PROCESSING:
      StopPulsating();
      SetVisible(true);
      SetTooltipText(u"Generating summary...");
      break;
    case ButtonState::ERROR:
      StopPulsating();
      SetVisible(true);
      SetTooltipText(u"Error generating summary");
      break;
  }
  
  UpdateAppearance();
}

SynapseButton::ButtonState SynapseButton::GetState() const {
  return state_;
}

void SynapseButton::StartPulsating() {
  if (!throb_animation_->is_animating()) {
    animation_start_time_ = base::TimeTicks::Now();
    throb_animation_->StartThrobbing(kAnimationCycleCount);
  }
}

void SynapseButton::StopPulsating() {
  if (throb_animation_->is_animating()) {
    throb_animation_->Stop();
  }
}

void SynapseButton::SetPageUrl(const std::string& page_url) {
  page_url_ = page_url;
}

std::string SynapseButton::GetPageUrl() const {
  return page_url_;
}

void SynapseButton::SetClickCallback(ButtonEventCallback callback) {
  click_callback_ = std::move(callback);
}

void SynapseButton::OnThemeChanged() {
  views::ImageButton::OnThemeChanged();
  LoadButtonImages();
  UpdateAppearance();
}

void SynapseButton::OnMouseEntered(const ui::MouseEvent& event) {
  views::ImageButton::OnMouseEntered(event);
  
  // Stop pulsating when hovered
  if (state_ == ButtonState::PULSATING) {
    StopPulsating();
    UpdateAppearance();
  }
}

void SynapseButton::OnMouseExited(const ui::MouseEvent& event) {
  views::ImageButton::OnMouseExited(event);
  
  // Resume pulsating when mouse exits
  if (state_ == ButtonState::PULSATING) {
    StartPulsating();
  }
}

void SynapseButton::AnimationEnded(const gfx::Animation* animation) {
  if (animation == throb_animation_.get()) {
    // Check if we should restart the animation
    base::TimeDelta elapsed = base::TimeTicks::Now() - animation_start_time_;
    if (elapsed.InSeconds() < 60) {  // Restart if less than a minute has passed
      throb_animation_->StartThrobbing(kAnimationCycleCount);
    }
  }
}

void SynapseButton::AnimationProgressed(const gfx::Animation* animation) {
  if (animation == throb_animation_.get()) {
    // Update button appearance based on animation progress
    UpdateAppearance();
  }
}

void SynapseButton::AnimationCanceled(const gfx::Animation* animation) {
  if (animation == throb_animation_.get()) {
    // Reset to non-animated state
    UpdateAppearance();
  }
}

void SynapseButton::OnButtonClicked() {
  if (click_callback_ && state_ != ButtonState::INACTIVE && 
      state_ != ButtonState::PROCESSING) {
    click_callback_.Run(page_url_);
  }
}

void SynapseButton::UpdateAppearance() {
  switch (state_) {
    case ButtonState::INACTIVE:
      SetImage(views::Button::STATE_NORMAL, gfx::ImageSkia());
      break;
    case ButtonState::AVAILABLE:
      SetImage(views::Button::STATE_NORMAL, normal_image_);
      SetImage(views::Button::STATE_HOVERED, hovered_image_);
      SetImage(views::Button::STATE_PRESSED, pressed_image_);
      SetImage(views::Button::STATE_DISABLED, disabled_image_);
      break;
    case ButtonState::PULSATING: {
      if (throb_animation_->is_animating()) {
        // Calculate scale and alpha based on animation progress
        float progress = throb_animation_->GetCurrentValue();
        float scale = 1.0f + (kAnimationThrobScale - 1.0f) * progress;
        float alpha = 1.0f - 0.3f * progress;
        
        // Create pulsating icon
        gfx::ImageSkia pulsating_icon = 
            CreatePulsatingIcon(normal_image_, scale, alpha);
        
        SetImage(views::Button::STATE_NORMAL, pulsating_icon);
        SetImage(views::Button::STATE_HOVERED, hovered_image_);
        SetImage(views::Button::STATE_PRESSED, pressed_image_);
      } else {
        SetImage(views::Button::STATE_NORMAL, normal_image_);
        SetImage(views::Button::STATE_HOVERED, hovered_image_);
        SetImage(views::Button::STATE_PRESSED, pressed_image_);
      }
      break;
    }
    case ButtonState::PROCESSING:
      SetImage(views::Button::STATE_NORMAL, processing_image_);
      SetImage(views::Button::STATE_HOVERED, processing_image_);
      SetImage(views::Button::STATE_PRESSED, processing_image_);
      SetImage(views::Button::STATE_DISABLED, processing_image_);
      break;
    case ButtonState::ERROR:
      SetImage(views::Button::STATE_NORMAL, error_image_);
      SetImage(views::Button::STATE_HOVERED, error_image_);
      SetImage(views::Button::STATE_PRESSED, error_image_);
      SetImage(views::Button::STATE_DISABLED, error_image_);
      break;
  }
  
  SchedulePaint();
}

void SynapseButton::LoadButtonImages() {
  // In a real implementation, these would load from resources
  // For now, we'll create placeholder vector icons
  
  // Get colors from theme
  const ui::ColorProvider* color_provider = GetColorProvider();
  SkColor normal_color = color_provider ? 
      color_provider->GetColor(ui::kColorButtonBackground) : 
      gfx::kPlaceholderColor;
  SkColor hovered_color = color_provider ? 
      color_provider->GetColor(ui::kColorButtonBackgroundHovered) : 
      gfx::kPlaceholderColor;
  SkColor pressed_color = color_provider ? 
      color_provider->GetColor(ui::kColorButtonBackgroundPressed) : 
      gfx::kPlaceholderColor;
  SkColor disabled_color = color_provider ? 
      color_provider->GetColor(ui::kColorButtonBackgroundDisabled) : 
      gfx::kPlaceholderColor;
  SkColor error_color = SkColorSetRGB(0xE5, 0x39, 0x35);  // Red
  
  // Create placeholder icons
  // In a real implementation, we would use actual synapse icons
  // normal_image_ = CreateColoredIcon(kSynapseIcon, normal_color, kButtonSize);
  // hovered_image_ = CreateColoredIcon(kSynapseIcon, hovered_color, kButtonSize);
  // pressed_image_ = CreateColoredIcon(kSynapseIcon, pressed_color, kButtonSize);
  // disabled_image_ = CreateColoredIcon(kSynapseIcon, disabled_color, kButtonSize);
  // processing_image_ = CreateColoredIcon(kSynapseProcessingIcon, normal_color, kButtonSize);
  // error_image_ = CreateColoredIcon(kSynapseErrorIcon, error_color, kButtonSize);
  
  // For now, just create placeholder colored squares
  normal_image_ = gfx::ImageSkia();
  hovered_image_ = gfx::ImageSkia();
  pressed_image_ = gfx::ImageSkia();
  disabled_image_ = gfx::ImageSkia();
  processing_image_ = gfx::ImageSkia();
  error_image_ = gfx::ImageSkia();
}

}  // namespace summarization
}  // namespace ai
}  // namespace browser_core// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser_core/ai/summarization/synapse_button.h"

#include "base/bind.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/image/image_skia_operations.h"

namespace browser_core {
namespace ai {
namespace summarization {

namespace {

// Animation parameters
constexpr int kAnimationDurationMs = 2000;
constexpr float kAnimationThrobScale = 1.2f;
constexpr int kAnimationCycleCount = 3;

// Button size
constexpr int kButtonSize = 20;

// Helper function to create a colored icon
gfx::ImageSkia CreateColoredIcon(const gfx::VectorIcon& icon,
                               SkColor color,
                               int size) {
  return gfx::CreateVectorIcon(icon, size, color);
}

// Helper function to create a pulsating icon
gfx::ImageSkia CreatePulsatingIcon(const gfx::ImageSkia& base_icon,
                                 float scale,
                                 float alpha) {
  // Scale the icon
  gfx::ImageSkia scaled_icon = gfx::ImageSkiaOperations::CreateResizedImage(
      base_icon, skia::ImageOperations::RESIZE_BEST,
      gfx::Size(base_icon.width() * scale, base_icon.height() * scale));
  
  // Apply alpha
  return gfx::ImageSkiaOperations::CreateTransparentImage(scaled_icon, alpha);
}

}  // namespace

SynapseButton::SynapseButton(const std::string& page_url)
    : views::ImageButton(base::BindRepeating(&SynapseButton::OnButtonClicked,
                                           base::Unretained(this))),
      page_url_(page_url),
      weak_ptr_factory_(this) {
  // Set up the throb animation
  throb_animation_ = std::make_unique<gfx::ThrobAnimation>(this);
  throb_animation_->SetThrobDuration(kAnimationDurationMs);
  throb_animation_->SetTweenType(gfx::Tween::EASE_IN_OUT);
}

SynapseButton::~SynapseButton() = default;

bool SynapseButton::Initialize() {
  // Load button images
  LoadButtonImages();
  
  // Set initial appearance
  SetState(ButtonState::INACTIVE);
  
  // Set button size
  SetPreferredSize(gfx::Size(kButtonSize, kButtonSize));
  
  // Set tooltip
  SetTooltipText(u"Summarize this page");
  
  return true;
}

void SynapseButton::SetState(ButtonState state) {
  if (state_ == state)
    return;
  
  ButtonState old_state = state_;
  state_ = state;
  
  // Handle state transition
  switch (state_) {
    case ButtonState::INACTIVE:
      StopPulsating();
      SetVisible(false);
      break;
    case ButtonState::AVAILABLE:
      StopPulsating();
      SetVisible(true);
      break;
    case ButtonState::PULSATING:
      StartPulsating();
      SetVisible(true);
      break;
    case ButtonState::PROCESSING:
      StopPulsating();
      SetVisible(true);
      SetTooltipText(u"Generating summary...");
      break;
    case ButtonState::ERROR:
      StopPulsating();
      SetVisible(true);
      SetTooltipText(u"Error generating summary");
      break;
  }
  
  UpdateAppearance();
}

SynapseButton::ButtonState SynapseButton::GetState() const {
  return state_;
}

void SynapseButton::StartPulsating() {
  if (!throb_animation_->is_animating()) {
    animation_start_time_ = base::TimeTicks::Now();
    throb_animation_->StartThrobbing(kAnimationCycleCount);
  }
}

void SynapseButton::StopPulsating() {
  if (throb_animation_->is_animating()) {
    throb_animation_->Stop();
  }
}

void SynapseButton::SetPageUrl(const std::string& page_url) {
  page_url_ = page_url;
}

std::string SynapseButton::GetPageUrl() const {
  return page_url_;
}

void SynapseButton::SetClickCallback(ButtonEventCallback callback) {
  click_callback_ = std::move(callback);
}

void SynapseButton::OnThemeChanged() {
  views::ImageButton::OnThemeChanged();
  LoadButtonImages();
  UpdateAppearance();
}

void SynapseButton::OnMouseEntered(const ui::MouseEvent& event) {
  views::ImageButton::OnMouseEntered(event);
  
  // Stop pulsating when hovered
  if (state_ == ButtonState::PULSATING) {
    StopPulsating();
    UpdateAppearance();
  }
}

void SynapseButton::OnMouseExited(const ui::MouseEvent& event) {
  views::ImageButton::OnMouseExited(event);
  
  // Resume pulsating when mouse exits
  if (state_ == ButtonState::PULSATING) {
    StartPulsating();
  }
}

void SynapseButton::AnimationEnded(const gfx::Animation* animation) {
  if (animation == throb_animation_.get()) {
    // Check if we should restart the animation
    base::TimeDelta elapsed = base::TimeTicks::Now() - animation_start_time_;
    if (elapsed.InSeconds() < 60) {  // Restart if less than a minute has passed
      throb_animation_->StartThrobbing(kAnimationCycleCount);
    }
  }
}

void SynapseButton::AnimationProgressed(const gfx::Animation* animation) {
  if (animation == throb_animation_.get()) {
    // Update button appearance based on animation progress
    UpdateAppearance();
  }
}

void SynapseButton::AnimationCanceled(const gfx::Animation* animation) {
  if (animation == throb_animation_.get()) {
    // Reset to non-animated state
    UpdateAppearance();
  }
}

void SynapseButton::OnButtonClicked() {
  if (click_callback_ && state_ != ButtonState::INACTIVE && 
      state_ != ButtonState::PROCESSING) {
    click_callback_.Run(page_url_);
  }
}

void SynapseButton::UpdateAppearance() {
  switch (state_) {
    case ButtonState::INACTIVE:
      SetImage(views::Button::STATE_NORMAL, gfx::ImageSkia());
      break;
    case ButtonState::AVAILABLE:
      SetImage(views::Button::STATE_NORMAL, normal_image_);
      SetImage(views::Button::STATE_HOVERED, hovered_image_);
      SetImage(views::Button::STATE_PRESSED, pressed_image_);
      SetImage(views::Button::STATE_DISABLED, disabled_image_);
      break;
    case ButtonState::PULSATING: {
      if (throb_animation_->is_animating()) {
        // Calculate scale and alpha based on animation progress
        float progress = throb_animation_->GetCurrentValue();
        float scale = 1.0f + (kAnimationThrobScale - 1.0f) * progress;
        float alpha = 1.0f - 0.3f * progress;
        
        // Create pulsating icon
        gfx::ImageSkia pulsating_icon = 
            CreatePulsatingIcon(normal_image_, scale, alpha);
        
        SetImage(views::Button::STATE_NORMAL, pulsating_icon);
        SetImage(views::Button::STATE_HOVERED, hovered_image_);
        SetImage(views::Button::STATE_PRESSED, pressed_image_);
      } else {
        SetImage(views::Button::STATE_NORMAL, normal_image_);
        SetImage(views::Button::STATE_HOVERED, hovered_image_);
        SetImage(views::Button::STATE_PRESSED, pressed_image_);
      }
      break;
    }
    case ButtonState::PROCESSING:
      SetImage(views::Button::STATE_NORMAL, processing_image_);
      SetImage(views::Button::STATE_HOVERED, processing_image_);
      SetImage(views::Button::STATE_PRESSED, processing_image_);
      SetImage(views::Button::STATE_DISABLED, processing_image_);
      break;
    case ButtonState::ERROR:
      SetImage(views::Button::STATE_NORMAL, error_image_);
      SetImage(views::Button::STATE_HOVERED, error_image_);
      SetImage(views::Button::STATE_PRESSED, error_image_);
      SetImage(views::Button::STATE_DISABLED, error_image_);
      break;
  }
  
  SchedulePaint();
}

void SynapseButton::LoadButtonImages() {
  // In a real implementation, these would load from resources
  // For now, we'll create placeholder vector icons
  
  // Get colors from theme
  const ui::ColorProvider* color_provider = GetColorProvider();
  SkColor normal_color = color_provider ? 
      color_provider->GetColor(ui::kColorButtonBackground) : 
      gfx::kPlaceholderColor;
  SkColor hovered_color = color_provider ? 
      color_provider->GetColor(ui::kColorButtonBackgroundHovered) : 
      gfx::kPlaceholderColor;
  SkColor pressed_color = color_provider ? 
      color_provider->GetColor(ui::kColorButtonBackgroundPressed) : 
      gfx::kPlaceholderColor;
  SkColor disabled_color = color_provider ? 
      color_provider->GetColor(ui::kColorButtonBackgroundDisabled) : 
      gfx::kPlaceholderColor;
  SkColor error_color = SkColorSetRGB(0xE5, 0x39, 0x35);  // Red
  
  // Create placeholder icons
  // In a real implementation, we would use actual synapse icons
  // normal_image_ = CreateColoredIcon(kSynapseIcon, normal_color, kButtonSize);
  // hovered_image_ = CreateColoredIcon(kSynapseIcon, hovered_color, kButtonSize);
  // pressed_image_ = CreateColoredIcon(kSynapseIcon, pressed_color, kButtonSize);
  // disabled_image_ = CreateColoredIcon(kSynapseIcon, disabled_color, kButtonSize);
  // processing_image_ = CreateColoredIcon(kSynapseProcessingIcon, normal_color, kButtonSize);
  // error_image_ = CreateColoredIcon(kSynapseErrorIcon, error_color, kButtonSize);
  
  // For now, just create placeholder colored squares
  normal_image_ = gfx::ImageSkia();
  hovered_image_ = gfx::ImageSkia();
  pressed_image_ = gfx::ImageSkia();
  disabled_image_ = gfx::ImageSkia();
  processing_image_ = gfx::ImageSkia();
  error_image_ = gfx::ImageSkia();
}

}  // namespace summarization
}  // namespace ai
}  // namespace browser_core