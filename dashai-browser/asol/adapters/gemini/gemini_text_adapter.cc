#include "asol/adapters/gemini/gemini_text_adapter.h"
#include <iostream> // For placeholder logging
#include <vector>   // For header list in dummy network call
#include <sstream>  // For std::ostringstream (manual JSON construction)
#include <iomanip>  // For std::setw, std::setfill with std::hex for EscapeJsonString

// Note: The dashaibrowser::asol::adapters::GeminiTextAdapter::NetworkRequestHandler
//       class definition is removed from here as it's replaced by IHttpClient.

namespace dashaibrowser {
namespace asol {
namespace adapters {

// Helper to escape strings for JSON
std::string EscapeJsonString(const std::string& input) {
    std::ostringstream oss;
    for (char c : input) {
        switch (c) {
            case '"': oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\b': oss << "\\b"; break;
            case '\f': oss << "\\f"; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default:
                // Control characters (0x00-0x1F) need to be escaped.
                // JSON spec also recommends escaping others like / for HTML safety,
                // but we'll keep it simple for now.
                if (c >= 0 && c <= 0x1f) {
                    oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(c));
                } else {
                    oss << c;
                }
        }
    }
    return oss.str();
}

GeminiTextAdapter::GeminiTextAdapter(std::unique_ptr<utils::IHttpClient> http_client)
    : http_client_(std::move(http_client)) {
    if (!http_client_) {
        // Default to CurlHttpClient if none provided, assuming GlobalInit has been called.
        // This makes direct instantiation easier for AsolServiceImpl.
        http_client_ = std::make_unique<utils::CurlHttpClient>();
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
        std::cout << "GeminiTextAdapter::Initialize Warning: API key is missing in config." << std::endl;
    }
    if (config_.api_endpoint_summarize.empty()) {
        std::cerr << "GeminiTextAdapter::Initialize Error: Summarize API endpoint is missing." << std::endl;
        initialized_ = false;
        return false;
    }
    if (config_.api_endpoint_translate.empty()) {
        config_.api_endpoint_translate = config_.api_endpoint_summarize; // Default if not set
        std::cout << "GeminiTextAdapter::Initialize Info: Translate endpoint not set, using summarize endpoint." << std::endl;
    }
    if (config_.api_endpoint_generate_text.empty()) {
        config_.api_endpoint_generate_text = config_.api_endpoint_summarize; // Default if not set
         std::cout << "GeminiTextAdapter::Initialize Info: GenerateText endpoint not set, using summarize endpoint." << std::endl;
    }

    initialized_ = true;
    std::cout << "GeminiTextAdapter: Initialized. API Key: "
              << (config_.api_key.empty() ? "NOT SET (will rely on URL construction)" : "SET")
              << std::endl;
    std::cout << "GeminiTextAdapter: Summarize Endpoint: " << config_.api_endpoint_summarize << std::endl;
    std::cout << "GeminiTextAdapter: Translate Endpoint: " << config_.api_endpoint_translate << std::endl;
    std::cout << "GeminiTextAdapter: GenerateText Endpoint: " << config_.api_endpoint_generate_text << std::endl;
    return initialized_;
}

// TODO: Replace manual JSON construction and parsing with a proper JSON library.
std::string GeminiTextAdapter::GetSummary(
    const std::string& text,
    const dashaibrowser::ipc::UserPreferences& prefs,
    dashaibrowser::ipc::ErrorDetails* error_details) {

    SetError(error_details, 0, "", "");
    if (!initialized_) {
        SetError(error_details, 500, "Adapter not initialized.", "Service configuration error.");
        return "";
    }
    if (text.empty()) {
        SetError(error_details, 400, "Input text is empty for summary.", "Cannot summarize empty text.");
        return "";
    }

    std::cout << "GeminiTextAdapter::GetSummary: Requesting summary for text (first 50 chars): \""
              << text.substr(0, 50) << "...\"" << std::endl;

    std::string escaped_text = EscapeJsonString("Summarize the following text: " + text);
    std::string request_body =
        "{\n"
        "  \"contents\": [\n"
        "    {\n"
        "      \"role\": \"user\",\n"
        "      \"parts\": [\n"
        "        { \"text\": \"" + escaped_text + "\" }\n"
        "      ]\n"
        "    }\n"
        "  ],\n"
        "  \"generationConfig\": {\n"
        "    \"responseMimeType\": \"text/plain\"\n"
        // TODO: Consider making temperature, topK, topP configurable via UserPreferences
        // "    \"temperature\": 0.7,\n"
        // "    \"topK\": 1,\n"
        // "    \"topP\": 1,\n"
        // "    \"maxOutputTokens\": 2048,\n"
        // "    \"stopSequences\": []\n"
        "  }\n"
        "}";

    std::vector<std::string> headers = {"Content-Type: application/json"};

    std::string url = config_.api_endpoint_summarize;
    if (url.find('?') == std::string::npos) {
        url += "?key=" + config_.api_key;
    } else {
        url += "&key=" + config_.api_key;
    }

    // std::cout << "GeminiTextAdapter::GetSummary: URL: " << url << std::endl; // Verbose
    // std::cout << "GeminiTextAdapter::GetSummary: Request Body: " << request_body << std::endl; // Verbose


    utils::HttpResponse http_response = http_client_->Post(
        url, request_body, headers, config_.timeout_ms
    );

    // std::cout << "GeminiTextAdapter::GetSummary: Response Status: " << http_response.status_code << std::endl; // Verbose
    // std::cout << "GeminiTextAdapter::GetSummary: Response Body: " << http_response.body << std::endl; // Verbose


    if (!http_response.IsSuccess()) {
        std::string err_msg = "Gemini API HTTP request failed for GetSummary. Status: " + std::to_string(http_response.status_code);
        if (!http_response.error_message.empty()) {
             err_msg += ". Network Error: " + http_response.error_message;
        } else if (!http_response.body.empty()) {
            size_t err_msg_start = http_response.body.find("\"message\": \"");
            if (err_msg_start != std::string::npos) {
                err_msg_start += 12;
                size_t err_msg_end = http_response.body.find("\"", err_msg_start);
                if (err_msg_end != std::string::npos) {
                    err_msg += ". API Message: " + http_response.body.substr(err_msg_start, err_msg_end - err_msg_start);
                }
            } else {
                 err_msg += ". Body: " + http_response.body.substr(0,100) + "...";
            }
        }
        SetError(error_details, http_response.status_code == 0 ? 504 : static_cast<int32_t>(http_response.status_code),
                 err_msg, "AI service communication error.");
        return "";
    }

    size_t candidates_pos = http_response.body.find("\"candidates\":");
    if (candidates_pos == std::string::npos) {
        SetError(error_details, 503, "Gemini response missing 'candidates' for GetSummary.", "AI service returned an unexpected response format.");
        return "";
    }
    size_t text_field_pos = http_response.body.find("\"text\": \"", candidates_pos);
    if (text_field_pos != std::string::npos) {
        text_field_pos += 9;
        size_t text_end_pos = http_response.body.find("\"", text_field_pos);
        if (text_end_pos != std::string::npos) {
            std::string summary = http_response.body.substr(text_field_pos, text_end_pos - text_field_pos);
            return summary;
        }
    }

    SetError(error_details, 503, "Failed to parse summary text from Gemini response for GetSummary.", "AI service returned an unexpected response format.");
    return "";
}

// TODO: Replace manual JSON construction and parsing with a proper JSON library.
std::string GeminiTextAdapter::TranslateText(
    const std::string& text,
    const std::string& source_lang_code,
    const std::string& target_lang_code,
    const dashaibrowser::ipc::UserPreferences& prefs,
    dashaibrowser::ipc::ErrorDetails* error_details) {

    SetError(error_details, 0, "", "");
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

    std::cout << "GeminiTextAdapter::TranslateText: Requesting translation for (first 50 chars): \""
              << text.substr(0, 50) << "...\" from " << source_lang_code
              << " to " << target_lang_code << std::endl;

    std::string prompt_instruction = "Translate the following text";
    if (!source_lang_code.empty() && source_lang_code != "auto") {
        prompt_instruction += " from " + EscapeJsonString(source_lang_code); // Escape lang codes too if they could be tricky
    }
    prompt_instruction += " to " + EscapeJsonString(target_lang_code) + ". The text to translate is: ";

    std::string escaped_text_to_translate = EscapeJsonString(text);

    std::string request_body =
        "{\n"
        "  \"contents\": [\n"
        "    {\n"
        "      \"role\": \"user\",\n"
        "      \"parts\": [\n"
        "        { \"text\": \"" + EscapeJsonString(prompt_instruction) + escaped_text_to_translate + "\" }\n"
        "      ]\n"
        "    }\n"
        "  ],\n"
        "  \"generationConfig\": {\n"
        "    \"responseMimeType\": \"text/plain\"\n"
        "  }\n"
        "}";

    std::vector<std::string> headers = {"Content-Type: application/json"};
    std::string url = config_.api_endpoint_translate;
     if (url.find('?') == std::string::npos) {
        url += "?key=" + config_.api_key;
    } else {
        url += "&key=" + config_.api_key;
    }

    // std::cout << "GeminiTextAdapter::TranslateText: URL: " << url << std::endl; // Verbose
    // std::cout << "GeminiTextAdapter::TranslateText: Request Body: " << request_body << std::endl; // Verbose

    utils::HttpResponse http_response = http_client_->Post(
        url, request_body, headers, config_.timeout_ms
    );

    // std::cout << "GeminiTextAdapter::TranslateText: Response Status: " << http_response.status_code << std::endl; // Verbose
    // std::cout << "GeminiTextAdapter::TranslateText: Response Body: " << http_response.body << std::endl; // Verbose

    if (!http_response.IsSuccess()) {
        std::string err_msg = "Gemini API HTTP request failed for translation. Status: " + std::to_string(http_response.status_code);
        if (!http_response.error_message.empty()) {
             err_msg += ". Network Error: " + http_response.error_message;
        } else if (!http_response.body.empty()) {
            size_t err_msg_start = http_response.body.find("\"message\": \"");
            if (err_msg_start != std::string::npos) {
                err_msg_start += 12;
                size_t err_msg_end = http_response.body.find("\"", err_msg_start);
                if (err_msg_end != std::string::npos) {
                    err_msg += ". API Message: " + http_response.body.substr(err_msg_start, err_msg_end - err_msg_start);
                }
            } else {
                 err_msg += ". Body: " + http_response.body.substr(0,100) + "...";
            }
        }
        SetError(error_details, http_response.status_code == 0 ? 504 : static_cast<int32_t>(http_response.status_code),
                 err_msg, "AI service communication error for translation.");
        return "";
    }

    size_t candidates_pos = http_response.body.find("\"candidates\":");
    if (candidates_pos == std::string::npos) {
        SetError(error_details, 503, "Gemini translation response missing 'candidates'.", "AI service returned an unexpected response format for translation.");
        return "";
    }
    size_t text_field_pos = http_response.body.find("\"text\": \"", candidates_pos);
    if (text_field_pos != std::string::npos) {
        text_field_pos += 9;
        size_t text_end_pos = http_response.body.find("\"", text_field_pos);
        if (text_end_pos != std::string::npos) {
            std::string translated_text = http_response.body.substr(text_field_pos, text_end_pos - text_field_pos);
            return translated_text;
        }
    }

    SetError(error_details, 503, "Failed to parse translated text from Gemini response.", "AI service returned an unexpected response format for translation.");
    return "";
}

// TODO: Replace manual JSON construction and parsing with a proper JSON library.
std::string GeminiTextAdapter::GenerateText(
    const std::string& prompt,
    const dashaibrowser::ipc::UserPreferences& prefs,
    dashaibrowser::ipc::ErrorDetails* error_details) {

    SetError(error_details, 0, "", ""); // Clear previous errors
    if (!initialized_) {
        SetError(error_details, 500, "Adapter not initialized.", "Service configuration error.");
        return "";
    }
    if (prompt.empty()) {
        SetError(error_details, 400, "Input prompt is empty for text generation.", "Cannot generate text from empty prompt.");
        return "";
    }

    std::cout << "GeminiTextAdapter::GenerateText: Requesting text generation for prompt (first 50 chars): \""
              << prompt.substr(0, 50) << "...\"" << std::endl;

    std::string escaped_prompt = EscapeJsonString(prompt);
    std::string request_body =
        "{\n"
        "  \"contents\": [\n"
        "    {\n"
        "      \"role\": \"user\",\n"
        "      \"parts\": [\n"
        "        { \"text\": \"" + escaped_prompt + "\" }\n"
        "      ]\n"
        "    }\n"
        "  ],\n"
        "  \"generationConfig\": {\n"
        "    \"responseMimeType\": \"text/plain\"\n"
        // TODO: Add other generation configs from prefs if applicable (temperature, max tokens etc.)
        "  }\n"
        "}";

    std::vector<std::string> headers = {"Content-Type: application/json"};

    std::string url = config_.api_endpoint_generate_text; // Use specific endpoint for text gen
    if (url.find('?') == std::string::npos) {
        url += "?key=" + config_.api_key;
    } else {
        url += "&key=" + config_.api_key;
    }

    // std::cout << "GeminiTextAdapter::GenerateText: URL: " << url << std::endl; // Verbose
    // std::cout << "GeminiTextAdapter::GenerateText: Request Body: " << request_body << std::endl; // Verbose

    utils::HttpResponse http_response = http_client_->Post(
        url, request_body, headers, config_.timeout_ms
    );

    // std::cout << "GeminiTextAdapter::GenerateText: Response Status: " << http_response.status_code << std::endl; // Verbose
    // std::cout << "GeminiTextAdapter::GenerateText: Response Body: " << http_response.body << std::endl; // Verbose

    if (!http_response.IsSuccess()) {
        std::string err_msg = "Gemini API HTTP request failed for GenerateText. Status: " + std::to_string(http_response.status_code);
        if (!http_response.error_message.empty()) {
             err_msg += ". Network Error: " + http_response.error_message;
        } else if (!http_response.body.empty()) {
            size_t err_msg_start = http_response.body.find("\"message\": \"");
            if (err_msg_start != std::string::npos) {
                err_msg_start += 12;
                size_t err_msg_end = http_response.body.find("\"", err_msg_start);
                if (err_msg_end != std::string::npos) {
                    err_msg += ". API Message: " + http_response.body.substr(err_msg_start, err_msg_end - err_msg_start);
                }
            } else {
                 err_msg += ". Body: " + http_response.body.substr(0,100) + "...";
            }
        }
        SetError(error_details, http_response.status_code == 0 ? 504 : static_cast<int32_t>(http_response.status_code),
                 err_msg, "AI service communication error for text generation.");
        return "";
    }

    size_t candidates_pos = http_response.body.find("\"candidates\":");
    if (candidates_pos == std::string::npos) {
        SetError(error_details, 503, "Gemini response missing 'candidates' for GenerateText.", "AI service returned an unexpected response format.");
        return "";
    }
    size_t text_field_pos = http_response.body.find("\"text\": \"", candidates_pos);
    if (text_field_pos != std::string::npos) {
        text_field_pos += 9;
        size_t text_end_pos = http_response.body.find("\"", text_field_pos);
        if (text_end_pos != std::string::npos) {
            std::string generated_text = http_response.body.substr(text_field_pos, text_end_pos - text_field_pos);
            // TODO: Unescape JSON string
            return generated_text;
        }
    }

    SetError(error_details, 503, "Failed to parse generated text from Gemini response for GenerateText.", "AI service returned an unexpected response format.");
    return "";
}

} // namespace adapters
} // namespace asol
} // namespace dashaibrowser
