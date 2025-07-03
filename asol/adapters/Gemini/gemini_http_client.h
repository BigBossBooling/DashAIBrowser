// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASOL_ADAPTERS_GEMINI_GEMINI_HTTP_CLIENT_H_
#define ASOL_ADAPTERS_GEMINI_GEMINI_HTTP_CLIENT_H_

#include <memory>
#include <string>

#include "asol/adapters/gemini/gemini_types.h"
#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "third_party/nlohmann_json/json.hpp"

namespace asol {
namespace adapters {
namespace gemini {

// GeminiHttpClient handles HTTP communication with the Gemini API.
class GeminiHttpClient {
 public:
  // Callback for asynchronous responses
  using ResponseCallback = base::OnceCallback<void(const GeminiResponse&)>;

  explicit GeminiHttpClient(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~GeminiHttpClient();

  // Disable copy and assign
  GeminiHttpClient(const GeminiHttpClient&) = delete;
  GeminiHttpClient& operator=(const GeminiHttpClient&) = delete;

  // Set the API key for authentication
  void SetApiKey(const std::string& api_key);

  // Set the API endpoint
  void SetApiEndpoint(const std::string& api_endpoint);

  // Send a request to the Gemini API synchronously
  GeminiResponse SendRequest(const nlohmann::json& request_payload,
                            const std::string& model_name);

  // Send a request to the Gemini API asynchronously
  void SendRequestAsync(const nlohmann::json& request_payload,
                       const std::string& model_name,
                       ResponseCallback callback);
                       
  // Send a streaming request to the Gemini API
  void SendStreamingRequest(const nlohmann::json& request_payload,
                           const std::string& model_name,
                           StreamingResponseCallback callback);

 private:
  // Process the response from the API
  GeminiResponse ProcessResponse(const std::string& response_body);

  // Handle the completion of an asynchronous request
  void OnRequestComplete(ResponseCallback callback,
                        std::unique_ptr<network::SimpleURLLoader> loader,
                        std::unique_ptr<std::string> response_body);

  // Create a URL for the API request
  GURL CreateRequestUrl(const std::string& model_name);

  // URL loader factory for making HTTP requests
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;

  // API key for authentication
  std::string api_key_;

  // API endpoint URL
  std::string api_endpoint_ = "https://generativelanguage.googleapis.com/v1beta/models/";

  // For generating weak pointers to this
  base::WeakPtrFactory<GeminiHttpClient> weak_ptr_factory_{this};
};

}  // namespace gemini
}  // namespace adapters
}  // namespace asol

#endif  // ASOL_ADAPTERS_GEMINI_GEMINI_HTTP_CLIENT_H_