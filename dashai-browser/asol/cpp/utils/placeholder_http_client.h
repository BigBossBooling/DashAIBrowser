#ifndef DASHAI_BROWSER_ASOL_CPP_UTILS_PLACEHOLDER_HTTP_CLIENT_H_
#define DASHAI_BROWSER_ASOL_CPP_UTILS_PLACEHOLDER_HTTP_CLIENT_H_

#include "asol/cpp/utils/network_request_util.h" // For IHttpClient, HttpResponse

namespace dashaibrowser {
namespace asol {
namespace utils {

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

#endif // DASHAI_BROWSER_ASOL_CPP_UTILS_PLACEHOLDER_HTTP_CLIENT_H_
