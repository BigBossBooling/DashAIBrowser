#ifndef DASHAI_BROWSER_ASOL_ADAPTERS_GEMINI_GEMINI_TEXT_ADAPTER_H_
#define DASHAI_BROWSER_ASOL_ADAPTERS_GEMINI_GEMINI_TEXT_ADAPTER_H_

#include "proto/asol_service.pb.h" // For UserPreferences, ErrorDetails
#include "asol/cpp/utils/network_request_util.h" // For IHttpClient
#include <string>
#include <vector>
#include <map> // For AdapterConfig
#include <memory> // For std::unique_ptr

namespace dashaibrowser {
namespace asol {
namespace adapters {

// Configuration for the adapter, e.g., API key, endpoint URL.
struct GeminiAdapterConfig {
    std::string api_key;
    std::string api_endpoint_summarize;
    std::string api_endpoint_translate;
    int timeout_ms = 10000;
};

// Interface for a Gemini Text Adapter
class IGeminiTextAdapter {
public:
    virtual ~IGeminiTextAdapter() = default;

    virtual bool Initialize(const GeminiAdapterConfig& config) = 0;

    virtual std::string GetSummary(
        const std::string& text,
        const dashaibrowser::ipc::UserPreferences& prefs,
        dashaibrowser::ipc::ErrorDetails* error_details
    ) = 0;

    virtual std::string TranslateText(
        const std::string& text,
        const std::string& source_lang_code,
        const std::string& target_lang_code,
        const dashaibrowser::ipc::UserPreferences& prefs,
        dashaibrowser::ipc::ErrorDetails* error_details
    ) = 0;
};


// Concrete implementation of the Gemini Text Adapter
class GeminiTextAdapter : public IGeminiTextAdapter {
public:
    // Constructor can optionally take an IHttpClient for testing/mocking.
    // If nullptr is passed, it will create a default PlaceholderHttpClient.
    explicit GeminiTextAdapter(std::unique_ptr<utils::IHttpClient> http_client = nullptr);
    ~GeminiTextAdapter() override;

    bool Initialize(const GeminiAdapterConfig& config) override;

    std::string GetSummary(
        const std::string& text,
        const dashaibrowser::ipc::UserPreferences& prefs,
        dashaibrowser::ipc::ErrorDetails* error_details
    ) override;

    std::string TranslateText(
        const std::string& text,
        const std::string& source_lang_code,
        const std::string& target_lang_code,
        const dashaibrowser::ipc::UserPreferences& prefs,
        dashaibrowser::ipc::ErrorDetails* error_details
    ) override;

private:
    GeminiAdapterConfig config_;
    bool initialized_ = false;
    std::unique_ptr<utils::IHttpClient> http_client_; // Use the utility interface

    void SetError(ipc::ErrorDetails* error_details,
                  int32_t code,
                  const std::string& message,
                  const std::string& user_message = "");
};

} // namespace adapters
} // namespace asol
} // namespace dashaibrowser

#endif // DASHAI_BROWSER_ASOL_ADAPTERS_GEMINI_GEMINI_TEXT_ADAPTER_H_
