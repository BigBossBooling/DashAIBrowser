// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iostream>
#include <memory>
#include <string>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/run_loop.h"
#include "base/task/single_thread_task_executor.h"
#include "browser_core/browser_ai_integration.h"
#include "browser_core/features/summarization_feature.h"
#include "asol/core/ai_service_manager.h"
#include "asol/core/privacy_proxy.h"
#include "ui/views/widget/widget.h"

// This example demonstrates how to use the AI-Summarization feature in a browser.
// It initializes the necessary components and simulates loading a web page.

namespace {

// Sample HTML content for testing
const char kSampleHtmlContent[] = R"(
<!DOCTYPE html>
<html>
<head>
  <title>Sample Article for Summarization</title>
  <meta name="author" content="John Doe">
  <meta name="date" content="2025-01-15">
</head>
<body>
  <article>
    <h1>Understanding Artificial Intelligence</h1>
    <p>Artificial Intelligence (AI) is transforming the way we interact with technology. 
    From virtual assistants to autonomous vehicles, AI is becoming increasingly integrated 
    into our daily lives. This article explores the fundamentals of AI, its current 
    applications, and potential future developments.</p>
    
    <h2>What is Artificial Intelligence?</h2>
    <p>Artificial Intelligence refers to the simulation of human intelligence in machines 
    that are programmed to think and learn like humans. The term may also be applied to 
    any machine that exhibits traits associated with a human mind such as learning and 
    problem-solving.</p>
    
    <p>AI can be categorized into two types: narrow or weak AI, which is designed to 
    perform a narrow task (e.g., facial recognition), and general or strong AI, which 
    can perform any intellectual task that a human being can do. Currently, all existing 
    AI systems are narrow AI.</p>
    
    <h2>Machine Learning: The Engine of AI</h2>
    <p>Machine Learning (ML) is a subset of AI that provides systems the ability to 
    automatically learn and improve from experience without being explicitly programmed. 
    ML focuses on the development of computer programs that can access data and use it 
    to learn for themselves.</p>
    
    <p>The learning process begins with observations or data, such as examples, direct 
    experience, or instruction, in order to look for patterns in data and make better 
    decisions in the future based on the examples that we provide. The primary aim is 
    to allow the computers to learn automatically without human intervention or assistance 
    and adjust actions accordingly.</p>
    
    <h2>Deep Learning: A Breakthrough in AI</h2>
    <p>Deep Learning is a subfield of machine learning concerned with algorithms inspired 
    by the structure and function of the brain called artificial neural networks. Deep 
    learning has been instrumental in advancing the capabilities of AI systems.</p>
    
    <p>Neural networks consist of layers of interconnected nodes, each building upon the 
    previous layer to refine and optimize the prediction or categorization. This 
    architecture enables deep learning models to process vast amounts of data and identify 
    complex patterns.</p>
    
    <h2>Current Applications of AI</h2>
    <p>AI is currently being used in numerous applications across various industries:</p>
    
    <p>Healthcare: AI is being used for disease diagnosis, drug discovery, and personalized 
    medicine. For example, AI systems can analyze medical images to detect cancer with 
    accuracy comparable to human radiologists.</p>
    
    <p>Finance: AI algorithms are used for fraud detection, algorithmic trading, and 
    customer service. Banks use AI to identify unusual transactions that may indicate 
    fraudulent activity.</p>
    
    <p>Transportation: Self-driving cars and traffic management systems use AI to navigate 
    roads and optimize traffic flow. Companies like Tesla and Waymo are at the forefront 
    of developing autonomous vehicles.</p>
    
    <h2>The Future of AI</h2>
    <p>The future of AI holds immense potential for further innovation and integration 
    into our society. As AI systems become more sophisticated, they will be able to 
    handle increasingly complex tasks and make more nuanced decisions.</p>
    
    <p>However, the advancement of AI also raises important ethical and societal questions. 
    Issues such as privacy, security, and the impact on employment need to be carefully 
    considered as we continue to develop and deploy AI technologies.</p>
    
    <p>In conclusion, AI represents one of the most significant technological advancements 
    of our time. By understanding its capabilities and limitations, we can harness its 
    potential to solve complex problems and improve our quality of life.</p>
  </article>
</body>
</html>
)";

}  // namespace

