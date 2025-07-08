// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser_core/ai/multimedia_understanding.h"

#include <sstream>
#include <algorithm>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_util.h"
#include "base/values.h"

namespace browser_core {
namespace ai {

namespace {

// Constants for AI prompts
constexpr char kVideoAnalysisPrompt[] = 
    "Analyze the following video frames and provide a comprehensive understanding of the video content. "
    "Identify objects, scenes, actions, and topics. Generate a summary of the video content. "
    "\n\nVideo frames (base64 encoded images with timestamps):\n{frames}\n\n"
    "Format response as JSON with the following fields: "
    "title, description, objects (array of objects with name, description, confidence, bounding_box, start_time, end_time), "
    "scenes (array of objects with description, objects, actions, setting, start_time, end_time), "
    "topics (array of strings), summary (string).";

constexpr char kAudioAnalysisPrompt[] = 
    "Analyze the following audio data and provide a comprehensive understanding of the content. "
    "Generate a transcript, identify speakers if possible, and create a summary. "
    "\n\nAudio data:\n{audio_data}\n\n"
    "Format response as JSON with the following fields: "
    "title, description, segments (array of objects with speaker, transcript, start_time, end_time, confidence), "
    "summary (string).";

// JavaScript for extracting video frames
constexpr char kExtractVideoFramesScript[] = R"(
  (function(videoSelector, intervalSeconds) {
    return new Promise((resolve, reject) => {
      const video = document.querySelector(videoSelector);
      if (!video) {
        reject('Video element not found');
        return;
      }
      
      const frames = [];
      const canvas = document.createElement('canvas');
      const context = canvas.getContext('2d');
      
      // Set canvas size to match video
      canvas.width = video.videoWidth;
      canvas.height = video.videoHeight;
      
      // Function to capture a frame
      const captureFrame = (time) => {
        video.currentTime = time;
        
        // Wait for the currentTime to actually change
        const checkTime = () => {
          if (Math.abs(video.currentTime - time) < 0.1) {
            // Draw the video frame to the canvas
            context.drawImage(video, 0, 0, canvas.width, canvas.height);
            
            // Convert the canvas to a data URL
            const dataURL = canvas.toDataURL('image/jpeg', 0.8);
            
            frames.push({
              time: time,
              data: dataURL.split(',')[1]  // Remove the data URL prefix
            });
            
            // Check if we've captured all frames
            if (time + intervalSeconds < video.duration) {
              captureFrame(time + intervalSeconds);
            } else {
              resolve(JSON.stringify(frames));
            }
          } else {
            // Check again in a moment
            setTimeout(checkTime, 100);
          }
        };
        
        checkTime();
      };
      
      // Start capturing frames
      video.addEventListener('loadedmetadata', () => {
        captureFrame(0);
      });
      
      // Load the video if it hasn't started loading yet
      if (video.readyState >= 1) {
        captureFrame(0);
      } else {
        video.load();
      }
    });
  })(arguments[0], arguments[1]);
)";

// JavaScript for extracting audio data
constexpr char kExtractAudioDataScript[] = R"(
  (function(audioSelector) {
    return new Promise((resolve, reject) => {
      const audio = document.querySelector(audioSelector);
      if (!audio) {
        reject('Audio element not found');
        return;
      }
      
      // For this example, we'll just return metadata about the audio
      // In a real implementation, we would need to extract the actual audio data
      // which is more complex and requires Web Audio API
      const audioInfo = {
        src: audio.src,
        duration: audio.duration,
        currentTime: audio.currentTime,
        paused: audio.paused,
        ended: audio.ended,
        muted: audio.muted,
        volume: audio.volume
      };
      
      resolve(JSON.stringify(audioInfo));
    });
  })(arguments[0]);
)";

}  // namespace

MultimediaUnderstanding::MultimediaUnderstanding() = default;
MultimediaUnderstanding::~MultimediaUnderstanding() = default;

bool MultimediaUnderstanding::Initialize(asol::core::AIServiceManager* ai_service_manager) {
  // Call the parent class's Initialize method
  if (!ContentUnderstanding::Initialize(ai_service_manager)) {
    return false;
  }
  
  // Additional initialization specific to MultimediaUnderstanding
  // (none needed for now, but could be added in the future)
  
  return true;
}

