#include "asol/cpp/asol_service_impl.h"
#include <iostream> // For placeholder logging

namespace dashaibrowser {
namespace asol {

AsolServiceImpl::AsolServiceImpl() {
    std::cout << "AsolServiceImpl: Instance created." << std::endl;
    if (!InitializeAdapters()) {
        // Handle adapter initialization failure, e.g., by logging or throwing.
        // For now, just log. The service methods will check `adapters_initialized_`.
        std::cerr << "AsolServiceImpl: Failed to initialize AI adapters!" << std::endl;
    }
}

AsolServiceImpl::~AsolServiceImpl() {
    std::cout << "AsolServiceImpl: Instance destroyed." << std::endl;
}

bool AsolServiceImpl::InitializeAdapters() {
    // Create and initialize the Gemini adapter.
    // In a real application, config would come from a file or environment variables.
    gemini_adapter_ = std::make_unique<adapters::GeminiTextAdapter>(
        std::make_unique<utils::PlaceholderHttpClient>() // Inject placeholder HTTP client
    );

    adapters::GeminiAdapterConfig gemini_config;
    // TODO: Populate gemini_config.api_key from a secure source.
    // For now, it can be empty for PlaceholderHttpClient.
    gemini_config.api_key = "YOUR_GEMINI_API_KEY_HERE"; // Placeholder
    gemini_config.api_endpoint_summarize = "https://generativelanguage.googleapis.com/v1beta/models/gemini-pro:generateContent_summarize_placeholder";
    gemini_config.api_endpoint_translate = "https://generativelanguage.googleapis.com/v1beta/models/gemini-pro:generateContent_translate_placeholder";

    if (!gemini_adapter_->Initialize(gemini_config)) {
        std::cerr << "AsolServiceImpl: Failed to initialize GeminiTextAdapter." << std::endl;
        gemini_adapter_.reset(); // Release if initialization failed
        adapters_initialized_ = false;
        return false;
    }

    // Initialize other adapters here if any.
    adapters_initialized_ = true;
    std::cout << "AsolServiceImpl: AI Adapters initialized successfully." << std::endl;
    return true;
}


void AsolServiceImpl::SetError(ipc::ErrorDetails* error_details,
                               int32_t code,
                               const std::string& message,
                               const std::string& user_message) {
    if (error_details) {
        error_details->set_error_code(code);
        error_details->set_error_message(message);
        if (!user_message.empty()) {
            error_details->set_user_facing_message(user_message);
        }
    }
}

::grpc::Status AsolServiceImpl::GetSummary(
    ::grpc::ServerContext* context,
    const ipc::SummaryRequest* request,
    ipc::SummaryResponse* response) {

    std::cout << "AsolServiceImpl::GetSummary: Received request ID "
              << request->request_id() << " for text: \""
              << request->original_text().substr(0, 50) << "...\"" << std::endl;

    response->set_request_id(request->request_id());

    if (!adapters_initialized_ || !gemini_adapter_) {
        response->set_success(false);
        SetError(response->mutable_error_details(), 500, "AI adapter not available.", "Service not properly configured.");
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, "AI adapter not available.");
    }

    if (request->original_text().empty()) {
        response->set_success(false);
        SetError(response->mutable_error_details(), 1001, "Original text is empty.", "Cannot summarize empty text.");
        return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "Original text is empty.");
    }

    ipc::ErrorDetails adapter_error;
    std::string summary = gemini_adapter_->GetSummary(
        request->original_text(),
        request->preferences(),
        &adapter_error
    );

    if (adapter_error.error_code() != 0 || summary.empty()) {
        response->set_success(false);
        // Copy error details from adapter_error to response->mutable_error_details()
        response->mutable_error_details()->CopyFrom(adapter_error);
        if (response->error_details().error_message().empty()) { // Ensure some message is there
             SetError(response->mutable_error_details(), adapter_error.error_code() == 0 ? 500 : adapter_error.error_code(),
                     "Adapter failed to produce summary.", "AI service could not complete the request.");
        }
        std::cerr << "AsolServiceImpl::GetSummary Error from adapter: " << response->error_details().error_message() << std::endl;
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, response->error_details().error_message());
    }

    response->set_success(true);
    response->set_summarized_text(summary);

    std::cout << "AsolServiceImpl::GetSummary: Sending response for ID "
              << response->request_id() << ", Success: " << response->success() << std::endl;

    return ::grpc::Status::OK;
}

::grpc::Status AsolServiceImpl::TranslateText(
    ::grpc::ServerContext* context,
    const ipc::TranslationRequest* request,
    ipc::TranslationResponse* response) {

    std::cout << "AsolServiceImpl::TranslateText: Received request ID "
              << request->request_id() << " to translate \""
              << request->text_to_translate().substr(0, 50) << "...\" from "
              << request->source_language_code() << " to " << request->target_language_code()
              << std::endl;

    response->set_request_id(request->request_id());

    if (!adapters_initialized_ || !gemini_adapter_) {
        response->set_success(false);
        SetError(response->mutable_error_details(), 500, "AI adapter not available.", "Service not properly configured.");
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, "AI adapter not available.");
    }

    if (request->text_to_translate().empty()) {
        response->set_success(false);
        SetError(response->mutable_error_details(), 1001, "Text to translate is empty.", "Cannot translate empty text.");
        return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "Text to translate is empty.");
    }
    if (request->target_language_code().empty()) {
        response->set_success(false);
        SetError(response->mutable_error_details(), 1002, "Target language code is missing.", "Please specify a target language.");
        return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "Target language code is missing.");
    }

    ipc::ErrorDetails adapter_error;
    std::string translated_text = gemini_adapter_->TranslateText(
        request->text_to_translate(),
        request->source_language_code(),
        request->target_language_code(),
        request->preferences(),
        &adapter_error
    );

    if (adapter_error.error_code() != 0 || translated_text.empty()) {
        response->set_success(false);
        response->mutable_error_details()->CopyFrom(adapter_error);
         if (response->error_details().error_message().empty()) {
             SetError(response->mutable_error_details(), adapter_error.error_code() == 0 ? 500 : adapter_error.error_code(),
                     "Adapter failed to produce translation.", "AI service could not complete the request.");
        }
        std::cerr << "AsolServiceImpl::TranslateText Error from adapter: " << response->error_details().error_message() << std::endl;
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, response->error_details().error_message());
    }

    response->set_success(true);
    response->set_translated_text(translated_text);
    // For placeholder, we assume Gemini doesn't explicitly return detected source lang,
    // so we'll just pass through what was requested or a default.
    // A real adapter would populate this if the AI service provides it.
    response->set_detected_source_language(
        request->source_language_code() == "auto" ? "en_simulated_detection" : request->source_language_code()
    );

    std::cout << "AsolServiceImpl::TranslateText: Sending response for ID "
              << response->request_id() << ", Success: " << response->success() << std::endl;

    return ::grpc::Status::OK;
}

}  // namespace asol
}  // namespace dashaibrowser
