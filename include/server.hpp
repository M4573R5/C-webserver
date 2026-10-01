#pragma once

#include <string>
#include <string_view>
#include <filesystem>
using namespace std;

namespace net {
    class AdvancedWebServer {
        private:
            int server_fd{-1};
            int port;
            filesystem::path document_root; // Root folder for files

            void handle_client(int client_socket) const;
            string parse_resource_path(string_view request) const;
            string_view get_content_type(const filesystem::path& file_path) const;
            void send_error_response(int client_socket, string_view status, string_view message) const;

        public:
            AdvancedWebServer(int server_port, string_view root_dir);
            ~AdvancedWebServer();

            AdvancedWebServer(const AdvancedWebServer&) = delete;
            AdvancedWebServer& operator=(const AdvancedWebServer&) = delete;

            bool start();
            void run();
    };
}
