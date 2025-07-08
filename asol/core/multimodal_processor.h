// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASOL_CORE_MULTIMODAL_PROCESSOR_H_
#define ASOL_CORE_MULTIMODAL_PROCESSOR_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "asol/core/ai_service_provider.h"

namespace asol {
namespace core {

// Multimodal processor for handling voice, audio, and combined input types
class MultimodalProcessor {
 public:
  // Voice command result
  struct VoiceCommandResult {
    std::string transcribed_text;
    std::string detected_intent;
    double confidence_score = 0.0;
    bool success = false;
  };

  // Audio analysis result
  struct AudioAnalysisResult {
    std::string audio_type;
    std::vector<std::string> detected_features;
    std::string content_description;
    double confidence_score = 0.0;
    bool success = false;
  };

  // Holographic content interaction result
  struct HolographicResult {
    std::string spatial_description;
    std::vector<std::string> interaction_points;
    std::string rendered_content;
    bool success = false;
  };

  using VoiceCallback = base::OnceCallback<void(const VoiceCommandResult&)>;
  using AudioCallback = base::OnceCallback<void(const AudioAnalysisResult&)>;
  using HolographicCallback = base::OnceCallback<void(const HolographicResult&)>;

  MultimodalProcessor();
  ~MultimodalProcessor();

  // Disallow copy and assign
  MultimodalProcessor(const MultimodalProcessor&) = delete;
  MultimodalProcessor& operator=(const MultimodalProcessor&) = delete;

  // Initialize the multimodal processor
  bool Initialize();

  // Voice processing capabilities
  void ProcessVoiceCommand(const std::vector<uint8_t>& audio_data,
                         VoiceCallback callback);

  void ProcessSpeechRecognition(const std::vector<uint8_t>& audio_data,
                              base::OnceCallback<void(const std::string&)> callback);

  // Audio analysis capabilities
  void AnalyzeAudioContent(const std::vector<uint8_t>& audio_data,
                         AudioCallback callback);

  void DetectAudioFeatures(const std::vector<uint8_t>& audio_data,
                         base::OnceCallback<void(const std::vector<std::string>&)> callback);

  // Holographic content interaction
  void ProcessHolographicContent(const std::string& web_content,
                               const std::vector<uint8_t>& spatial_data,
                               HolographicCallback callback);

  void RenderSpatialInterface(const std::string& content,
                            base::OnceCallback<void(const std::string&)> callback);

  // Combined multimodal processing
  void ProcessMultimodalInput(const std::string& text,
                            const std::vector<uint8_t>& image_data,
                            const std::vector<uint8_t>& audio_data,
                            AIServiceProvider::AIResponseCallback callback);

  // Enable/disable multimodal features
  void EnableFeature(const std::string& feature_name, bool enabled);
  bool IsFeatureEnabled(const std::string& feature_name) const;

  // Get a weak pointer to this instance
  base::WeakPtr<MultimodalProcessor> GetWeakPtr();

 private:
  // Helper methods for processing
  std::string TranscribeAudio(const std::vector<uint8_t>& audio_data);
  std::string AnalyzeImageContent(const std::vector<uint8_t>& image_data);
  std::string CombineMultimodalInputs(const std::string& text,
                                    const std::string& image_analysis,
                                    const std::string& audio_transcription);

  // Feature configuration
  std::unordered_map<std::string, bool> enabled_features_;

  // For weak pointers
  base::WeakPtrFactory<MultimodalProcessor> weak_ptr_factory_{this};
};

}  // namespace core
}  // namespace asol

#endif  // ASOL_CORE_MULTIMODAL_PROCESSOR_H_