int main(int argc, char* argv[]) {
  // Initialize the CommandLine singleton.
  base::CommandLine::Init(argc, argv);

  // This object instance is required by base.
  base::AtExitManager at_exit_manager;

  // A MessageLoop is required for base::TaskRunner.
  base::SingleThreadTaskExecutor main_task_executor;
  
  // Create a run loop to manage the lifetime of the example.
  base::RunLoop run_loop;

  // Initialize AI components
  auto ai_service_manager = std::make_unique<asol::core::AIServiceManager>();
  auto privacy_proxy = std::make_unique<asol::core::PrivacyProxy>();
  
  // Initialize browser AI integration
  auto browser_ai_integration = std::make_unique<browser_core::BrowserAIIntegration>();
  if (!browser_ai_integration->Initialize(ai_service_manager.get(), privacy_proxy.get())) {
    std::cerr << "Failed to initialize browser AI integration" << std::endl;
    return 1;
  }
  
  // Create a sample browser widget
  views::Widget::InitParams params;
  params.bounds = gfx::Rect(0, 0, 800, 600);
  params.type = views::Widget::InitParams::TYPE_WINDOW;
  
  auto browser_widget = std::make_unique<views::Widget>();
  browser_widget->Init(params);
  
  // Create a sample toolbar view
  auto toolbar_view = std::make_unique<views::View>();
  
  // Simulate loading a web page
  std::string page_url = "https://example.com/ai-article";
  std::string html_content = kSampleHtmlContent;
  
  // Notify the browser AI integration of page load
  browser_ai_integration->OnPageLoaded(
      page_url, html_content, toolbar_view.get(), browser_widget.get());
  
  // Get the summarization feature
  browser_core::features::SummarizationFeature* summarization_feature = 
      browser_ai_integration->GetBrowserFeatures()->GetSummarizationFeature();
  
  // Set the feature mode to automatic
  summarization_feature->SetFeatureMode(
      browser_core::features::SummarizationFeature::FeatureMode::AUTOMATIC);
  
  // Set preferred summary format and length
  summarization_feature->SetPreferredSummaryFormat(
      browser_core::ai::SummarizationService::SummaryFormat::BULLET_POINTS);
  summarization_feature->SetPreferredSummaryLength(
      browser_core::ai::SummarizationService::SummaryLength::MEDIUM);
  
  // Show the browser widget
  browser_widget->Show();
  
  // Run the example until the user closes the window
  run_loop.Run();
  
  // Clean up
  browser_ai_integration->OnBrowserClosed();
  
  return 0;
}// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iostream>
#include <memory>
#include <string>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/run_loop.h"
#include "base/task/single_thread_task_executor.h"
#include "browser_core/browser_ai_integration.h"
#include "browser_core/features/summarization_feature.h"
#include "asol/core/ai_service_manager.h"
#include "asol/core/privacy_proxy.h"
#include "ui/views/widget/widget.h"

// This example demonstrates how to use the AI-Summarization feature in a browser.
// It initializes the necessary components and simulates loading a web page.

namespace {

// Sample HTML content for testing
const char kSampleHtmlContent[] = R"(
<!DOCTYPE html>
<html>
<head>
  <title>Sample Article for Summarization</title>
  <meta name="author" content="John Doe">
  <meta name="date" content="2025-01-15">
</head>
<body>
  <article>
    <h1>Understanding Artificial Intelligence</h1>
    <p>Artificial Intelligence (AI) is transforming the way we interact with technology. 
    From virtual assistants to autonomous vehicles, AI is becoming increasingly integrated 
    into our daily lives. This article explores the fundamentals of AI, its current 
    applications, and potential future developments.</p>
    
    <h2>What is Artificial Intelligence?</h2>
    <p>Artificial Intelligence refers to the simulation of human intelligence in machines 
    that are programmed to think and learn like humans. The term may also be applied to 
    any machine that exhibits traits associated with a human mind such as learning and 
    problem-solving.</p>
    
    <p>AI can be categorized into two types: narrow or weak AI, which is designed to 
    perform a narrow task (e.g., facial recognition), and general or strong AI, which 
    can perform any intellectual task that a human being can do. Currently, all existing 
    AI systems are narrow AI.</p>
    
    <h2>Machine Learning: The Engine of AI</h2>
    <p>Machine Learning (ML) is a subset of AI that provides systems the ability to 
    automatically learn and improve from experience without being explicitly programmed. 
    ML focuses on the development of computer programs that can access data and use it 
    to learn for themselves.</p>
    
    <p>The learning process begins with observations or data, such as examples, direct 
    experience, or instruction, in order to look for patterns in data and make better 
    decisions in the future based on the examples that we provide. The primary aim is 
    to allow the computers to learn automatically without human intervention or assistance 
    and adjust actions accordingly.</p>
    
