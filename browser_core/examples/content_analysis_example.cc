// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iostream>
#include <memory>
#include <string>

#include "asol/adapters/adapter_factory.h"
#include "asol/core/ai_service_provider.h"
#include "asol/core/multi_adapter_manager.h"
#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/run_loop.h"
#include "base/task/single_thread_task_executor.h"
#include "browser_core/content/content_extractor.h"

namespace {

// Sample HTML content for testing
const char kSampleHtmlContent[] = R"(
<!DOCTYPE html>
<html>
<head>
  <title>Understanding Web Security Best Practices</title>
  <meta name="author" content="Jane Smith">
  <meta name="date" content="2025-03-22">
</head>
<body>
  <article>
    <h1>Understanding Web Security Best Practices</h1>
    <p>Web security is a critical aspect of modern application development. 
    With cyber threats becoming increasingly sophisticated, implementing robust 
    security measures is essential for protecting user data and maintaining trust. 
    This article explores key web security best practices that developers should 
    implement in their applications.</p>
    
    <h2>1. Implement HTTPS Everywhere</h2>
    <p>HTTPS (HTTP Secure) encrypts the data transmitted between a user's browser 
    and your web server. This encryption prevents attackers from intercepting and 
    tampering with the data in transit, a type of attack known as a man-in-the-middle 
    attack. All websites should use HTTPS, not just those handling sensitive information.</p>
    
    <p>Key steps for implementing HTTPS include:</p>
    <ul>
      <li>Obtain an SSL/TLS certificate from a trusted certificate authority</li>
      <li>Configure your web server to use HTTPS</li>
      <li>Implement HTTP Strict Transport Security (HSTS)</li>
      <li>Redirect all HTTP traffic to HTTPS</li>
    </ul>
    
    <h2>2. Practice Proper Authentication and Authorization</h2>
    <p>Authentication verifies a user's identity, while authorization determines what 
    actions they can perform. Implementing these correctly is fundamental to web security.</p>
    
    <p>Best practices include:</p>
    <ul>
      <li>Use strong password policies</li>
      <li>Implement multi-factor authentication (MFA)</li>
      <li>Use secure session management</li>
      <li>Apply the principle of least privilege</li>
      <li>Consider using OAuth 2.0 or OpenID Connect for authentication</li>
    </ul>
    
    <h2>3. Prevent Common Web Vulnerabilities</h2>
    <p>Several common vulnerabilities can compromise web applications. Understanding 
    and preventing these is essential:</p>
    
    <h3>Cross-Site Scripting (XSS)</h3>
    <p>XSS attacks occur when malicious scripts are injected into trusted websites. 
    Prevent XSS by validating and sanitizing user input, implementing Content Security 
    Policy (CSP), and using proper output encoding.</p>
    
    <h3>SQL Injection</h3>
    <p>SQL injection attacks insert malicious SQL code into database queries. Prevent 
    these by using parameterized queries or prepared statements, and implementing 
    proper input validation.</p>
    
    <h3>Cross-Site Request Forgery (CSRF)</h3>
    <p>CSRF tricks users into performing unwanted actions on a site they're authenticated 
    to. Prevent CSRF by using anti-CSRF tokens, implementing SameSite cookies, and 
    verifying request origins.</p>
    
    <h2>4. Keep Dependencies Updated</h2>
    <p>Outdated libraries and frameworks often contain known vulnerabilities. Regularly 
    update all dependencies and use tools like dependency scanners to identify and 
    address security issues in third-party components.</p>
    
    <h2>5. Implement Proper Error Handling</h2>
    <p>Improper error handling can leak sensitive information to attackers. Implement 
    custom error pages, log errors securely, and avoid exposing stack traces or 
    detailed error messages to users.</p>
    
    <h2>Conclusion</h2>
    <p>Web security is not a one-time implementation but an ongoing process. By 
    following these best practices and staying informed about emerging threats, 
    developers can significantly enhance the security of their web applications 
    and protect their users' data.</p>
    
    <p>Remember that security is only as strong as the weakest link, so a comprehensive 
    approach addressing all potential vulnerabilities is essential.</p>
  </article>
</body>
</html>
)";

}  // namespace

