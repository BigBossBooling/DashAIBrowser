# ASOL Architecture

## Overview

ASOL (AI Service Orchestration Layer) is a modular system designed to integrate AI capabilities into the DashAIBrowser. It provides a unified interface for interacting with various AI models and services, abstracting away the complexities of different APIs and implementations.

## Core Components

### 1. Adapter Interface

The adapter interface (`asol/adapters/adapter_interface.h`) defines a common API for all AI model adapters. This allows the system to interact with different AI models in a consistent way, regardless of the underlying implementation details.

Key features:
- Common response structure (`ModelResponse`)
- Synchronous, asynchronous, and streaming processing methods
- Capability-based model selection
- Standardized initialization and configuration
- HTTP client for API communication

### 2. Model Adapters

Model adapters implement the adapter interface for specific AI models. Each adapter handles the details of communicating with a particular AI service, formatting requests and responses, and managing authentication.

Current adapters:
- **Gemini Adapter** (`asol/adapters/gemini/`): Integrates with Google's Gemini API for text processing

### 3. Service Manager

The service manager (`asol/core/service_manager.h`) is the central component that manages all adapters and routes requests to the appropriate adapter based on capabilities or explicit selection.

Key features:
- Adapter registration and management
- Capability-based routing
- Centralized configuration and configuration loading
- Streaming support with partial responses
- Singleton pattern for global access

## Communication Flow

1. Client code requests an AI service through the service manager
2. Service manager selects the appropriate adapter based on the request
3. Adapter formats the request for the specific AI model
4. Adapter sends the request to the AI service
5. Adapter receives and parses the response
6. Response is returned to the client through the service manager

## Extension Points

The system is designed to be easily extensible:

1. **New AI Models**: Add new adapters by implementing the adapter interface
2. **New Capabilities**: Extend existing adapters with new capabilities
3. **Enhanced Routing**: Improve the service manager's adapter selection logic

## Usage Example

```cpp
// Get the service manager
auto service_manager = asol::core::ServiceManager::GetInstance();

// Register and initialize adapters
service_manager->RegisterAdapter("gemini", asol::adapters::CreateAdapter("gemini"));

// Load configuration from file
std::string config_json = asol::core::ConfigLoader::LoadDefault();
service_manager->InitializeAdapters(config_json);

// Process text with a specific adapter (synchronous)
auto response = service_manager->ProcessText("gemini", "What is the capital of France?");

// Process text with any adapter that supports a specific capability
auto response = service_manager->ProcessTextWithCapability("summarization", long_text);

// Process text asynchronously
service_manager->ProcessTextAsync("gemini", "Tell me a joke", 
    [](const asol::adapters::ModelResponse& response) {
      if (response.success) {
        std::cout << "Response: " << response.text << std::endl;
      }
    });

// Process text with streaming response
service_manager->ProcessTextStream("gemini", "Write a story", 
    [](const asol::adapters::ModelResponse& response, bool is_done) {
      if (response.success) {
        std::cout << response.text;
        std::cout.flush();
      }
      if (is_done) {
        std::cout << std::endl << "Streaming complete." << std::endl;
      }
    });
```

## Future Directions

1. **Additional Adapters**: Support for more AI models and services
2. **Enhanced Capabilities**: More specialized AI capabilities
3. **Performance Optimization**: Caching, batching, and other optimizations
4. **Security Enhancements**: Better handling of API keys and sensitive data