# ASOL Research Mode

## Overview

ASOL Research Mode is a powerful feature designed to enhance the research experience within the browser. It allows users to collect, organize, and analyze information from multiple web pages, generating insights and summaries to aid in comprehensive research tasks.

## Key Features

### 1. Research Sessions

Research Mode organizes information into research sessions, which are collections of pages related to a specific research topic. Each session includes:

- A name and topic
- A collection of web pages with their content
- Key points extracted from each page
- Timestamps for tracking when information was collected
- Summaries and analyses of the collected information

### 2. Automatic Content Collection

Research Mode can automatically collect content from pages you visit:

- **Auto-add Pages**: When enabled, pages you visit are automatically added to your current research session
- **Content Extraction**: Intelligently extracts the main content from pages, filtering out navigation, ads, and other non-essential elements
- **Title and URL Tracking**: Maintains references to the original sources

### 3. AI-Powered Analysis

Research Mode leverages ASOL's AI capabilities to analyze and organize research:

- **Key Points Extraction**: Automatically identifies and extracts the most important points from each page
- **Session Summaries**: Generates comprehensive summaries that synthesize information from all collected pages
- **Document Generation**: Creates well-structured documents from your research in various formats

### 4. Research Organization

Research Mode provides tools for organizing and managing your research:

- **Multiple Sessions**: Create and switch between different research sessions for different topics
- **Search Within Sessions**: Find specific information across all pages in a session
- **Page Management**: Add, remove, and update pages in your research sessions

## Using Research Mode

### Enabling Research Mode

1. Click the ASOL icon in the browser toolbar to open the AI panel
2. Click the "Research Mode" toggle to enable research mode
3. Create a new research session or select an existing one

### Creating a Research Session

1. With Research Mode enabled, click "New Session"
2. Enter a name and topic for your research session
3. Choose whether to automatically add pages you visit

### Adding Pages to Research

Pages can be added to your research session in several ways:

- **Automatic Collection**: If auto-add is enabled, pages you visit are automatically added
- **Manual Addition**: Click "Add Current Page" in the Research panel to add the current page
- **Context Menu**: Right-click on a page and select "Add to Research"

### Generating Insights

Research Mode provides several ways to analyze your collected information:

- **Generate Key Points**: Extract the most important points from a page
- **Create Summary**: Generate a comprehensive summary of all collected information
- **Export to Document**: Create a well-structured document from your research

### Searching Your Research

1. In the Research panel, click the search icon
2. Enter your search query
3. View matching pages and key points from across your research session

## Configuration Options

Research Mode can be customized through several settings:

- **Auto-add Pages**: Automatically add pages you visit to your research session
- **Auto-generate Key Points**: Automatically extract key points when pages are added
- **Max Pages Per Session**: Limit the number of pages in a session (default: 100)
- **Content Extraction**: Choose what content to extract from pages (full page, visible content, etc.)

## Integration with Other Features

Research Mode integrates with other ASOL features:

- **Side Panel Integration**: Access Research Mode directly from the browser's side panel
- **Context Menu Integration**: Add pages to research sessions via the context menu
- **Performance Optimization**: Efficient storage and processing of research data

## Technical Implementation

Research Mode is implemented through several key components:

- **ResearchModeController**: Manages research sessions and coordinates with other components
- **PageContextExtractor**: Extracts relevant content from web pages
- **ServiceManager**: Processes research data using AI capabilities
- **Feature Flags**: Controls the availability and behavior of Research Mode features

## Best Practices

For the most effective research experience:

1. **Create Focused Sessions**: Create separate research sessions for different topics
2. **Review Auto-added Content**: Periodically review and clean up automatically added pages
3. **Generate Key Points**: Use the key points feature to distill important information
4. **Create Summaries**: Generate summaries to synthesize information across multiple sources
5. **Export Important Research**: Export valuable research to documents for sharing or archiving

## Future Enhancements

Planned enhancements for Research Mode include:

1. **Citation Management**: Automatic generation of citations in various formats
2. **Collaborative Research**: Sharing research sessions with others
3. **Advanced Filtering**: More sophisticated filtering and organization of research data
4. **Visual Knowledge Graphs**: Visual representation of connections between research topics
5. **Offline Support**: Full functionality when offline