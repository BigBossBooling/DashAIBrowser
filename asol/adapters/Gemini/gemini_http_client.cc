// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "asol/adapters/gemini/gemini_http_client.h"

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "net/base/url_util.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/resource_request.h"

namespace asol {
namespace adapters {
namespace gemini {

GeminiHttpClient::GeminiHttpClient(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : url_loader_factory_(std::move(url_loader_factory)) {
  DCHECK(url_loader_factory_);
}

GeminiHttpClient::~GeminiHttpClient() = default;

void GeminiHttpClient::SetApiKey(const std::string& api_key) {
  api_key_ = api_key;
}

void GeminiHttpClient::SetApiEndpoint(const std::string& api_endpoint) {
  api_endpoint_ = api_endpoint;
}

GeminiResponse GeminiHttpClient::SendRequest(
    const nlohmann::json& request_payload,
    const std::string& model_name) {
  if (api_key_.empty()) {
    GeminiResponse response;
    response.success = false;
    response.error_message = "API key not set";
    return response;
  }

  // Create the request URL
  GURL url = CreateRequestUrl(model_name);
  if (!url.is_valid()) {
    GeminiResponse response;
    response.success = false;
    response.error_message = "Invalid API URL";
    return response;
  }

  // Create the URL loader
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = url;
  resource_request->method = "POST";
  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  resource_request->headers.SetHeader("Content-Type", "application/json");

  // Convert the request payload to a string
  std::string request_body = request_payload.dump();

  // Create the URL loader
  auto loader = network::SimpleURLLoader::Create(
      std::move(resource_request), network::SimpleURLLoader::BYPASS_CACHE);
  loader->AttachStringForUpload(request_body, "application/json");

  // Send the request
  std::unique_ptr<std::string> response_body;
  auto download_result = loader->DownloadToString(
      url_loader_factory_.get(),
      [&response_body](std::unique_ptr<std::string> downloaded_data) {
        response_body = std::move(downloaded_data);
      });

  // Check for network errors
  if (download_result != net::OK) {
    GeminiResponse response;
    response.success = false;
    response.error_message = "Network error: " + std::to_string(download_result);
    return response;
  }

  // Check for HTTP errors
  int response_code = loader->ResponseInfo() && loader->ResponseInfo()->headers
                          ? loader->ResponseInfo()->headers->response_code()
                          : 0;
  if (response_code != net::HTTP_OK) {
    GeminiResponse response;
    response.success = false;
    response.error_message =
        "HTTP error: " + std::to_string(response_code) +
        (response_body ? ": " + *response_body : "");
    return response;
  }

  // Process the response
  if (!response_body) {
    GeminiResponse response;
    response.success = false;
    response.error_message = "Empty response from API";
    return response;
  }

  return ProcessResponse(*response_body);
}

void GeminiHttpClient::SendRequestAsync(
    const nlohmann::json& request_payload,
    const std::string& model_name,
    ResponseCallback callback) {
  if (api_key_.empty()) {
    GeminiResponse response;
    response.success = false;
    response.error_message = "API key not set";
    std::move(callback).Run(response);
    return;
  }

  // Create the request URL
  GURL url = CreateRequestUrl(model_name);
  if (!url.is_valid()) {
    GeminiResponse response;
    response.success = false;
    response.error_message = "Invalid API URL";
    std::move(callback).Run(response);
    return;
  }

  // Create the URL loader
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = url;
  resource_request->method = "POST";
  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  resource_request->headers.SetHeader("Content-Type", "application/json");

  // Convert the request payload to a string
  std::string request_body = request_payload.dump();

  // Create the URL loader
  auto loader = network::SimpleURLLoader::Create(
      std::move(resource_request), network::SimpleURLLoader::BYPASS_CACHE);
  loader->AttachStringForUpload(request_body, "application/json");

  // Send the request
  auto* raw_loader = loader.get();
  raw_loader->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&GeminiHttpClient::OnRequestComplete,
                    weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                    std::move(loader)));
}

GeminiResponse GeminiHttpClient::ProcessResponse(
    const std::string& response_body) {
  GeminiResponse response;

  try {
    // Parse the JSON response
    auto json_response = nlohmann::json::parse(response_body);

    // Check for error
    if (json_response.contains("error")) {
      response.success = false;
      if (json_response["error"].contains("message")) {
        response.error_message = json_response["error"]["message"];
      } else {
        response.error_message = "Unknown API error";
      }
      return response;
    }

    // Extract the generated text
    if (json_response.contains("candidates") && json_response["candidates"].is_array() &&
        !json_response["candidates"].empty()) {
      auto& candidate = json_response["candidates"][0];
      if (candidate.contains("content") && candidate["content"].contains("parts") &&
          candidate["content"]["parts"].is_array() && !candidate["content"]["parts"].empty()) {
        auto& part = candidate["content"]["parts"][0];
        if (part.contains("text")) {
          response.text = part["text"];
          response.success = true;
        }
      }
    }

    // If we couldn't extract the text, set an error
    if (response.text.empty() && response.error_message.empty()) {
      response.success = false;
      response.error_message = "Could not extract text from response";
    }

    // Extract metadata
    if (json_response.contains("usage") && json_response["usage"].is_object()) {
      auto& usage = json_response["usage"];
      if (usage.contains("promptTokenCount")) {
        response.metadata.push_back(
            {"prompt_tokens", std::to_string(usage["promptTokenCount"].get<int>())});
      }
      if (usage.contains("candidatesTokenCount")) {
        response.metadata.push_back(
            {"completion_tokens", std::to_string(usage["candidatesTokenCount"].get<int>())});
      }
      if (usage.contains("totalTokenCount")) {
        response.metadata.push_back(
            {"total_tokens", std::to_string(usage["totalTokenCount"].get<int>())});
      }
    }

    // Add model info if available
    if (json_response.contains("model")) {
      response.metadata.push_back({"model", json_response["model"]});
    }

  } catch (const std::exception& e) {
    response.success = false;
    response.error_message = "Failed to parse API response: ";
    response.error_message += e.what();
  }

  return response;
}

void GeminiHttpClient::OnRequestComplete(
    ResponseCallback callback,
    std::unique_ptr<network::SimpleURLLoader> loader,
    std::unique_ptr<std::string> response_body) {
  GeminiResponse response;

  // Check for network errors
  if (!loader->ResponseInfo()) {
    response.success = false;
    response.error_message = "Network error: No response info";
    std::move(callback).Run(response);
    return;
  }

  // Check for HTTP errors
  int response_code = loader->ResponseInfo()->headers
                          ? loader->ResponseInfo()->headers->response_code()
                          : 0;
  if (response_code != net::HTTP_OK) {
    response.success = false;
    response.error_message =
        "HTTP error: " + std::to_string(response_code) +
        (response_body ? ": " + *response_body : "");
    std::move(callback).Run(response);
    return;
  }

  // Process the response
  if (!response_body) {
    response.success = false;
    response.error_message = "Empty response from API";
    std::move(callback).Run(response);
    return;
  }

  // Process the response and invoke the callback
  response = ProcessResponse(*response_body);
  std::move(callback).Run(response);
}

void GeminiHttpClient::SendStreamingRequest(
    const nlohmann::json& request_payload,
    const std::string& model_name,
    StreamingResponseCallback callback) {
  if (api_key_.empty()) {
    GeminiResponse response;
    response.success = false;
    response.error_message = "API key not set";
    callback(response, true);  // true indicates this is the final response
    return;
  }

  // Create the request URL with streaming endpoint
  std::string url_str = api_endpoint_;
  if (!base::EndsWith(url_str, "/", base::CompareCase::SENSITIVE)) {
    url_str += "/";
  }
  url_str += model_name + ":streamGenerateContent";
  GURL url(url_str);
  
  if (!url.is_valid()) {
    GeminiResponse response;
    response.success = false;
    response.error_message = "Invalid API URL";
    callback(response, true);
    return;
  }
  
  // Add the API key as a query parameter
  url = net::AppendQueryParameter(url, "key", api_key_);

  // Create the URL loader
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = url;
  resource_request->method = "POST";
  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  resource_request->headers.SetHeader("Content-Type", "application/json");
  resource_request->headers.SetHeader("Accept", "text/event-stream");

  // Convert the request payload to a string
  std::string request_body = request_payload.dump();

  // Create the URL loader
  auto loader = network::SimpleURLLoader::Create(
      std::move(resource_request), network::SimpleURLLoader::BYPASS_CACHE);
  loader->AttachStringForUpload(request_body, "application/json");

  // In a real implementation, we would use a streaming parser to process
  // the server-sent events (SSE) from the API. For now, we'll simulate
  // a streaming response with a few chunks.
  
  // Simulate streaming response with 3 chunks
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](StreamingResponseCallback user_callback) {
            // First chunk
            GeminiResponse response;
            response.success = true;
            response.text = "This is the first ";
            response.is_partial = true;
            user_callback(response, false);
            
            // Second chunk
            response.text = "part of a simulated ";
            user_callback(response, false);
            
            // Final chunk
            response.text = "streaming response.";
            response.is_partial = false;
            response.metadata.push_back({"model", "gemini-pro"});
            response.metadata.push_back({"total_tokens", "15"});
            user_callback(response, true);
          },
          std::move(callback)));
}

GURL GeminiHttpClient::CreateRequestUrl(const std::string& model_name) {
  std::string url_str = api_endpoint_;
  if (!base::EndsWith(url_str, "/", base::CompareCase::SENSITIVE)) {
    url_str += "/";
  }
  url_str += model_name + ":generateContent";

  GURL url(url_str);
  if (!url.is_valid()) {
    return GURL();
  }

  // Add the API key as a query parameter
  return net::AppendQueryParameter(url, "key", api_key_);
}

}  // namespace gemini
}  // namespace adapters
}  // namespace asol