// This example demonstrates how to use the ContentExtractor and AI services
// to analyze web content and provide insights.
int main(int argc, char* argv[]) {
  // Initialize base
  base::AtExitManager exit_manager;
  base::CommandLine::Init(argc, argv);
  base::SingleThreadTaskExecutor task_executor;
  
  // Create configuration for AI services
  std::unordered_map<std::string, std::string> config;
  
  // Add API keys for each provider
  // In a real application, these would be loaded from secure storage
  config["gemini_api_key"] = "GEMINI_API_KEY";
  config["openai_api_key"] = "OPENAI_API_KEY";
  config["copilot_api_key"] = "COPILOT_API_KEY";
  config["claude_api_key"] = "CLAUDE_API_KEY";
  
  // Set default provider
  config["default_provider"] = "gemini";
  
  // Create the multi-adapter manager
  auto adapter_manager = asol::adapters::AdapterFactory::CreateMultiAdapterManager(config);
  
  // Create the content extractor
  auto content_extractor = std::make_unique<browser_core::content::ContentExtractor>();
  if (!content_extractor->Initialize()) {
    std::cerr << "Failed to initialize content extractor" << std::endl;
    return 1;
  }
  
  // Sample page URL
  std::string page_url = "https://example.com/web-security-article";
  
  // Extract content from the sample HTML
  std::cout << "Extracting content from the web page..." << std::endl;
  auto extracted_content = content_extractor->ExtractContentSync(page_url, kSampleHtmlContent);
  
  if (!extracted_content.success) {
    std::cerr << "Content extraction failed: " << extracted_content.error_message << std::endl;
    return 1;
  }
  
  // Display extracted content information
  std::cout << "\nExtracted Content Information:" << std::endl;
  std::cout << "Title: " << extracted_content.title << std::endl;
  std::cout << "Author: " << extracted_content.author << std::endl;
  std::cout << "Date: " << extracted_content.date << std::endl;
  std::cout << "Content Type: ";
  
  switch (extracted_content.content_type) {
    case browser_core::content::ContentExtractor::ContentType::ARTICLE:
      std::cout << "Article";
      break;
    case browser_core::content::ContentExtractor::ContentType::PRODUCT:
      std::cout << "Product";
      break;
    case browser_core::content::ContentExtractor::ContentType::DOCUMENTATION:
      std::cout << "Documentation";
      break;
    case browser_core::content::ContentExtractor::ContentType::FORUM:
      std::cout << "Forum";
      break;
    case browser_core::content::ContentExtractor::ContentType::SOCIAL:
      std::cout << "Social";
      break;
    case browser_core::content::ContentExtractor::ContentType::MIXED:
      std::cout << "Mixed";
      break;
    default:
      std::cout << "Unknown";
  }
  std::cout << std::endl;
  
  std::cout << "Number of Paragraphs: " << extracted_content.paragraphs.size() << std::endl;
  std::cout << "Number of Headings: " << extracted_content.headings.size() << std::endl;
  std::cout << "Number of Images: " << extracted_content.images.size() << std::endl;
  std::cout << "Number of Links: " << extracted_content.links.size() << std::endl;
  
  // Create a run loop for async operations
  base::RunLoop run_loop;
  
  // Analyze the content using AI
  std::cout << "\nAnalyzing content with AI..." << std::endl;
  
  // Prepare the prompt for content analysis
  std::string analysis_prompt = "Analyze the following web article and provide insights:\n\n";
  analysis_prompt += "Title: " + extracted_content.title + "\n\n";
  analysis_prompt += "Content:\n" + extracted_content.main_text.substr(0, 1500) + "...\n\n";
  analysis_prompt += "Please provide:\n";
  analysis_prompt += "1. A brief summary of the main topics\n";
  analysis_prompt += "2. The target audience for this content\n";
  analysis_prompt += "3. Key takeaways\n";
  analysis_prompt += "4. Suggestions for related topics";
  
  // Set up the AI request
  asol::core::AIRequestParams params;
  params.task_type = asol::core::AIServiceProvider::TaskType::TEXT_GENERATION;
  params.input_text = analysis_prompt;
  
  // Process the request with the active provider
  adapter_manager->ProcessRequest(
      params,
      base::BindOnce([](base::OnceClosure quit_closure, bool success, const std::string& response) {
        if (success) {
          std::cout << "\nAI Analysis Results:" << std::endl;
          std::cout << response << std::endl;
        } else {
          std::cout << "Error during AI analysis: " << response << std::endl;
        }
        std::move(quit_closure).Run();
      }, run_loop.QuitClosure()));
  
  // Run the loop to wait for the response
  run_loop.Run();
  
  // Create a new run loop for the next request
  base::RunLoop run_loop2;
  
  // Perform a sentiment analysis on the content
  std::cout << "\nPerforming sentiment analysis..." << std::endl;
  
  std::string sentiment_prompt = "Analyze the sentiment and tone of the following article excerpt:\n\n";
  sentiment_prompt += extracted_content.main_text.substr(0, 1000) + "...\n\n";
  sentiment_prompt += "Provide a brief sentiment analysis including:\n";
  sentiment_prompt += "1. Overall tone (formal, informal, technical, conversational, etc.)\n";
  sentiment_prompt += "2. Sentiment (positive, negative, neutral)\n";
  sentiment_prompt += "3. Writing style and level of expertise demonstrated";
  
  // Set up the AI request for sentiment analysis
  params.input_text = sentiment_prompt;
  
  // Process the request with a different provider for comparison
  adapter_manager->ProcessRequestWithProvider(
      "openai",  // Use OpenAI for this analysis
      params,
      base::BindOnce([](base::OnceClosure quit_closure, bool success, const std::string& response) {
        if (success) {
          std::cout << "\nSentiment Analysis Results (OpenAI):" << std::endl;
          std::cout << response << std::endl;
        } else {
          std::cout << "Error during sentiment analysis: " << response << std::endl;
        }
        std::move(quit_closure).Run();
      }, run_loop2.QuitClosure()));
  
  // Run the loop to wait for the response
  run_loop2.Run();
  
  std::cout << "\nContent analysis complete!" << std::endl;
  
  return 0;
}