    <h2>Deep Learning: A Breakthrough in AI</h2>
    <p>Deep Learning is a subfield of machine learning concerned with algorithms inspired 
    by the structure and function of the brain called artificial neural networks. Deep 
    learning has been instrumental in advancing the capabilities of AI systems.</p>
    
    <p>Neural networks consist of layers of interconnected nodes, each building upon the 
    previous layer to refine and optimize the prediction or categorization. This 
    architecture enables deep learning models to process vast amounts of data and identify 
    complex patterns.</p>
    
    <h2>Current Applications of AI</h2>
    <p>AI is currently being used in numerous applications across various industries:</p>
    
    <p>Healthcare: AI is being used for disease diagnosis, drug discovery, and personalized 
    medicine. For example, AI systems can analyze medical images to detect cancer with 
    accuracy comparable to human radiologists.</p>
    
    <p>Finance: AI algorithms are used for fraud detection, algorithmic trading, and 
    customer service. Banks use AI to identify unusual transactions that may indicate 
    fraudulent activity.</p>
    
    <p>Transportation: Self-driving cars and traffic management systems use AI to navigate 
    roads and optimize traffic flow. Companies like Tesla and Waymo are at the forefront 
    of developing autonomous vehicles.</p>
    
    <h2>The Future of AI</h2>
    <p>The future of AI holds immense potential for further innovation and integration 
    into our society. As AI systems become more sophisticated, they will be able to 
    handle increasingly complex tasks and make more nuanced decisions.</p>
    
    <p>However, the advancement of AI also raises important ethical and societal questions. 
    Issues such as privacy, security, and the impact on employment need to be carefully 
    considered as we continue to develop and deploy AI technologies.</p>
    
    <p>In conclusion, AI represents one of the most significant technological advancements 
    of our time. By understanding its capabilities and limitations, we can harness its 
    potential to solve complex problems and improve our quality of life.</p>
  </article>
</body>
</html>
)";

}  // namespace

int main(int argc, char* argv[]) {
  // Initialize the CommandLine singleton.
  base::CommandLine::Init(argc, argv);

  // This object instance is required by base.
  base::AtExitManager at_exit_manager;

  // A MessageLoop is required for base::TaskRunner.
  base::SingleThreadTaskExecutor main_task_executor;
  
  // Create a run loop to manage the lifetime of the example.
  base::RunLoop run_loop;

  // Initialize AI components
  auto ai_service_manager = std::make_unique<asol::core::AIServiceManager>();
  auto privacy_proxy = std::make_unique<asol::core::PrivacyProxy>();
  
  // Initialize browser AI integration
  auto browser_ai_integration = std::make_unique<browser_core::BrowserAIIntegration>();
  if (!browser_ai_integration->Initialize(ai_service_manager.get(), privacy_proxy.get())) {
    std::cerr << "Failed to initialize browser AI integration" << std::endl;
    return 1;
  }
  
  // Create a sample browser widget
  views::Widget::InitParams params;
  params.bounds = gfx::Rect(0, 0, 800, 600);
  params.type = views::Widget::InitParams::TYPE_WINDOW;
  
  auto browser_widget = std::make_unique<views::Widget>();
  browser_widget->Init(params);
  
  // Create a sample toolbar view
  auto toolbar_view = std::make_unique<views::View>();
  
  // Simulate loading a web page
  std::string page_url = "https://example.com/ai-article";
  std::string html_content = kSampleHtmlContent;
  
  // Notify the browser AI integration of page load
  browser_ai_integration->OnPageLoaded(
      page_url, html_content, toolbar_view.get(), browser_widget.get());
  
  // Get the summarization feature
  browser_core::features::SummarizationFeature* summarization_feature = 
      browser_ai_integration->GetBrowserFeatures()->GetSummarizationFeature();
  
  // Set the feature mode to automatic
  summarization_feature->SetFeatureMode(
      browser_core::features::SummarizationFeature::FeatureMode::AUTOMATIC);
  
  // Set preferred summary format and length
  summarization_feature->SetPreferredSummaryFormat(
      browser_core::ai::SummarizationService::SummaryFormat::BULLET_POINTS);
  summarization_feature->SetPreferredSummaryLength(
      browser_core::ai::SummarizationService::SummaryLength::MEDIUM);
  
  // Show the browser widget
  browser_widget->Show();
  
  // Run the example until the user closes the window
  run_loop.Run();
  
  // Clean up
  browser_ai_integration->OnBrowserClosed();
  
  return 0;
}