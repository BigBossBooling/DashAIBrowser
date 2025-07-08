// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <chrono>
#include <thread>

#include "asol/adapters/adapter_factory.h"
#include "asol/core/ai_service_provider.h"
#include "asol/core/multi_adapter_manager.h"
#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/run_loop.h"
#include "base/task/single_thread_task_executor.h"

// Example of using the MultiAdapterManager's caching functionality
int main(int argc, char* argv[]) {
  // Initialize base
  base::AtExitManager exit_manager;
  base::CommandLine::Init(argc, argv);
  base::SingleThreadTaskExecutor task_executor;
  
  // Create configuration
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
  
  // Configure the cache
  asol::core::MultiAdapterManager::CacheConfig cache_config;
  cache_config.enabled = true;
  cache_config.max_entries = 50;
  cache_config.max_age_seconds = 3600;  // 1 hour
  adapter_manager->ConfigureCache(cache_config);
  
  // Print available providers
  std::cout << "Available AI providers:" << std::endl;
  for (const auto& provider_id : adapter_manager->GetRegisteredProviderIds()) {
    auto provider = adapter_manager->GetProvider(provider_id);
    std::cout << "- " << provider->GetProviderName() 
              << " (ID: " << provider_id << ")" << std::endl;
  }
  
  // Print active provider
  std::cout << "Active provider: " << adapter_manager->GetActiveProviderId() << std::endl;
  
  // Example request parameters
  asol::core::AIRequestParams params;
  params.task_type = asol::core::AIServiceProvider::TaskType::TEXT_GENERATION;
  params.input_text = "What are the benefits of caching AI responses?";
  
  // First request - should be a cache miss
  std::cout << "\nSending first request (should be a cache miss)..." << std::endl;
  {
    base::RunLoop run_loop;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    adapter_manager->ProcessRequest(
        params,
        base::BindOnce([](base::OnceClosure quit_closure, 
                         std::chrono::high_resolution_clock::time_point start_time,
                         bool success, 
                         const std::string& response) {
          auto end_time = std::chrono::high_resolution_clock::now();
          auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
              end_time - start_time).count();
          
          if (success) {
            std::cout << "Response received in " << duration << "ms:" << std::endl;
            std::cout << response << std::endl;
          } else {
            std::cout << "Error: " << response << std::endl;
          }
          std::move(quit_closure).Run();
        }, run_loop.QuitClosure(), start_time));
    
    run_loop.Run();
  }
  
  // Print cache statistics
  auto cache_stats = adapter_manager->GetCacheStats();
  std::cout << "\nCache statistics after first request:" << std::endl;
  std::cout << "Total entries: " << cache_stats.total_entries << std::endl;
  std::cout << "Hits: " << cache_stats.hits << std::endl;
  std::cout << "Misses: " << cache_stats.misses << std::endl;
  std::cout << "Hit rate: " << (cache_stats.hit_rate * 100) << "%" << std::endl;
  
  // Second request with the same parameters - should be a cache hit
  std::cout << "\nSending second request with same parameters (should be a cache hit)..." << std::endl;
  {
    base::RunLoop run_loop;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    adapter_manager->ProcessRequest(
        params,
        base::BindOnce([](base::OnceClosure quit_closure, 
                         std::chrono::high_resolution_clock::time_point start_time,
                         bool success, 
                         const std::string& response) {
          auto end_time = std::chrono::high_resolution_clock::now();
          auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
              end_time - start_time).count();
          
          if (success) {
            std::cout << "Response received in " << duration << "ms:" << std::endl;
            std::cout << response << std::endl;
          } else {
            std::cout << "Error: " << response << std::endl;
          }
          std::move(quit_closure).Run();
        }, run_loop.QuitClosure(), start_time));
    
    run_loop.Run();
  }
  
  // Print updated cache statistics
  cache_stats = adapter_manager->GetCacheStats();
  std::cout << "\nCache statistics after second request:" << std::endl;
  std::cout << "Total entries: " << cache_stats.total_entries << std::endl;
  std::cout << "Hits: " << cache_stats.hits << std::endl;
  std::cout << "Misses: " << cache_stats.misses << std::endl;
  std::cout << "Hit rate: " << (cache_stats.hit_rate * 100) << "%" << std::endl;
  
  // Third request with different parameters - should be a cache miss
  std::cout << "\nSending third request with different parameters (should be a cache miss)..." << std::endl;
  params.input_text = "What are the drawbacks of caching AI responses?";
  
  {
    base::RunLoop run_loop;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    adapter_manager->ProcessRequest(
        params,
        base::BindOnce([](base::OnceClosure quit_closure, 
                         std::chrono::high_resolution_clock::time_point start_time,
                         bool success, 
                         const std::string& response) {
          auto end_time = std::chrono::high_resolution_clock::now();
          auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
              end_time - start_time).count();
          
          if (success) {
            std::cout << "Response received in " << duration << "ms:" << std::endl;
            std::cout << response << std::endl;
          } else {
            std::cout << "Error: " << response << std::endl;
          }
          std::move(quit_closure).Run();
        }, run_loop.QuitClosure(), start_time));
    
    run_loop.Run();
  }
  
  // Print updated cache statistics
  cache_stats = adapter_manager->GetCacheStats();
  std::cout << "\nCache statistics after third request:" << std::endl;
  std::cout << "Total entries: " << cache_stats.total_entries << std::endl;
  std::cout << "Hits: " << cache_stats.hits << std::endl;
  std::cout << "Misses: " << cache_stats.misses << std::endl;
  std::cout << "Hit rate: " << (cache_stats.hit_rate * 100) << "%" << std::endl;
  
  // Clear the cache
  std::cout << "\nClearing the cache..." << std::endl;
  adapter_manager->ClearCache();
  
  // Fourth request with the same parameters as the second - should be a cache miss after clearing
  std::cout << "\nSending fourth request after clearing cache (should be a cache miss)..." << std::endl;
  params.input_text = "What are the benefits of caching AI responses?";
  
  {
    base::RunLoop run_loop;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    adapter_manager->ProcessRequest(
        params,
        base::BindOnce([](base::OnceClosure quit_closure, 
                         std::chrono::high_resolution_clock::time_point start_time,
                         bool success, 
                         const std::string& response) {
          auto end_time = std::chrono::high_resolution_clock::now();
          auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
              end_time - start_time).count();
          
          if (success) {
            std::cout << "Response received in " << duration << "ms:" << std::endl;
            std::cout << response << std::endl;
          } else {
            std::cout << "Error: " << response << std::endl;
          }
          std::move(quit_closure).Run();
        }, run_loop.QuitClosure(), start_time));
    
    run_loop.Run();
  }
  
  // Print final cache statistics
  cache_stats = adapter_manager->GetCacheStats();
  std::cout << "\nFinal cache statistics:" << std::endl;
  std::cout << "Total entries: " << cache_stats.total_entries << std::endl;
  std::cout << "Hits: " << cache_stats.hits << std::endl;
  std::cout << "Misses: " << cache_stats.misses << std::endl;
  std::cout << "Hit rate: " << (cache_stats.hit_rate * 100) << "%" << std::endl;
  
  // Disable caching
  std::cout << "\nDisabling cache..." << std::endl;
  cache_config.enabled = false;
  adapter_manager->ConfigureCache(cache_config);
  
  // Fifth request - should bypass cache
  std::cout << "\nSending fifth request with cache disabled..." << std::endl;
  
  {
    base::RunLoop run_loop;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    adapter_manager->ProcessRequest(
        params,
        base::BindOnce([](base::OnceClosure quit_closure, 
                         std::chrono::high_resolution_clock::time_point start_time,
                         bool success, 
                         const std::string& response) {
          auto end_time = std::chrono::high_resolution_clock::now();
          auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
              end_time - start_time).count();
          
          if (success) {
            std::cout << "Response received in " << duration << "ms:" << std::endl;
            std::cout << response << std::endl;
          } else {
            std::cout << "Error: " << response << std::endl;
          }
          std::move(quit_closure).Run();
        }, run_loop.QuitClosure(), start_time));
    
    run_loop.Run();
  }
  
  return 0;
}