# ASOL UI Integration

## Overview

The ASOL UI integration provides a user interface for interacting with the AI capabilities of the DashAIBrowser. It consists of several components that work together to provide a seamless user experience.

## Components

### 1. UI Controller

The UI controller (`asol/ui/asol_ui_controller.h`) serves as a bridge between the UI components and the ASOL backend. It handles user input, routes requests to the appropriate adapters, and manages the UI state.

Key features:
- Processing text input from the UI
- Capability-based routing
- Adapter selection
- Streaming support for real-time responses
- Panel visibility management

### 2. UI Panel

The UI panel (`asol/ui/asol_ui_panel.h`) is the visual component that displays AI interactions. It provides a user interface for entering text, selecting adapters and capabilities, and viewing responses.

Key features:
- Text input field
- Adapter and capability selection
- Conversation display
- Streaming response visualization
- Error handling

### 3. Browser Integration

The browser integration (`asol/browser/asol_browser_integration.h`) connects the ASOL system to the browser. It observes web contents and provides AI capabilities to the browser.

Key features:
- WebContents integration
- Page context extraction
- Panel visibility management
- Navigation handling

## User Flow

1. User activates the AI panel (via keyboard shortcut, menu item, or toolbar button)
2. User enters text in the input field
3. User selects an adapter or capability (or uses the default)
4. User submits the text
5. UI controller processes the text and routes it to the appropriate adapter
6. Adapter processes the text and returns a response
7. UI panel displays the response (with streaming updates if supported)
8. User can continue the conversation or clear it

## Implementation Details

### UI Controller

The UI controller is implemented as a singleton that manages the UI state and routes requests to the ASOL backend. It provides methods for processing text input, selecting adapters and capabilities, and managing the UI panel.

```cpp
// Process text input from the UI
void ProcessTextInput(const std::string& text_input,
                     const std::string& adapter_id,
                     UiResponseCallback callback);

// Process text input using capability-based routing
void ProcessTextInputWithCapability(const std::string& text_input,
                                  const std::string& capability,
                                  UiResponseCallback callback);
```

### UI Panel

The UI panel is implemented as a views::Widget that displays the conversation and provides controls for interacting with the AI. It uses the views framework to create a responsive and accessible UI.

```cpp
// Add a user message to the conversation
void AddUserMessage(const std::string& message);

// Add an AI response to the conversation
void AddAiResponse(const std::string& response, bool is_final);

// Add an error message to the conversation
void AddErrorMessage(const std::string& error);
```

### Browser Integration

The browser integration is implemented as a WebContentsUserData that observes web contents and provides AI capabilities to the browser. It creates and manages the UI controller and panel.

```cpp
// Show the AI panel
void ShowAiPanel();

// Hide the AI panel
void HideAiPanel();

// Toggle the AI panel
void ToggleAiPanel();
```

## Future Enhancements

1. **Context Extraction**: Extract context from the current page to provide more relevant AI responses
2. **Multi-modal Input**: Support for images, audio, and other input types
3. **Customizable UI**: Allow users to customize the appearance and behavior of the AI panel
4. **Keyboard Shortcuts**: Add keyboard shortcuts for common actions
5. **Accessibility**: Ensure the UI is fully accessible to all users
6. **Internationalization**: Support for multiple languages in the UI
7. **History Management**: Save and restore conversation history
8. **Settings UI**: Add a UI for configuring ASOL settings