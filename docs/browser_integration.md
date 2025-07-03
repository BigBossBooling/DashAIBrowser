# ASOL Browser Integration Guide

## Overview

This document provides a comprehensive guide to the browser integration features of the ASOL system. It covers the various ways ASOL integrates with the browser to provide a seamless AI experience.

## Integration Components

### 1. Side Panel Integration

The side panel integration is the primary way users interact with ASOL. It provides a dedicated panel in the browser's side panel UI where users can interact with AI capabilities.

#### Key Components:
- `SidePanelController`: Manages the ASOL side panel integration
- `side_panel::SidePanelEntry`: Represents the ASOL entry in the side panel
- `side_panel::SidePanelRegistry`: Registry for side panel entries

#### Usage:
```cpp
// Create and initialize the side panel controller
auto* controller = SidePanelController::CreateForWebContents(web_contents);

// Show the side panel
controller->ShowSidePanel();

// Hide the side panel
controller->HideSidePanel();

// Toggle the side panel
controller->ToggleSidePanel();
```

### 2. Page Context Extraction

The page context extraction feature allows ASOL to understand the content of the current page and provide more relevant AI responses.

#### Key Components:
- `PageContextExtractor`: Extracts context from the current page
- JavaScript execution for content extraction
- Context processing and relevance filtering

#### Usage:
```cpp
// Create a context extractor
auto extractor = std::make_unique<PageContextExtractor>(web_contents);

// Extract context from the current page
extractor->ExtractContext(base::BindOnce([](const std::string& context) {
  // Use the extracted context
  LOG(INFO) << "Extracted context: " << context;
}));

// Extract context from selected text
extractor->ExtractSelectedText(base::BindOnce([](const std::string& selected_text) {
  // Use the selected text
  LOG(INFO) << "Selected text: " << selected_text;
}));
```

### 3. Browser Features and Flags

ASOL uses feature flags to control the availability and behavior of various features. This allows for granular control and experimentation.

#### Key Components:
- `browser_features.h`: Defines feature flags and parameters
- `base::Feature`: Represents a feature that can be enabled or disabled
- `base::FeatureParam`: Represents a parameter for a feature

#### Usage:
```cpp
// Check if a feature is enabled
if (base::FeatureList::IsEnabled(features::kAsolSidePanelIntegration)) {
  // Feature-specific code here
}

// Get a feature parameter
int cache_size = base::GetFieldTrialParamByFeatureAsInt(
    features::kAsolResponseCaching, "cache_size", 100);
```

### 4. Browser Integration

The browser integration component connects ASOL to the browser's core functionality, such as web contents observation and navigation handling.

#### Key Components:
- `AsolBrowserIntegration`: Integrates ASOL with the browser
- `WebContentsObserver`: Observes web contents events
- `WebContentsUserData`: Associates ASOL with web contents

#### Usage:
```cpp
// Create and initialize the browser integration
auto* integration = AsolBrowserIntegration::CreateForWebContents(web_contents);

// Show the AI panel
integration->ShowAiPanel();

// Hide the AI panel
integration->HideAiPanel();

// Toggle the AI panel
integration->ToggleAiPanel();
```

## Integration Patterns

### 1. WebContentsUserData Pattern

ASOL uses the `WebContentsUserData` pattern to associate its components with web contents. This pattern ensures that each web contents has its own instance of ASOL components.

```cpp
class MyComponent : public content::WebContentsUserData<MyComponent> {
 public:
  // ...
  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

// In the .cc file:
WEB_CONTENTS_USER_DATA_KEY_IMPL(MyComponent);
```

### 2. Observer Pattern

ASOL uses the observer pattern to respond to browser events, such as navigation and page changes.

```cpp
class MyObserver : public content::WebContentsObserver {
 public:
  explicit MyObserver(content::WebContents* web_contents)
      : content::WebContentsObserver(web_contents) {}

  // Override observer methods
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override {
    // Handle navigation
  }
};
```

### 3. Feature Flag Pattern

ASOL uses feature flags to control the availability and behavior of features.

```cpp
// Define a feature
const base::Feature kMyFeature{"MyFeature", base::FEATURE_ENABLED_BY_DEFAULT};

// Define a parameter
const base::FeatureParam<int> kMyParam{&kMyFeature, "my_param", 100};

// Check if the feature is enabled
if (base::FeatureList::IsEnabled(kMyFeature)) {
  // Feature-specific code here
}

// Get the parameter value
int param_value = base::GetFieldTrialParamByFeatureAsInt(
    kMyFeature, "my_param", 100);
```

## Best Practices

1. **Respect Browser Lifecycle**: Always respect the browser's lifecycle events, such as web contents destruction.

2. **Minimize UI Thread Impact**: Avoid blocking the UI thread with long-running operations.

3. **Handle Errors Gracefully**: Always handle errors gracefully and provide meaningful feedback to users.

4. **Use Feature Flags**: Use feature flags to control the availability and behavior of features.

5. **Respect User Privacy**: Always respect user privacy and handle user data with care.

6. **Test Thoroughly**: Test integration thoroughly with different browser configurations and states.

## Conclusion

The ASOL browser integration provides a seamless AI experience within the browser. By following the patterns and best practices outlined in this guide, you can ensure that your integration is robust, performant, and user-friendly.