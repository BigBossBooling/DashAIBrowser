#include "asol/adapters/gemini/gemini_text_adapter.h"
#include <iostream> // For placeholder logging
#include <vector>   // For header list in dummy network call

// Note: The dashaibrowser::asol::adapters::GeminiTextAdapter::NetworkRequestHandler
//       class definition is removed from here as it's replaced by IHttpClient.

namespace dashaibrowser {
namespace asol {
namespace adapters {

GeminiTextAdapter::GeminiTextAdapter(std::unique_ptr<utils::IHttpClient> http_client)
    : http_client_(std::move(http_client)) {
    if (!http_client_) {
        http_client_ = std::make_unique<utils::PlaceholderHttpClient>();
    }
    std::cout << "GeminiTextAdapter: Instance created." << std::endl;
}

GeminiTextAdapter::~GeminiTextAdapter() {
    std::cout << "GeminiTextAdapter: Instance destroyed." << std::endl;
}

void GeminiTextAdapter::SetError(ipc::ErrorDetails* error_details,
                                 int32_t code,
                                 const std::string& message,
                                 const std::string& user_message) {
    if (error_details) {
        error_details->set_error_code(code);
        error_details->set_error_message(message);
        if (!user_message.empty()) {
            error_details->set_user_facing_message(user_message);
        }
    } else {
        std::cerr << "GeminiTextAdapter::SetError called with null error_details. Code: " << code << ", Msg: " << message << std::endl;
    }
}

bool GeminiTextAdapter::Initialize(const GeminiAdapterConfig& config) {
    config_ = config;
    if (config_.api_key.empty()) {
        std::cerr << "GeminiTextAdapter::Initialize Error: API key is missing." << std::endl;
    }
    if (config_.api_endpoint_summarize.empty() || config_.api_endpoint_translate.empty()) {
        std::cerr << "GeminiTextAdapter::Initialize Error: API endpoint(s) are missing." << std::endl;
    }
    initialized_ = true;
    std::cout << "GeminiTextAdapter: Initialized. API Key: "
              << (config_.api_key.empty() ? "NOT SET" : config_.api_key.substr(0,5) + "...")
              << std::endl;
    std::cout << "GeminiTextAdapter: Summarize Endpoint: " << config_.api_endpoint_summarize << std::endl;
    std::cout << "GeminiTextAdapter: Translate Endpoint: " << config_.api_endpoint_translate << std::endl;
    return initialized_;
}

std::string GeminiTextAdapter::GetSummary(
    const std::string& text,
    const dashaibrowser::ipc::UserPreferences& prefs,
    dashaibrowser::ipc::ErrorDetails* error_details) {

    if (!initialized_) {
        SetError(error_details, 500, "Adapter not initialized.", "Service configuration error.");
        return "";
    }
    if (text.empty()) {
        SetError(error_details, 400, "Input text is empty for summary.", "Cannot summarize empty text.");
        return "";
    }

    std::cout << "GeminiTextAdapter::GetSummary: Requesting summary for text: \""
              << text.substr(0, 50) << "...\"" << std::endl;

    std::string request_body = R"({"contents": [{"parts": [{"text": ")" + text + R"("}]}]})";
    if (text == "error_test") request_body = R"({"contents": [{"parts": [{"text": "error_test"}]}]})";
    if (text == "error_test_network_failure") request_body = R"({"contents": [{"parts": [{"text": "error_test_network_failure"}]}]})";
    if (text == "error_test_timeout") request_body = R"({"contents": [{"parts": [{"text": "error_test_timeout"}]}]})";
    if (text == "error_test_401_unauthorized") request_body = R"({"contents": [{"parts": [{"text": "error_test_401_unauthorized"}]}]})";


    std::vector<std::string> headers = {
        "Content-Type: application/json",
        // Gemini API key is often passed as a query parameter `key=API_KEY`
        // or sometimes as a header like `X-Goog-Api-Key: API_KEY`.
        // For this placeholder, we'll assume it's part of the URL or handled by the http_client if needed.
        // If it were a header:
        // "X-Goog-Api-Key: " + config_.api_key
    };

    // Construct URL with API key if it's a query parameter
    std::string url = config_.api_endpoint_summarize;
    if (!config_.api_key.empty()) {
        url += (url.find('?') == std::string::npos ? "?" : "&") + std::string("key=") + config_.api_key;
    }

    utils::HttpResponse http_response = http_client_->Post(
        url,
        request_body,
        headers,
        config_.timeout_ms
    );

    std::cout << "GeminiTextAdapter::GetSummary: Raw response status: " << http_response.status_code
              << ", body: " << http_response.body << std::endl;

    if (!http_response.IsSuccess()) {
        std::string err_msg = "HTTP request failed with status " + std::to_string(http_response.status_code);
        if (!http_response.error_message.empty()) {
             err_msg += ": " + http_response.error_message;
        } else if (!http_response.body.empty()) {
            err_msg += ". Body: " + http_response.body.substr(0,100) + "...";
        }
        SetError(error_details, http_response.status_code == 0 ? 504 : static_cast<int32_t>(http_response.status_code),
                 err_msg, "AI service communication error.");
        return "";
    }

    // Placeholder JSON parsing
    if (http_response.body.find("\"error\"") != std::string::npos) {
        std::string error_msg = "Gemini API returned an error. Raw: " + http_response.body;
        // Potentially parse the actual error message from JSON here
        SetError(error_details, 502, error_msg, "AI service failed to process the request.");
        return "";
    }

    size_t text_start = http_response.body.find("\"text\": \"");
    if (text_start != std::string::npos) {
        text_start += 9;
        size_t text_end = http_response.body.find("\"", text_start);
        if (text_end != std::string::npos) {
            std::string summary = http_response.body.substr(text_start, text_end - text_start);
            SetError(error_details, 0, "", "");
            return summary;
        }
    }

    SetError(error_details, 503, "Failed to parse summary from Gemini response.", "AI service returned an unexpected response.");
    return "";
}

std::string GeminiTextAdapter::TranslateText(
    const std::string& text,
    const std::string& source_lang_code,
    const std::string& target_lang_code,
    const dashaibrowser::ipc::UserPreferences& prefs,
    dashaibrowser::ipc::ErrorDetails* error_details) {

    if (!initialized_) {
        SetError(error_details, 500, "Adapter not initialized.", "Service configuration error.");
        return "";
    }
     if (text.empty()) {
        SetError(error_details, 400, "Input text is empty for translation.", "Cannot translate empty text.");
        return "";
    }
    if (target_lang_code.empty()) {
        SetError(error_details, 400, "Target language code is empty.", "Please specify a target language.");
        return "";
    }

    std::cout << "GeminiTextAdapter::TranslateText: Requesting translation for: \""
              << text.substr(0, 50) << "...\" from " << source_lang_code
              << " to " << target_lang_code << std::endl;

    std::string prompt = "Translate the following text from " +
                         (source_lang_code.empty() || source_lang_code == "auto" ? "the detected language" : source_lang_code) +
                         " to " + target_lang_code + ":\n\n" + text;
    std::string request_body = R"({"contents": [{"parts": [{"text": ")" + prompt + R"("}]}]})";
    if (text == "error_test") request_body = R"({"contents": [{"parts": [{"text": "error_test"}]}]})";

    std::vector<std::string> headers = {"Content-Type: application/json"};
    std::string url = config_.api_endpoint_translate;
    if (!config_.api_key.empty()) {
        url += (url.find('?') == std::string::npos ? "?" : "&") + std::string("key=") + config_.api_key;
    }

    utils::HttpResponse http_response = http_client_->Post(
        url,
        request_body,
        headers,
        config_.timeout_ms
    );

    std::cout << "GeminiTextAdapter::TranslateText: Raw response status: " << http_response.status_code
              << ", body: " << http_response.body << std::endl;

    if (!http_response.IsSuccess()) {
        std::string err_msg = "HTTP request failed with status " + std::to_string(http_response.status_code);
        if (!http_response.error_message.empty()) {
             err_msg += ": " + http_response.error_message;
        } else if (!http_response.body.empty()) {
            err_msg += ". Body: " + http_response.body.substr(0,100) + "...";
        }
        SetError(error_details, http_response.status_code == 0 ? 504 : static_cast<int32_t>(http_response.status_code),
                 err_msg, "AI service communication error.");
        return "";
    }

    if (http_response.body.find("\"error\"") != std::string::npos) {
        std::string error_msg = "Gemini API returned an error. Raw: " + http_response.body;
        SetError(error_details, 502, error_msg, "AI service failed to process the request.");
        return "";
    }

    size_t text_start = http_response.body.find("\"text\": \"");
    if (text_start != std::string::npos) {
        text_start += 9;
        size_t text_end = http_response.body.find("\"", text_start);
        if (text_end != std::string::npos) {
            std::string translated_text = http_response.body.substr(text_start, text_end - text_start);
            SetError(error_details, 0, "", "");
            return translated_text;
        }
    }

    SetError(error_details, 503, "Failed to parse translation from Gemini response.", "AI service returned an unexpected response.");
    return "";
}

} // namespace adapters
} // namespace asol
} // namespace dashaibrowser
