# DashAIBrowser Multi-Adapter Feature

This feature adds support for multiple AI service providers in DashAIBrowser, allowing users to choose between different AI models and services.

## Supported AI Providers

- **Google Gemini**: Google's Gemini AI models
- **OpenAI**: OpenAI's GPT models
- **Microsoft Copilot**: Microsoft's Copilot AI
- **Anthropic Claude**: Anthropic's Claude AI models

## Architecture

The multi-adapter feature consists of the following components:

### Core Components

- **MultiAdapterManager**: Manages multiple AI service providers and allows switching between them
- **AdapterFactory**: Creates and initializes AI service providers
- **AIServiceProvider**: Interface for AI service providers
- **AITextAdapter**: Interface for text-based AI adapters

### Provider-Specific Adapters

- **GeminiServiceProvider** and **GeminiTextAdapter**: Adapter for Google Gemini
- **OpenAIServiceProvider** and **OpenAITextAdapter**: Adapter for OpenAI
- **CopilotServiceProvider** and **CopilotTextAdapter**: Adapter for Microsoft Copilot
- **ClaudeServiceProvider** and **ClaudeTextAdapter**: Adapter for Anthropic Claude

### UI Components

- **AIProviderSelector**: UI for selecting and configuring AI providers
- **AISettingsPage**: Settings page for configuring AI providers
- **AIProviderMenuButton**: Toolbar button for selecting AI providers

## Usage

### Using the MultiAdapterManager

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
```

### Using the AI Settings Page

```cpp
#include "browser_core/ui/ai_settings_page.h"

// Create the AI settings page
auto settings_page = std::make_unique<browser_core::ui::AISettingsPage>(adapter_manager.get());

// Initialize and show the settings page
settings_page->Initialize();
settings_page->Show();

// Configure a provider
settings_page->SetConfigValue("temperature", "0.5");
settings_page->ApplySettings();
```

### Using the AI Provider Menu Button

```cpp
#include "browser_core/ui/ai_provider_menu_button.h"

// Create the AI provider menu button
auto menu_button = std::make_unique<browser_core::ui::AIProviderMenuButton>(ai_integration.get());

// Add the button to your toolbar
toolbar->AddChildView(std::move(menu_button));
```

## Examples

The following examples demonstrate how to use the multi-adapter feature:

- **asol/examples/multi_adapter_example.cc**: Example of using the MultiAdapterManager
- **browser_core/examples/ai_settings_example.cc**: Example of using the AI Settings Page
- **browser_core/examples/ai_menu_button_example.cc**: Example of using the AI Provider Menu Button

## Building and Running

To build and run the examples:

```bash
# Build the multi-adapter example
gn gen out/Debug
ninja -C out/Debug asol/examples:multi_adapter_example

# Run the example
out/Debug/multi_adapter_example

# Build the AI settings example
ninja -C out/Debug browser_core:ai_settings_example

# Run the example
out/Debug/ai_settings_example

# Build the AI provider menu button example
ninja -C out/Debug browser_core:ai_menu_button_example

# Run the example
out/Debug/ai_menu_button_example
```

## Configuration Options

Each adapter supports various configuration options. See the adapter-specific documentation for details:

- **Gemini**: See `asol/adapters/gemini/README.md`
- **OpenAI**: See `asol/adapters/openai/README.md`
- **Copilot**: See `asol/adapters/copilot/README.md`
- **Claude**: See `asol/adapters/claude/README.md`

## Adding a New Adapter

To add a new adapter:

1. Create a new directory under `asol/adapters/`
2. Implement the adapter and service provider classes
3. Add the adapter to the `AdapterFactory`
4. Update the BUILD.gn files to include the new adapter

See the existing adapters for examples of the implementation pattern.