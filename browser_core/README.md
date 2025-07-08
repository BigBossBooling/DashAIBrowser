# DashAI Browser Core Components

This directory contains the core components of the DashAI Browser, including AI features, content extraction, and browser integration.

## AI-Summarization & Content Digest Feature

The AI-Summarization feature provides intelligent summarization of web content, allowing users to quickly understand the key points of articles, documentation, and other web pages.

### Components

- **SummarizationService**: Core service that processes content and generates summaries using AI models.
- **SummarizationFeature**: Browser feature that integrates the summarization service with the browser UI.
- **ContentExtractor**: Extracts and processes content from web pages for summarization.
- **BrowserContentHandler**: Manages content extraction and processing for browser pages.
- **BrowserFeatures**: Manages all AI-powered features in the browser.
- **BrowserAIIntegration**: Main integration point for AI features in the browser.

### Key Features

1. **Intelligent Content Summarization**
   - Automatically detects when a page is suitable for summarization
   - Provides different summary formats (executive summary, bullet points, Q&A, etc.)
   - Adjustable summary length based on user preference

2. **Synapse Button**
   - Pulsating icon in the Omnibox that indicates when a page can be summarized
   - Clicking it triggers the summarization feature

3. **Summary Sidebar**
   - Non-intrusive sidebar that displays the summary
   - Includes source links back to the original content
   - Provides controls for adjusting summary format and length

4. **Privacy-Preserving Processing**
   - All content is processed through a privacy proxy before being sent to AI services
   - Sensitive information is redacted to protect user privacy

### Usage

The AI-Summarization feature can be triggered in several ways:

1. **Manual Trigger**: Click the Synapse button in the Omnibox to generate a summary.
2. **Automatic Trigger**: The browser can automatically generate summaries for eligible pages.
3. **Hybrid Mode**: Combines manual and automatic triggers based on content characteristics.

### Building and Testing

To build the AI-Summarization feature:

```bash
gn gen out/Debug
ninja -C out/Debug browser_core
```

To run the example application:

```bash
./out/Debug/summarization_example
```

## Integration with ASOL

The AI-Summarization feature integrates with the AI Service Orchestration Layer (ASOL) to leverage the best AI model for summarization while ensuring privacy protection.

- **AIServiceManager**: Manages AI services and selects the appropriate model for summarization.
- **PrivacyProxy**: Processes content to remove sensitive information before sending to AI services.

## Future Enhancements

1. **Multi-model Support**: Support for different AI models based on content type and user preference.
2. **Offline Summarization**: Local summarization for privacy-sensitive content.
3. **Customizable Summaries**: More options for customizing summary format and style.
4. **Cross-page Summarization**: Summarize multiple related pages together.
5. **Summary Sharing**: Easy sharing of summaries with others.