void MultimediaUnderstanding::AnalyzeVideoElement(
    WebContents* web_contents,
    const std::string& video_selector,
    VideoAnalysisCallback callback) {
  if (!web_contents) {
    VideoAnalysisResult result;
    result.success = false;
    result.error_message = "Web contents is null";
    std::move(callback).Run(result);
    return;
  }
  
  // Extract frames from the video
  ExtractVideoFrames(web_contents, video_selector, 1.0f,  // Extract a frame every second
      base::BindOnce([](
          MultimediaUnderstanding* self,
          VideoAnalysisCallback callback,
          const std::vector<std::string>& frames) {
        // Analyze the extracted frames
        self->AnalyzeVideoFrames(frames, std::move(callback));
      }, this, std::move(callback)));
}

void MultimediaUnderstanding::ExtractVideoFrames(
    WebContents* web_contents,
    const std::string& video_selector,
    float interval_seconds,
    base::OnceCallback<void(const std::vector<std::string>&)> callback) {
  // Execute JavaScript to extract video frames
  web_contents->ExecuteJavaScript(
      GetVideoFrameExtractionScript(),
      base::BindOnce([](
          base::OnceCallback<void(const std::vector<std::string>&)> callback,
          const WebContents::JavaScriptResult& result) {
        std::vector<std::string> frames;
        
        if (!result.success) {
          std::move(callback).Run(frames);
          return;
        }
        
        // Parse the JSON result
        absl::optional<base::Value> json = base::JSONReader::Read(result.result);
        if (!json || !json->is_list()) {
          std::move(callback).Run(frames);
          return;
        }
        
        const base::Value::List& list = json->GetList();
        for (const auto& item : list) {
          if (!item.is_dict()) continue;
          
          const base::Value::Dict& dict = item.GetDict();
          std::string frame_data = dict.FindString("data").value_or("");
          
          if (!frame_data.empty()) {
            frames.push_back(frame_data);
          }
        }
        
        std::move(callback).Run(frames);
      }, std::move(callback)));
}

