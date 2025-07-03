# ASOL Code Quality Guidelines

## Overview

This document outlines the code quality guidelines for the ASOL project. Following these guidelines ensures that the codebase remains maintainable, readable, and robust as it grows.

## Coding Style

The ASOL project follows the [Chromium coding style](https://chromium.googlesource.com/chromium/src/+/main/styleguide/c++/c++.md) with some project-specific additions.

### Naming Conventions

- **Classes and Types**: Use `CamelCase` (e.g., `GeminiTextAdapter`)
- **Methods and Functions**: Use `CamelCase` (e.g., `ProcessText`)
- **Variables**: Use `snake_case` (e.g., `response_text`)
- **Constants**: Use `kConstantName` (e.g., `kMaxResponseSize`)
- **Macros**: Use `ALL_CAPS_WITH_UNDERSCORES` (e.g., `ASOL_TRACK_PERFORMANCE`)
- **Namespaces**: Use `lowercase` (e.g., `asol::adapters::gemini`)
- **File Names**: Use `snake_case.h/cc` (e.g., `gemini_text_adapter.h`)

### File Organization

- **Header Guards**: Use `#ifndef ASOL_PATH_FILE_H_` style
- **Include Order**:
  1. Related header (e.g., `foo.cc` includes `foo.h` first)
  2. C system headers
  3. C++ system headers
  4. Other libraries' headers
  5. Project headers
- **Forward Declarations**: Use when possible to reduce include dependencies

### Code Structure

- **Class Structure**:
  - Public methods first
  - Protected methods next
  - Private methods last
  - Group related methods together
- **Method Length**: Keep methods under 40 lines when possible
- **File Length**: Keep files under 1000 lines when possible
- **Line Length**: Limit lines to 80 characters when reasonable

## Documentation

### Code Comments

- **Class Comments**: Document the purpose and usage of each class
- **Method Comments**: Document the purpose, parameters, return values, and any side effects
- **Complex Logic**: Add comments explaining complex or non-obvious logic
- **TODO Comments**: Include the owner's username and a specific action item

Example:
```cpp
// GeminiTextAdapter processes text using the Gemini API.
// It handles request formatting, response parsing, and error handling.
class GeminiTextAdapter : public AdapterInterface {
 public:
  // ProcessText sends the text_input to the Gemini API and returns the response.
  // Returns a ModelResponse containing the generated text or an error message.
  ModelResponse ProcessText(const std::string& text_input) override;
  
  // TODO(username): Add support for multi-modal inputs.
};
```

### API Documentation

- Document all public APIs thoroughly
- Include usage examples for complex APIs
- Document performance considerations and thread safety
- Specify ownership semantics for pointers

## Error Handling

### Best Practices

- **Be Explicit**: Use explicit error handling rather than implicit
- **Check Inputs**: Validate all inputs at API boundaries
- **Meaningful Messages**: Provide clear, actionable error messages
- **Consistent Patterns**: Use consistent error handling patterns throughout the codebase
- **Avoid Exceptions**: Prefer return values for error handling over exceptions

### Logging

- **Error Conditions**: Log all error conditions with appropriate severity
- **Context**: Include sufficient context in log messages
- **Privacy**: Avoid logging sensitive information
- **Performance Impact**: Be mindful of the performance impact of logging

## Testing

### Test Coverage

- **Unit Tests**: Write unit tests for all public methods
- **Integration Tests**: Write integration tests for component interactions
- **Edge Cases**: Test boundary conditions and error cases
- **Performance Tests**: Include performance tests for critical paths

### Test Structure

- **Test Naming**: Use descriptive test names that indicate what is being tested
- **Test Organization**: Group related tests together
- **Test Independence**: Each test should be independent of others
- **Test Readability**: Tests should be easy to understand and maintain

Example:
```cpp
TEST_F(GeminiTextAdapterTest, ProcessTextWithValidInput) {
  // Setup
  GeminiTextAdapter adapter;
  adapter.Initialize(valid_config_);
  
  // Exercise
  ModelResponse response = adapter.ProcessText("Valid input text");
  
  // Verify
  EXPECT_TRUE(response.success);
  EXPECT_FALSE(response.text.empty());
}
```

## Performance Considerations

### Best Practices

- **Critical Paths**: Optimize critical paths for performance
- **Memory Usage**: Be mindful of memory usage, especially for long-lived objects
- **Copying**: Avoid unnecessary copying of large objects
- **Threading**: Use appropriate threading models for CPU-intensive operations
- **Measurement**: Use the performance tracker to measure and optimize performance

## Security

### Best Practices

- **Input Validation**: Validate all inputs, especially those from external sources
- **API Keys**: Handle API keys and other credentials securely
- **User Data**: Protect user data and respect privacy
- **Dependencies**: Keep dependencies up to date to avoid security vulnerabilities
- **Error Messages**: Avoid leaking sensitive information in error messages

## Code Review

### Process

- **Small Changes**: Keep changes small and focused
- **Self-Review**: Review your own code before submitting
- **Documentation**: Update documentation along with code changes
- **Tests**: Include tests with all code changes
- **Performance**: Consider performance implications of changes

### Checklist

- Does the code follow the style guidelines?
- Is the code well-documented?
- Are there appropriate tests?
- Are error cases handled properly?
- Are there any performance concerns?
- Are there any security concerns?
- Is the code maintainable?

## Conclusion

Following these guidelines will help ensure that the ASOL codebase remains high-quality, maintainable, and robust as it grows. These guidelines are not exhaustive, and good judgment should always be used when writing and reviewing code.