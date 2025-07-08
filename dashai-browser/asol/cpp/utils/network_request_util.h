#ifndef DASHAI_BROWSER_ASOL_CPP_UTILS_NETWORK_REQUEST_UTIL_H_
#define DASHAI_BROWSER_ASOL_CPP_UTILS_NETWORK_REQUEST_UTIL_H_

#include <string>
#include <vector>
#include <map>

namespace dashaibrowser {
namespace asol {
namespace utils {

// Basic structure for an HTTP response.
struct HttpResponse {
    long status_code = 0; // e.g., 200, 404, 500
    std::string body;
    std::map<std::string, std::string> headers;
    std::string error_message; // For network-level errors (e.g., connection failed)

    bool IsSuccess() const {
        return status_code >= 200 && status_code < 300 && error_message.empty();
    }
};

// Interface for a simple HTTP client.
// This allows for different implementations (e.g., curl-based, Chromium network stack based).
class IHttpClient {
public:
    virtual ~IHttpClient() = default;

    // Performs an HTTP POST request.
    // url: The target URL.
    // request_body: The body of the POST request.
    // headers: A list of header strings, e.g., "Content-Type: application/json".
    // timeout_ms: Request timeout in milliseconds.
    virtual HttpResponse Post(const std::string& url,
                              const std::string& request_body,
                              const std::vector<std::string>& headers,
                              int timeout_ms = 10000) = 0;

    // Could add Get, Put, Delete etc. as needed
    // virtual HttpResponse Get(const std::string& url,
    //                          const std::vector<std::string>& headers,
    //                          int timeout_ms = 10000) = 0;
};


// Placeholder implementation of IHttpClient.
// This will not make actual network calls but simulate them.
class PlaceholderHttpClient : public IHttpClient {
public:
    PlaceholderHttpClient();
    ~PlaceholderHttpClient() override;

    HttpResponse Post(const std::string& url,
                      const std::string& request_body,
                      const std::vector<std::string>& headers,
                      int timeout_ms = 10000) override;
};

} // namespace utils
} // namespace asol
} // namespace dashaibrowser

#endif // DASHAI_BROWSER_ASOL_CPP_UTILS_NETWORK_REQUEST_UTIL_H_