void MultimediaUnderstanding::AnalyzeVideoFrames(
    const std::vector<std::string>& frame_data,
    VideoAnalysisCallback callback) {
  // If no frames were provided, return an error
  if (frame_data.empty()) {
    VideoAnalysisResult result;
    result.success = false;
    result.error_message = "No video frames provided for analysis";
    std::move(callback).Run(result);
    return;
  }
  
  // Format the frames for the AI prompt
  std::stringstream frames_stream;
  for (size_t i = 0; i < frame_data.size(); ++i) {
    frames_stream << "Frame " << i << " (time: " << (i * 1.0f) << "s): " 
                 << frame_data[i].substr(0, 100) << "...\n";
  }
  std::string frames_str = frames_stream.str();
  
  // Prepare the AI prompt
  std::string prompt = kVideoAnalysisPrompt;
  base::ReplaceSubstringsAfterOffset(&prompt, 0, "{frames}", frames_str);
  
  // Request AI analysis
  ai_service_manager_->GetTextAdapter()->GenerateText(
      prompt,
      base::BindOnce([](
          VideoAnalysisCallback callback,
          const asol::core::TextAdapter::GenerateTextResult& result) {
        if (!result.success) {
          VideoAnalysisResult error_result;
          error_result.success = false;
          error_result.error_message = "Failed to generate AI analysis: " + result.error_message;
          std::move(callback).Run(error_result);
          return;
        }
        
        // Parse the AI response
        VideoAnalysisResult analysis_result;
        analysis_result.success = true;
        
        // Parse JSON response
        absl::optional<base::Value> json = base::JSONReader::Read(result.text);
        if (!json || !json->is_dict()) {
          analysis_result.success = false;
          analysis_result.error_message = "Failed to parse AI response as JSON";
          std::move(callback).Run(analysis_result);
          return;
        }
        
        const base::Value::Dict& dict = json->GetDict();
        
        // Extract basic information
        analysis_result.title = dict.FindString("title").value_or("");
        analysis_result.description = dict.FindString("description").value_or("");
        analysis_result.summary = dict.FindString("summary").value_or("");
        
        // Extract topics
        const base::Value::List* topics_list = dict.FindList("topics");
        if (topics_list) {
          for (const auto& topic : *topics_list) {
            if (topic.is_string()) {
              analysis_result.topics.push_back(topic.GetString());
            }
          }
        }
        
        // Extract objects
        const base::Value::List* objects_list = dict.FindList("objects");
        if (objects_list) {
          for (const auto& obj : *objects_list) {
            if (!obj.is_dict()) continue;
            
            const base::Value::Dict& obj_dict = obj.GetDict();
            
            VideoObject video_obj;
            video_obj.name = obj_dict.FindString("name").value_or("");
            video_obj.description = obj_dict.FindString("description").value_or("");
            video_obj.confidence = obj_dict.FindDouble("confidence").value_or(0.0);
            video_obj.start_time = obj_dict.FindDouble("start_time").value_or(0.0);
            video_obj.end_time = obj_dict.FindDouble("end_time").value_or(0.0);
            
            // Extract bounding box
            const base::Value::List* bbox_list = obj_dict.FindList("bounding_box");
            if (bbox_list && bbox_list->size() == 4) {
              int x = (*bbox_list)[0].GetInt();
              int y = (*bbox_list)[1].GetInt();
              int width = (*bbox_list)[2].GetInt();
              int height = (*bbox_list)[3].GetInt();
              video_obj.bounding_box.emplace_back(x, y);
              video_obj.bounding_box.emplace_back(width, height);
            }
            
            analysis_result.objects.push_back(video_obj);
          }
        }
        
        // Extract scenes
        const base::Value::List* scenes_list = dict.FindList("scenes");
        if (scenes_list) {
          for (const auto& scene : *scenes_list) {
            if (!scene.is_dict()) continue;
            
            const base::Value::Dict& scene_dict = scene.GetDict();
            
            VideoScene video_scene;
            video_scene.description = scene_dict.FindString("description").value_or("");
            video_scene.setting = scene_dict.FindString("setting").value_or("");
            video_scene.start_time = scene_dict.FindDouble("start_time").value_or(0.0);
            video_scene.end_time = scene_dict.FindDouble("end_time").value_or(0.0);
            
            // Extract objects in scene
            const base::Value::List* scene_objects_list = scene_dict.FindList("objects");
            if (scene_objects_list) {
              for (const auto& obj : *scene_objects_list) {
                if (obj.is_string()) {
                  video_scene.objects.push_back(obj.GetString());
                }
              }
            }
            
            // Extract actions in scene
            const base::Value::List* actions_list = scene_dict.FindList("actions");
            if (actions_list) {
              for (const auto& action : *actions_list) {
                if (action.is_string()) {
                  video_scene.actions.push_back(action.GetString());
                }
              }
            }
            
            analysis_result.scenes.push_back(video_scene);
          }
        }
        
        std::move(callback).Run(analysis_result);
      }, std::move(callback)));
}

void MultimediaUnderstanding::AnalyzeAudioElement(
    WebContents* web_contents,
    const std::string& audio_selector,
    AudioAnalysisCallback callback) {
  if (!web_contents) {
    AudioAnalysisResult result;
    result.success = false;
    result.error_message = "Web contents is null";
    std::move(callback).Run(result);
    return;
  }
  
  // Extract audio data
  ExtractAudioData(web_contents, audio_selector,
      base::BindOnce([](
          MultimediaUnderstanding* self,
          AudioAnalysisCallback callback,
          const std::string& audio_data) {
        // Process the audio data
        self->ProcessAudioTranscription(audio_data, std::move(callback));
      }, this, std::move(callback)));
}

