#include "asol/cpp/utils/curl_http_client.h"
#include <iostream> // For error logging
#include <algorithm> // For std::remove_if, for header parsing later if needed

namespace dashaibrowser {
namespace asol {
namespace utils {

bool CurlHttpClient::global_curl_initialized_ = false;

// Static callback function for libcurl to write received data
size_t CurlHttpClient::WriteCallback(void* contents, size_t size, size_t nmemb, void* userdata) {
    size_t real_size = size * nmemb;
    std::string* read_buffer = static_cast<std::string*>(userdata);
    if (read_buffer == nullptr) {
        // Should not happen if userdata is set correctly
        return 0;
    }
    read_buffer->append(static_cast<char*>(contents), real_size);
    return real_size;
}

bool CurlHttpClient::GlobalInit() {
    if (!global_curl_initialized_) {
        CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
        if (res != CURLE_OK) {
            std::cerr << "CurlHttpClient::GlobalInit: curl_global_init() failed: "
                      << curl_easy_strerror(res) << std::endl;
            return false;
        }
        global_curl_initialized_ = true;
        std::cout << "CurlHttpClient: libcurl global state initialized." << std::endl;
    }
    return true;
}

void CurlHttpClient::GlobalCleanup() {
    if (global_curl_initialized_) {
        curl_global_cleanup();
        global_curl_initialized_ = false;
        std::cout << "CurlHttpClient: libcurl global state cleaned up." << std::endl;
    }
}

CurlHttpClient::CurlHttpClient() {
    if (!global_curl_initialized_) {
        // This is not ideal; GlobalInit should be called explicitly at application start.
        // Throwing an exception or ensuring GlobalInit is called by main() is better.
        std::cerr << "CurlHttpClient Error: libcurl global state not initialized. Call GlobalInit() first." << std::endl;
        // Attempt to initialize, but this is risky if other threads are also doing it.
        if (!GlobalInit()) {
             // If still fails, curl_handle_ will be nullptr and Post will fail.
        }
    }

    curl_handle_ = curl_easy_init();
    if (!curl_handle_) {
        std::cerr << "CurlHttpClient: curl_easy_init() failed." << std::endl;
        // No good recovery here, Post() will have to handle nullptr curl_handle_.
    } else {
        std::cout << "CurlHttpClient: Instance created, curl_easy_init() successful." << std::endl;
    }
}

CurlHttpClient::~CurlHttpClient() {
    if (curl_handle_) {
        curl_easy_cleanup(curl_handle_);
        curl_handle_ = nullptr;
    }
    std::cout << "CurlHttpClient: Instance destroyed." << std::endl;
    // Note: GlobalCleanup() should be called by the application, not per instance.
}

HttpResponse CurlHttpClient::Post(const std::string& url,
                                  const std::string& request_body,
                                  const std::vector<std::string>& headers,
                                  int timeout_ms) {
    HttpResponse response;
    if (!curl_handle_) {
        response.error_message = "cURL handle not initialized.";
        response.status_code = 0; // Indicate internal error
        std::cerr << "CurlHttpClient::Post Error: cURL handle not initialized." << std::endl;
        return response;
    }

    // Reset the handle for reuse
    curl_easy_reset(curl_handle_);

    std::string read_buffer;
    struct curl_slist* header_list = nullptr;

    // Set URL
    curl_easy_setopt(curl_handle_, CURLOPT_URL, url.c_str());

    // Set POST method and request body
    curl_easy_setopt(curl_handle_, CURLOPT_POSTFIELDS, request_body.c_str());
    // If request_body can contain null bytes, use CURLOPT_POSTFIELDSIZE
    // curl_easy_setopt(curl_handle_, CURLOPT_POSTFIELDSIZE, (long)request_body.length());

    // Set headers
    for (const auto& header : headers) {
        header_list = curl_slist_append(header_list, header.c_str());
    }
    if (header_list) {
        curl_easy_setopt(curl_handle_, CURLOPT_HTTPHEADER, header_list);
    }

    // Set write callback to capture response body
    curl_easy_setopt(curl_handle_, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl_handle_, CURLOPT_WRITEDATA, &read_buffer);

    // Set timeout
    if (timeout_ms > 0) {
        curl_easy_setopt(curl_handle_, CURLOPT_TIMEOUT_MS, (long)timeout_ms);
    }

    // Enable following redirects (optional, but often useful)
    // curl_easy_setopt(curl_handle_, CURLOPT_FOLLOWLOCATION, 1L);

    // For HTTPS, you might need to set CA paths if not using system defaults:
    // curl_easy_setopt(curl_handle_, CURLOPT_CAINFO, "/path/to/ca-bundle.crt");
    // Or disable peer verification for testing (NOT RECOMMENDED FOR PRODUCTION):
    // curl_easy_setopt(curl_handle_, CURLOPT_SSL_VERIFYPEER, 0L);
    // curl_easy_setopt(curl_handle_, CURLOPT_SSL_VERIFYHOST, 0L);

    // Perform the request
    CURLcode res = curl_easy_perform(curl_handle_);

    if (res != CURLE_OK) {
        response.error_message = std::string("curl_easy_perform() failed: ") + curl_easy_strerror(res);
        response.status_code = 0; // Indicate cURL level error
        std::cerr << "CurlHttpClient::Post Error: " << response.error_message << std::endl;
    } else {
        // Get HTTP response code
        long http_code = 0;
        curl_easy_getinfo(curl_handle_, CURLINFO_RESPONSE_CODE, &http_code);
        response.status_code = http_code;
        response.body = read_buffer;
        // TODO: Populate response.headers if HeaderCallback is implemented and used.
    }

    // Clean up header list
    if (header_list) {
        curl_slist_free_all(header_list);
    }

    return response;
}

} // namespace utils
} // namespace asol
} // namespace dashaibrowser
