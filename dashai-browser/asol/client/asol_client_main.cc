#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <chrono> // For timeouts
#include <ctime>  // For time for request ID

#include <grpcpp/grpcpp.h>
#include "proto/asol_service.grpc.pb.h" // Generated gRPC classes

// Helper function to generate a unique request ID (simple version)
std::string GenerateRequestID(const std::string& prefix = "req") {
    static int counter = 0;
    return prefix + "_" + std::to_string(time(0)) + "_" + std::to_string(++counter);
}

class AsolClient {
public:
    AsolClient(std::shared_ptr<grpc::Channel> channel)
        : stub_(dashaibrowser::ipc::AsolInterface::NewStub(channel)) {}

    // Calls the GetSummary RPC
    void GetSummary(const std::string& text_to_summarize) {
        dashaibrowser::ipc::SummaryRequest request;
        request.set_request_id(GenerateRequestID("summary"));
        request.set_original_text(text_to_summarize);

        dashaibrowser::ipc::SummaryResponse response;
        grpc::ClientContext context;
        std::chrono::system_clock::time_point deadline =
            std::chrono::system_clock::now() + std::chrono::seconds(15);
        context.set_deadline(deadline);

        std::cout << "\n[Client] Sending GetSummary request..." << std::endl;
        grpc::Status status = stub_->GetSummary(&context, request, &response);

        if (status.ok() && response.success()) {
            std::cout << "[Client] Summary: " << response.summarized_text() << std::endl;
        } else {
            PrintRpcError("GetSummary", status, response.has_error_details() ? &response.error_details() : nullptr);
        }
    }

    // Calls the ChatWithJules RPC
    void ChatWithJules(const std::string& user_message, const std::string& session_id, const std::vector<std::string>& history) {
        dashaibrowser::ipc::ConversationRequest request;
        request.set_request_id(GenerateRequestID("chat"));
        request.set_session_id(session_id);
        request.set_user_message(user_message);
        for(const auto& hist_item : history) {
            request.add_history(hist_item);
        }

        dashaibrowser::ipc::ConversationResponse response;
        grpc::ClientContext context;
        std::chrono::system_clock::time_point deadline =
            std::chrono::system_clock::now() + std::chrono::seconds(30); // Longer timeout for chat
        context.set_deadline(deadline);

        // std::cout << "\n[Client] Sending ChatWithJules request..." << std::endl; // Less verbose for chat loop
        grpc::Status status = stub_->ChatWithJules(&context, request, &response);

        if (status.ok() && response.success()) {
            std::cout << "Jules: " << response.jules_response() << std::endl;
        } else {
            PrintRpcError("ChatWithJules", status, response.has_error_details() ? &response.error_details() : nullptr);
        }
    }

private:
    void PrintRpcError(const std::string& rpc_name, const grpc::Status& status, const dashaibrowser::ipc::ErrorDetails* details) {
        std::cerr << "[Client] " << rpc_name << " RPC failed." << std::endl;
        if (!status.ok()) {
            std::cerr << "  gRPC Error Code: " << status.error_code() << std::endl;
            std::cerr << "  gRPC Error Message: " << status.error_message() << std::endl;
            if (!status.error_details().empty()){
                 std::cerr << "  gRPC Error Details: " << status.error_details() << std::endl;
            }
        }
        if (details) {
            std::cerr << "  ASOL Error Code: " << details->error_code() << std::endl;
            std::cerr << "  ASOL Error Message: " << details->error_message() << std::endl;
            if (!details->user_facing_message().empty()) {
                 std::cerr << "  ASOL User Message: " << details->user_facing_message() << std::endl;
            }
        }
    }

    std::unique_ptr<dashaibrowser::ipc::AsolInterface::Stub> stub_;
};

void RunChatSession(AsolClient& client) {
    std::cout << "\nStarting interactive chat session with Jules." << std::endl;
    std::cout << "Type 'quit' or 'exit' to end the session." << std::endl;

    std::string user_input;
    std::string session_id = GenerateRequestID("session"); // Simple session ID for this run
    std::vector<std::string> chat_history; // Simple history

    while (true) {
        std::cout << "You: ";
        std::getline(std::cin, user_input);

        if (user_input == "quit" || user_input == "exit") {
            std::cout << "Jules: Goodbye!" << std::endl;
            break;
        }
        if (user_input.empty()) {
            continue;
        }

        // Add to history before sending (optional, for more advanced context)
        // For now, server-side prompt prepends "User: "
        // chat_history.push_back("User: " + user_input);

        client.ChatWithJules(user_input, session_id, chat_history);

        // If server sent back its response, we could add it to history for next turn
        // chat_history.push_back("Jules: " + <actual_response_from_server>);
        // Keep history size manageable if used:
        // if (chat_history.size() > 10) { chat_history.erase(chat_history.begin(), chat_history.begin() + chat_history.size() - 10); }
    }
}


int main(int argc, char** argv) {
    std::string target_str = "localhost:50051";
    if (argc > 1 && std::string(argv[1]) != "--chat") { // Basic arg parsing
        target_str = argv[1];
    }

    bool run_chat_mode = false;
    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            if (std::string(argv[i]) == "--chat") {
                run_chat_mode = true;
                break;
            }
        }
    }

    std::shared_ptr<grpc::Channel> channel =
        grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials());

    if (!channel) {
        std::cerr << "Failed to create gRPC channel to " << target_str << std::endl;
        return 1;
    }
    std::cout << "[Client] Connecting to ASOL Gateway at " << target_str << std::endl;

    AsolClient client(channel);

    if (run_chat_mode) {
        RunChatSession(client);
    } else {
        std::cout << "\nRunning GetSummary example. Use --chat for interactive mode." << std::endl;
        std::string sample_text_long =
            "The James Webb Space Telescope (JWST) is a space telescope designed primarily to conduct infrared astronomy. "
            "As the largest optical telescope in space, its high infrared resolution and sensitivity allow it to view objects "
            "too old, distant, or faint for the Hubble Space Telescope. This is expected to enable a broad range of "
            "investigations across the fields of astronomy and cosmology, such as observation of the first stars and "
            "the formation of the first galaxies, and detailed atmospheric characterization of potentially habitable exoplanets. "
            "JWST was launched on 25 December 2021 on an Ariane 5 rocket from Kourou, French Guiana, and arrived at the "
            "Sun–Earth L2 Lagrange point in January 2022. The first JWST image was released to the public via a press "
            "conference on 11 July 2022. The telescope is the successor of the Hubble Space Telescope and is a flagship "
            "mission of NASA in partnership with the European Space Agency (ESA) and the Canadian Space Agency (CSA).";
        client.GetSummary(sample_text_long);
    }


    std::cout << "\n[Client] ASOL client finished." << std::endl;
    return 0;
}
