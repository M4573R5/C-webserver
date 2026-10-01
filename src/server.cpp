#include "server.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace std;

namespace net {

    AdvancedWebServer::AdvancedWebServer(int server_port, string_view root_dir) : port(server_port), document_root(root_dir) {}

    AdvancedWebServer::~AdvancedWebServer() {
        if (server_fd != -1) close(server_fd);
    }

    bool AdvancedWebServer::start() {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd == -1) return false;

        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        if (bind(server_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) return false;
        if (listen(server_fd, 20) < 0) return false;

        cout << "running on port " << port << endl;
        cout << "open url: http://127.0.0.1:" << port << endl;
        return true;
    }

    void AdvancedWebServer::run() {
        while (true) {
            int client_socket = accept(server_fd, nullptr, nullptr);
            if (client_socket >= 0) {
                jthread(&AdvancedWebServer::handle_client, this, client_socket).detach();
            }
        }
    }

    string_view AdvancedWebServer::get_content_type(const filesystem::path& file_path) const {
        auto ext = file_path.extension().string();
        if (ext == ".html" || ext == ".htm") return "text/html";
        if (ext == ".css") return "text/css";
        if (ext == ".js") return "application/javascript";
        if (ext == ".json") return "application/json";
        if (ext == ".png") return "image/png";
        if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
        return "application/octet-stream";
    }

    void AdvancedWebServer::send_error_response(int client_socket, string_view status, string_view message) const {
        string body = "<html><body><h1>" + string(status) + "</h1><p>" + string(message) + "</p></body></html>";
        ostringstream response;
        response << "HTTP/1.1 " << status << "\r\n"
            << "Content-Type: text/html\r\n"
            << "Content-Length: " << body.length() << "\r\n"
            << "Connection: close\r\n\r\n"
            << body;
        string response_str = response.str();
        write(client_socket, response_str.data(), response_str.length());
    }

    void AdvancedWebServer::handle_client(int client_socket) const {
        vector<char> buffer(4096, 0);
        ssize_t bytes_read = read(client_socket, buffer.data(), buffer.size() - 1);
        if (bytes_read <= 0) {
            close(client_socket);
            return;
        }

        string_view request(buffer.data(), bytes_read);
        string uri_path = parse_resource_path(request);
        if (uri_path == "/api/stats") {
            string json_payload = "{"
                "\"runtime\": \"Modern C++20\","
                "\"memory_mode\": \"Zero-Copy RAII\","
                "\"active_workers\": \"Dynamic pool active\""
            "}";

            ostringstream response;
            response << "HTTP/1.1 200 OK\r\n"
                << "Content-Type: application/json\r\n"
                << "Content-Length: " << json_payload.length() << "\r\n"
                << "Connection: close\r\n\r\n"
                << json_payload;

            string response_str = response.str();
            write(client_socket, response_str.data(), response_str.length());
            close(client_socket);
            return;
        }
        else if (uri_path == "/") uri_path = "/index.html";
        
        filesystem::path safe_target_path = document_root / uri_path.substr(1);

        auto canonical_root = filesystem::canonical(document_root); //Prevent Directory Traversal Attacks
        error_code ec;
        auto canonical_target = filesystem::weakly_canonical(safe_target_path, ec);

        if (ec || canonical_target.string().find(canonical_root.string()) != 0) {
            send_error_response(client_socket, "403 Forbidden", "Directory traversal attempt detected.");
            close(client_socket);
            return;
        }

        // Validate Files
        if (!filesystem::exists(canonical_target) || filesystem::is_directory(canonical_target)) {
            send_error_response(client_socket, "404 Not Found", "The requested asset does not exist on this server cluster.");
            close(client_socket);
            return;
        }

        ifstream file_stream(canonical_target, ios::in | ios::binary);
        if (!file_stream) {
            send_error_response(client_socket, "500 Internal Server Error", "Could not open system file handles.");
            close(client_socket);
            return;
        }

        // Determine size and content type
        size_t file_size = filesystem::file_size(canonical_target);
        string_view content_type = get_content_type(canonical_target);

        ostringstream headers;
        headers << "HTTP/1.1 200 OK\r\n"
                << "Content-Type: " << content_type << "\r\n"
                << "Content-Length: " << file_size << "\r\n"
                << "Server: Custom C++20 Infrastructure\r\n"
                << "Connection: close\r\n\r\n";
        
        string header_str = headers.str();
        write(client_socket, header_str.data(), header_str.length());

        vector<char> file_buffer(4096); //4KB chunks
        while (file_stream.read(file_buffer.data(), file_buffer.size()) || file_stream.gcount() > 0) {
            write(client_socket, file_buffer.data(), file_stream.gcount());
        }

        cout << "[DEBUG] :" << uri_path << "200 OK (" << file_size << " bytes)" << endl;
        close(client_socket);
    }

    string AdvancedWebServer::parse_resource_path(string_view request) const {
        size_t first_space = request.find(' ');
        if (first_space == string_view::npos) return "/";
        size_t second_space = request.find(' ', first_space + 1);
        if (second_space == string_view::npos) return "/";
        return string(request.substr(first_space + 1, second_space - first_space - 1));
    }
}
