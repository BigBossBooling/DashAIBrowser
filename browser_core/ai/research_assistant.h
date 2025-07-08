// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_CORE_AI_RESEARCH_ASSISTANT_H_
#define BROWSER_CORE_AI_RESEARCH_ASSISTANT_H_

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "asol/core/ai_service_manager.h"
#include "browser_core/engine/browser_engine.h"
#include "browser_core/ai/content_understanding.h"

namespace browser_core {
namespace ai {

// ResearchAssistant provides advanced research capabilities.
class ResearchAssistant {
 public:
  // Research project
  struct ResearchProject {
    std::string id;
    std::string name;
    std::string description;
    base::Time created_time;
    base::Time last_modified_time;
  };

  // Research source
  struct ResearchSource {
    std::string url;
    std::string title;
    std::string summary;
    std::vector<std::string> key_points;
    std::string author;
    std::string published_date;
    float relevance_score;
    bool is_verified;
    base::Time added_time;
  };

  // Research note
  struct ResearchNote {
    std::string id;
    std::string project_id;
    std::string title;
    std::string content;
    std::vector<std::string> tags;
    std::vector<std::string> related_source_urls;
    base::Time created_time;
    base::Time last_modified_time;
  };

  // Research synthesis
  struct ResearchSynthesis {
    std::string title;
    std::string summary;
    std::vector<std::string> key_findings;
    std::vector<std::string> supporting_evidence;
    std::vector<std::string> counterpoints;
    std::vector<std::string> open_questions;
    std::vector<std::string> source_urls;
  };

  // Citation
  struct Citation {
    enum class Style {
      APA,
      MLA,
      CHICAGO,
      HARVARD,
      IEEE,
      CUSTOM
    };

    std::string url;
    std::string formatted_citation;
    Style style;
  };

  // Research result
  struct ResearchResult {
    bool success;
    std::string error_message;
  };

  // Callbacks
  using ResearchResultCallback = base::OnceCallback<void(const ResearchResult&)>;
  using ProjectsCallback = base::OnceCallback<void(const std::vector<ResearchProject>&)>;
  using SourcesCallback = base::OnceCallback<void(const std::vector<ResearchSource>&)>;
  using NotesCallback = base::OnceCallback<void(const std::vector<ResearchNote>&)>;
  using SynthesisCallback = base::OnceCallback<void(const ResearchSynthesis&)>;
  using CitationsCallback = base::OnceCallback<void(const std::vector<Citation>&)>;

  ResearchAssistant();
  ~ResearchAssistant();

  // Disallow copy and assign
  ResearchAssistant(const ResearchAssistant&) = delete;
  ResearchAssistant& operator=(const ResearchAssistant&) = delete;

  // Initialize with browser engine, AI service manager, and content understanding
  bool Initialize(BrowserEngine* browser_engine, 
                asol::core::AIServiceManager* ai_service_manager,
                ContentUnderstanding* content_understanding);

  // Project management
  void CreateProject(const std::string& name, 
                   const std::string& description,
                   base::OnceCallback<void(const ResearchProject&)> callback);
  
  void GetProjects(ProjectsCallback callback);
  
  void GetProject(const std::string& project_id,
                base::OnceCallback<void(const ResearchProject&)> callback);
  
  void UpdateProject(const ResearchProject& project,
                   ResearchResultCallback callback);
  
  void DeleteProject(const std::string& project_id,
                   ResearchResultCallback callback);

  // Source management
  void AddSource(const std::string& project_id,
               const std::string& url,
               SourcesCallback callback);
  
  void GetSources(const std::string& project_id,
                SourcesCallback callback);
  
  void RemoveSource(const std::string& project_id,
                  const std::string& url,
                  ResearchResultCallback callback);

  // Note management
  void CreateNote(const std::string& project_id,
                const std::string& title,
                const std::string& content,
                const std::vector<std::string>& tags,
                base::OnceCallback<void(const ResearchNote&)> callback);
  
  void GetNotes(const std::string& project_id,
              NotesCallback callback);
  
  void UpdateNote(const ResearchNote& note,
                ResearchResultCallback callback);
  
  void DeleteNote(const std::string& note_id,
                ResearchResultCallback callback);

  // Research synthesis
  void GenerateSynthesis(const std::string& project_id,
                       SynthesisCallback callback);

  // Citations
  void GenerateCitations(const std::string& project_id,
                       Citation::Style style,
                       CitationsCallback callback);

  // Research assistance
  void SuggestRelatedSources(const std::string& project_id,
                           SourcesCallback callback);
  
  void AnswerResearchQuestion(const std::string& project_id,
                            const std::string& question,
                            base::OnceCallback<void(const std::string&)> callback);
  
  void GenerateOutline(const std::string& project_id,
                     base::OnceCallback<void(const std::vector<std::string>&)> callback);

  // Add current page to project
  void AddCurrentPageToProject(int tab_id,
                             const std::string& project_id,
                             SourcesCallback callback);

  // Enable/disable research assistant
  void Enable(bool enable);
  bool IsEnabled() const;

  // Get a weak pointer to this instance
  base::WeakPtr<ResearchAssistant> GetWeakPtr();

 private:
  // Helper methods
  void AnalyzeSourceForRelevance(const std::string& url,
                               const std::string& content,
                               const std::string& project_id,
                               base::OnceCallback<void(float score)> callback);
  
  void ExtractKeyPointsFromSource(const std::string& content,
                                base::OnceCallback<void(const std::vector<std::string>&)> callback);
  
  void FormatCitation(const std::string& url,
                    const std::string& title,
                    const std::string& author,
                    const std::string& published_date,
                    Citation::Style style,
                    base::OnceCallback<void(const std::string&)> callback);

  // Browser engine, AI service manager, and content understanding
  BrowserEngine* browser_engine_ = nullptr;
  asol::core::AIServiceManager* ai_service_manager_ = nullptr;
  ContentUnderstanding* content_understanding_ = nullptr;

  // State
  bool is_enabled_ = true;

  // Private implementation
  class Impl;
  std::unique_ptr<Impl> impl_;

  // For weak pointers
  base::WeakPtrFactory<ResearchAssistant> weak_ptr_factory_{this};
};

}  // namespace ai
}  // namespace browser_core

#endif  // BROWSER_CORE_AI_RESEARCH_ASSISTANT_H_