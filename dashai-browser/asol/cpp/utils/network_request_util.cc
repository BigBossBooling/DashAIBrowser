#include "asol/cpp/utils/network_request_util.h"
#include <iostream> // For placeholder logging

namespace dashaibrowser {
namespace asol {
namespace utils {

PlaceholderHttpClient::PlaceholderHttpClient() {
    std::cout << "PlaceholderHttpClient: Instance created." << std::endl;
}

PlaceholderHttpClient::~PlaceholderHttpClient() {
    std::cout << "PlaceholderHttpClient: Instance destroyed." << std::endl;
}

HttpResponse PlaceholderHttpClient::Post(const std::string& url,
                                         const std::string& request_body,
                                         const std::vector<std::string>& headers,
                                         int timeout_ms) {
    std::cout << "PlaceholderHttpClient::Post:" << std::endl;
    std::cout << "  URL: " << url << std::endl;
    std::cout << "  Timeout (ms): " << timeout_ms << std::endl;
    std::cout << "  Headers:" << std::endl;
    for (const auto& header : headers) {
        std::cout << "    " << header << std::endl;
    }
    std::cout << "  Body: " << request_body.substr(0, 200) << (request_body.length() > 200 ? "..." : "") << std::endl;

    HttpResponse response;
    response.status_code = 200; // Assume success by default for placeholder

    // Simulate different responses based on URL or request body content for basic testing
    // This logic is similar to the one in GeminiTextAdapter's nested handler,
    // demonstrating that this utility will be used by adapters.
    if (url.find("summarize") != std::string::npos || url.find("generateContent") != std::string::npos ) {
        if (request_body.find("error_test_network_failure") != std::string::npos) {
            response.status_code = 0; // Simulate network error
            response.error_message = "Simulated network connection failed.";
            std::cout << "  -> Simulating network failure." << std::endl;
        } else if (request_body.find("error_test_timeout") != std::string::npos) {
            response.status_code = 0; // Simulate network error (timeout)
            response.error_message = "Simulated request timeout.";
            std::cout << "  -> Simulating request timeout." << std::endl;
        } else if (request_body.find("error_test_401_unauthorized") != std::string::npos) {
            response.status_code = 401;
            response.body = R"({"error": {"code": 401, "message": "Simulated API Unauthorized."}})";
            std::cout << "  -> Simulating 401 Unauthorized." << std::endl;
        } else if (request_body.find("error_test") != std::string::npos) {
            response.status_code = 400; // Bad request from API
            response.body = R"({"error": {"code": 400, "message": "Simulated API error from Gemini due to bad request."}})";
            std::cout << "  -> Simulating 400 Bad Request (API error)." << std::endl;
        } else {
            // Dummy JSON response for summarization/generation
            response.body = R"({"candidates": [{"content": {"parts": [{"text": "This is a simulated summary via PlaceholderHttpClient."}]}}]})";
            std::cout << "  -> Simulating successful summary/generation response." << std::endl;
        }
    } else if (url.find("translate") != std::string::npos) { // Assuming a different endpoint or param for translate
        // Dummy JSON response for translation
        response.body = R"({"candidates": [{"content": {"parts": [{"text": "This is a simulated translation via PlaceholderHttpClient."}]}}]})";
        std::cout << "  -> Simulating successful translation response." << std::endl;
    } else {
        response.status_code = 500;
        response.body = R"({"error": {"code": 500, "message": "Unknown simulated request type for PlaceholderHttpClient."}})";
        std::cout << "  -> Simulating unknown request type (500 error)." << std::endl;
    }

    response.headers["Content-Type"] = "application/json"; // Dummy header
    return response;
}

} // namespace utils
} // namespace asol
} // namespace dashaibrowser
