#ifndef DASHAI_BROWSER_ASOL_CPP_UTILS_CURL_HTTP_CLIENT_H_
#define DASHAI_BROWSER_ASOL_CPP_UTILS_CURL_HTTP_CLIENT_H_

#include "asol/cpp/utils/network_request_util.h" // For IHttpClient, HttpResponse
#include <curl/curl.h> // For libcurl
#include <string>
#include <vector>
#include <memory> // For std::unique_ptr

namespace dashaibrowser {
namespace asol {
namespace utils {

// Concrete implementation of IHttpClient using libcurl.
class CurlHttpClient : public IHttpClient {
public:
    CurlHttpClient();
    ~CurlHttpClient() override;

    // Initializes libcurl global state. Should be called once per application.
    // Returns true on success, false on failure.
    static bool GlobalInit();

    // Cleans up libcurl global state. Should be called once per application on exit.
    static void GlobalCleanup();

    HttpResponse Post(const std::string& url,
                      const std::string& request_body,
                      const std::vector<std::string>& headers,
                      int timeout_ms = 10000) override;

private:
    // libcurl write callback function.
    // userdata is expected to be a pointer to a std::string.
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userdata);

    // libcurl header callback function.
    // userdata is expected to be a pointer to a std::map<std::string, std::string>.
    // static size_t HeaderCallback(char* buffer, size_t size, size_t nitems, void* userdata);
    // For simplicity, we might not parse all response headers initially,
    // but focus on status code and body.

    CURL* curl_handle_ = nullptr; // Re-usable curl easy handle
    static bool global_curl_initialized_;
};

} // namespace utils
} // namespace asol
} // namespace dashaibrowser

#endif // DASHAI_BROWSER_ASOL_CPP_UTILS_CURL_HTTP_CLIENT_H_
