#include "asol/cpp/asol_service_impl.h"
#include <iostream> // For placeholder logging
#include <sstream>  // For constructing prompts with history

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
    // It will default to using CurlHttpClient.
    gemini_adapter_ = std::make_unique<adapters::GeminiTextAdapter>();

    adapters::GeminiAdapterConfig gemini_config;
    // TODO: Populate gemini_config.api_key from a secure source (e.g., env variable, config file).
    // The key "YOUR_GEMINI_API_KEY_HERE" is a placeholder and will not work.
    // For testing with the actual CurlHttpClient, a valid key needs to be set here or read from env.
    gemini_config.api_key = "YOUR_GEMINI_API_KEY_PLACEHOLDER"; // Placeholder, ensure this is clear

    gemini_config.api_endpoint_summarize = "https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash-latest:generateContent";
    gemini_config.api_endpoint_translate = "https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash-latest:generateContent";
    gemini_config.api_endpoint_generate_text = "https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash-latest:generateContent"; // Using same for general text

    if (!gemini_adapter_->Initialize(gemini_config)) {
        std::cerr << "AsolServiceImpl: Failed to initialize GeminiTextAdapter." << std::endl;
        gemini_adapter_.reset(); // Release if initialization failed
        adapters_initialized_ = false;
        return false;
    }

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

    if (adapter_error.error_code() != 0 || (summary.empty() && adapter_error.error_message().empty())) {
        response->set_success(false);
        response->mutable_error_details()->CopyFrom(adapter_error);
        if (response->error_details().error_message().empty()) {
             SetError(response->mutable_error_details(), adapter_error.error_code() == 0 ? 500 : adapter_error.error_code(),
                     "Adapter failed to produce summary and returned no error message.", "AI service could not complete the request.");
        }
        std::cerr << "AsolServiceImpl::GetSummary Error from adapter: (" << response->error_details().error_code()
                  << ") " << response->error_details().error_message() << std::endl;
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

    if (adapter_error.error_code() != 0 || (translated_text.empty() && adapter_error.error_message().empty())) {
        response->set_success(false);
        response->mutable_error_details()->CopyFrom(adapter_error);
         if (response->error_details().error_message().empty()) {
             SetError(response->mutable_error_details(), adapter_error.error_code() == 0 ? 500 : adapter_error.error_code(),
                     "Adapter failed to produce translation and returned no error message.", "AI service could not complete the request.");
        }
        std::cerr << "AsolServiceImpl::TranslateText Error from adapter: (" << response->error_details().error_code()
                  << ") " << response->error_details().error_message() << std::endl;
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, response->error_details().error_message());
    }

    response->set_success(true);
    response->set_translated_text(translated_text);
    response->set_detected_source_language(
        request->source_language_code() == "auto" ? "en_simulated_detection" : request->source_language_code()
    );

    std::cout << "AsolServiceImpl::TranslateText: Sending response for ID "
              << response->request_id() << ", Success: " << response->success() << std::endl;

    return ::grpc::Status::OK;
}

::grpc::Status AsolServiceImpl::ChatWithJules(
    ::grpc::ServerContext* context,
    const ipc::ConversationRequest* request,
    ipc::ConversationResponse* response) {

    std::cout << "AsolServiceImpl::ChatWithJules: Received request ID "
              << request->request_id() << " for session_id: " << request->session_id()
              << " User message: \"" << request->user_message().substr(0, 50) << "...\"" << std::endl;

    response->set_request_id(request->request_id());
    response->set_session_id(request->session_id());

    if (!adapters_initialized_ || !gemini_adapter_) {
        response->set_success(false);
        SetError(response->mutable_error_details(), 500, "AI adapter not available.", "Service not properly configured.");
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, "AI adapter not available.");
    }

    if (request->user_message().empty()) {
        response->set_success(false);
        SetError(response->mutable_error_details(), 1001, "User message is empty.", "Cannot chat with an empty message.");
        return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "User message is empty.");
    }

    // Construct prompt for Gemini
    // TODO: Implement more sophisticated history management and prompt engineering.
    std::ostringstream prompt_stream;
    prompt_stream << "You are Jules, a friendly and helpful AI assistant for the DashAIBrowser.\n";
    for (const auto& history_line : request->history()) {
        prompt_stream << history_line << "\n"; // Assuming history is already formatted "User: ..." or "Jules: ..."
    }
    prompt_stream << "User: " << request->user_message() << "\nJules: ";
    std::string full_prompt = prompt_stream.str();

    ipc::ErrorDetails adapter_error;
    std::string jules_reply = gemini_adapter_->GenerateText(
        full_prompt,
        request->preferences(),
        &adapter_error
    );

    if (adapter_error.error_code() != 0 || (jules_reply.empty() && adapter_error.error_message().empty())) {
        response->set_success(false);
        response->mutable_error_details()->CopyFrom(adapter_error);
        if (response->error_details().error_message().empty()) {
            SetError(response->mutable_error_details(), adapter_error.error_code() == 0 ? 500 : adapter_error.error_code(),
                     "Adapter failed to generate text and returned no error message.", "AI service could not complete the request.");
        }
        std::cerr << "AsolServiceImpl::ChatWithJules Error from adapter: (" << response->error_details().error_code()
                  << ") " << response->error_details().error_message() << std::endl;
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, response->error_details().error_message());
    }

    response->set_success(true);
    response->set_jules_response(jules_reply);

    std::cout << "AsolServiceImpl::ChatWithJules: Sending response for ID "
              << response->request_id() << ", Success: " << response->success() << std::endl;

    return ::grpc::Status::OK;
}


}  // namespace asol
}  // namespace dashaibrowser