void MultimediaUnderstanding::ExtractAudioData(
    WebContents* web_contents,
    const std::string& audio_selector,
    base::OnceCallback<void(const std::string&)> callback) {
  // Execute JavaScript to extract audio data
  web_contents->ExecuteJavaScript(
      GetAudioDataExtractionScript(),
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

void MultimediaUnderstanding::TranscribeAudio(
    WebContents* web_contents,
    const std::string& audio_selector,
    base::OnceCallback<void(const std::string&)> callback) {
  if (!web_contents) {
    std::move(callback).Run("");
    return;
  }
  
  // Extract audio data
  ExtractAudioData(web_contents, audio_selector,
      base::BindOnce([](
          MultimediaUnderstanding* self,
          base::OnceCallback<void(const std::string&)> callback,
          const std::string& audio_data) {
        // In a real implementation, this would use speech recognition AI to transcribe
        // the audio. For now, we'll just return a placeholder.
        std::string transcript = "This is a placeholder transcript. In a real implementation, "
                               "this would be generated by a speech recognition AI model.";
        std::move(callback).Run(transcript);
      }, this, std::move(callback)));
}

void MultimediaUnderstanding::ProcessAudioTranscription(
    const std::string& audio_data,
    AudioAnalysisCallback callback) {
  // If no audio data was provided, return an error
  if (audio_data.empty()) {
    AudioAnalysisResult result;
    result.success = false;
    result.error_message = "No audio data provided for analysis";
    std::move(callback).Run(result);
    return;
  }
  
  // Prepare the AI prompt
  std::string prompt = kAudioAnalysisPrompt;
  base::ReplaceSubstringsAfterOffset(&prompt, 0, "{audio_data}", audio_data);
  
  // Request AI analysis
  ai_service_manager_->GetTextAdapter()->GenerateText(
      prompt,
      base::BindOnce([](
          AudioAnalysisCallback callback,
          const asol::core::TextAdapter::GenerateTextResult& result) {
        if (!result.success) {
          AudioAnalysisResult error_result;
          error_result.success = false;
          error_result.error_message = "Failed to generate AI analysis: " + result.error_message;
          std::move(callback).Run(error_result);
          return;
        }
        
        // Parse the AI response
        AudioAnalysisResult analysis_result;
        analysis_result.success = true;
        
        // Parse JSON response
        absl::optional<base::Value> json = base::JSONReader::Read(result.text);
        if (!json || !json->is_dict()) {
          analysis_result.success = false;
          analysis_result.error_message = "Failed to parse AI response as JSON";
          std::move(callback).Run(analysis_result);
          return;
        }
        
        const base::Value::Dict& dict = json->GetDict();
        
        // Extract basic information
        analysis_result.title = dict.FindString("title").value_or("");
        analysis_result.description = dict.FindString("description").value_or("");
        analysis_result.summary = dict.FindString("summary").value_or("");
        
        // Extract segments
        const base::Value::List* segments_list = dict.FindList("segments");
        if (segments_list) {
          for (const auto& segment : *segments_list) {
            if (!segment.is_dict()) continue;
            
            const base::Value::Dict& segment_dict = segment.GetDict();
            
            AudioSegment audio_segment;
            audio_segment.speaker = segment_dict.FindString("speaker").value_or("");
            audio_segment.transcript = segment_dict.FindString("transcript").value_or("");
            audio_segment.start_time = segment_dict.FindDouble("start_time").value_or(0.0);
            audio_segment.end_time = segment_dict.FindDouble("end_time").value_or(0.0);
            audio_segment.confidence = segment_dict.FindDouble("confidence").value_or(0.0);
            
            analysis_result.segments.push_back(audio_segment);
          }
        }
        
        // Combine all segments into full transcript
        std::stringstream full_transcript;
        for (const auto& segment : analysis_result.segments) {
          full_transcript << "[" << segment.speaker << " " 
                         << segment.start_time << "-" << segment.end_time << "s]: "
                         << segment.transcript << "\n";
        }
        analysis_result.full_transcript = full_transcript.str();
        
        std::move(callback).Run(analysis_result);
      }, std::move(callback)));
}

const char* MultimediaUnderstanding::GetVideoFrameExtractionScript() {
  return kExtractVideoFramesScript;
}

const char* MultimediaUnderstanding::GetAudioDataExtractionScript() {
  return kExtractAudioDataScript;
}

base::WeakPtr<MultimediaUnderstanding> MultimediaUnderstanding::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

}  // namespace ai
}  // namespace browser_core