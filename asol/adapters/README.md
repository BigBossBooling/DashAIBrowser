# DashAIBrowser AI Adapters

This directory contains adapters for various AI service providers that can be used with DashAIBrowser.

## Available Adapters

- **Gemini**: Google's Gemini AI models
- **OpenAI**: OpenAI's GPT models
- **Microsoft Copilot**: Microsoft's Copilot AI
- **Claude**: Anthropic's Claude AI models

## Using the Adapters

The adapters can be used individually or through the `MultiAdapterManager` which allows switching between different providers.

### Example Usage

```cpp
#include "asol/adapters/adapter_factory.h"
#include "asol/core/multi_adapter_manager.h"

// Create configuration
std::unordered_map<std::string, std::string> config;
config["gemini_api_key"] = "YOUR_GEMINI_API_KEY";
config["openai_api_key"] = "YOUR_OPENAI_API_KEY";
config["copilot_api_key"] = "YOUR_COPILOT_API_KEY";
config["claude_api_key"] = "YOUR_CLAUDE_API_KEY";
config["default_provider"] = "gemini";  // Set default provider

// Create the multi-adapter manager
auto adapter_manager = asol::adapters::AdapterFactory::CreateMultiAdapterManager(config);

// Create request parameters
asol::core::AIRequestParams params;
params.task_type = asol::core::AIServiceProvider::TaskType::TEXT_GENERATION;
params.input_text = "Your prompt here";

// Process the request with the active provider
adapter_manager->ProcessRequest(
    params,
    base::BindOnce([](bool success, const std::string& response) {
      if (success) {
        // Handle successful response
        std::cout << response << std::endl;
      } else {
        // Handle error
        std::cout << "Error: " << response << std::endl;
      }
    }));

// Switch to a different provider
adapter_manager->SetActiveProvider("openai");

// Or use a specific provider without changing the active one
adapter_manager->ProcessRequestWithProvider(
    "claude",
    params,
    base::BindOnce([](bool success, const std::string& response) {
      // Handle response
    }));
```

## Configuration Options

Each adapter supports the following configuration options:

### Gemini

- `gemini_api_key`: API key for Google Gemini
- `gemini_model`: Model name (default: "gemini-pro")
- `gemini_temperature`: Temperature parameter (default: 0.7)
- `gemini_max_output_tokens`: Maximum output tokens (default: 1024)

### OpenAI

- `openai_api_key`: API key for OpenAI
- `openai_model`: Model name (default: "gpt-4o")
- `openai_temperature`: Temperature parameter (default: 0.7)
- `openai_max_tokens`: Maximum tokens (default: 1024)
- `openai_organization_id`: Organization ID (optional)

### Microsoft Copilot

- `copilot_api_key`: API key for Microsoft Copilot
- `copilot_endpoint`: API endpoint (default: "https://api.cognitive.microsoft.com/copilot/v1/chat/completions")
- `copilot_model`: Model name (default: "copilot-4")
- `copilot_temperature`: Temperature parameter (default: 0.7)
- `copilot_max_tokens`: Maximum tokens (default: 1024)
- `copilot_api_version`: API version (default: "2023-12-01-preview")

### Claude

- `claude_api_key`: API key for Anthropic Claude
- `claude_model`: Model name (default: "claude-3-opus-20240229")
- `claude_temperature`: Temperature parameter (default: 0.7)
- `claude_max_tokens`: Maximum tokens (default: 1024)
- `claude_anthropic_version`: API version (default: "2023-06-01")

## Supported Features

All adapters support the following features:

- Text generation
- Text summarization
- Content analysis
- Question answering
- Code generation
- Translation
- Conversation context management

## Adding a New Adapter

To add a new adapter:

1. Create a new directory under `asol/adapters/`
2. Implement the adapter and service provider classes
3. Add the adapter to the `AdapterFactory`
4. Update the BUILD.gn files to include the new adapter

See the existing adapters for examples of the implementation pattern.