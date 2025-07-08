// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_CORE_AI_MULTIMEDIA_UNDERSTANDING_H_
#define BROWSER_CORE_AI_MULTIMEDIA_UNDERSTANDING_H_

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "browser_core/ai/content_understanding.h"
#include "browser_core/engine/web_contents.h"
#include "asol/core/ai_service_manager.h"

namespace browser_core {
namespace ai {

// MultimediaUnderstanding extends ContentUnderstanding with capabilities
// for analyzing video and audio content, providing real-time understanding
// of multimedia elements on web pages.
class MultimediaUnderstanding : public ContentUnderstanding {
 public:
  // Video object information
  struct VideoObject {
    std::string name;
    std::string description;
    float confidence;
    std::vector<std::pair<int, int>> bounding_box;  // x, y, width, height
    float start_time;  // Seconds from start of video
    float end_time;    // Seconds from start of video
  };

  // Video scene information
  struct VideoScene {
    std::string description;
    std::vector<std::string> objects;
    std::vector<std::string> actions;
    std::string setting;
    float start_time;  // Seconds from start of video
    float end_time;    // Seconds from start of video
  };

  // Video analysis result
  struct VideoAnalysisResult {
    bool success;
    std::string error_message;
    std::string title;
    std::string description;
    float duration;  // Seconds
    std::vector<VideoObject> objects;
    std::vector<VideoScene> scenes;
    std::vector<std::string> topics;
    std::string summary;
    std::unordered_map<std::string, std::string> metadata;
  };

  // Audio segment information
  struct AudioSegment {
    std::string speaker;
    std::string transcript;
    float start_time;  // Seconds from start of audio
    float end_time;    // Seconds from start of audio
    float confidence;
  };

  // Audio analysis result
  struct AudioAnalysisResult {
    bool success;
    std::string error_message;
    std::string title;
    std::string description;
    float duration;  // Seconds
    std::vector<AudioSegment> segments;
    std::string full_transcript;
    std::string summary;
    std::unordered_map<std::string, std::string> metadata;
  };

  // Callback types
  using VideoAnalysisCallback = 
      base::OnceCallback<void(const VideoAnalysisResult&)>;
  using AudioAnalysisCallback = 
      base::OnceCallback<void(const AudioAnalysisResult&)>;

  MultimediaUnderstanding();
  ~MultimediaUnderstanding() override;

  // Disallow copy and assign
  MultimediaUnderstanding(const MultimediaUnderstanding&) = delete;
  MultimediaUnderstanding& operator=(const MultimediaUnderstanding&) = delete;

  // Initialize with AI service manager
  bool Initialize(asol::core::AIServiceManager* ai_service_manager) override;

  // Analyze video element on a page
  void AnalyzeVideoElement(WebContents* web_contents,
                         const std::string& video_selector,
                         VideoAnalysisCallback callback);

  // Analyze audio element on a page
  void AnalyzeAudioElement(WebContents* web_contents,
                         const std::string& audio_selector,
                         AudioAnalysisCallback callback);

  // Transcribe audio content
  void TranscribeAudio(WebContents* web_contents,
                     const std::string& audio_selector,
                     base::OnceCallback<void(const std::string&)> callback);

  // Get a weak pointer to this instance
  base::WeakPtr<MultimediaUnderstanding> GetWeakPtr();

 private:
  // Helper methods for video analysis
  void ExtractVideoFrames(WebContents* web_contents,
                        const std::string& video_selector,
                        float interval_seconds,
                        base::OnceCallback<void(const std::vector<std::string>&)> callback);

  void AnalyzeVideoFrames(const std::vector<std::string>& frame_data,
                        VideoAnalysisCallback callback);

  // Helper methods for audio analysis
  void ExtractAudioData(WebContents* web_contents,
                      const std::string& audio_selector,
                      base::OnceCallback<void(const std::string&)> callback);

  void ProcessAudioTranscription(const std::string& audio_data,
                               AudioAnalysisCallback callback);

  // JavaScript for extracting video frames
  static const char* GetVideoFrameExtractionScript();

  // JavaScript for extracting audio data
  static const char* GetAudioDataExtractionScript();

  // For weak pointers
  base::WeakPtrFactory<MultimediaUnderstanding> weak_ptr_factory_{this};
};

}  // namespace ai
}  // namespace browser_core

#endif  // BROWSER_CORE_AI_MULTIMEDIA_UNDERSTANDING_H_