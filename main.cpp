#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <string>
#include <fstream>

#define BUFFER_SIZE 4096
#define PORT 8080

sockaddr_in server_addr;
int backlog = 10;

struct HttpRequest
{
    std::string method;
    std::string path;
    std::string version;
};

struct HttpResponse
{
    std::string status;
    std::string content_type;
    std::string body;
};

void log_request(const HttpRequest &request, const HttpResponse &response)
{
    std::cout << request.method << " "
              << request.path << " -> "
              << response.status << " "
              << response.content_type << " "
              << response.body.size() << " bytes\n";
}

HttpRequest parse_request_line(const std::string &request_text)
{
    HttpRequest request;

    std::string target = "\r\n";
    size_t pos_target = request_text.find(target);

    if (pos_target == std::string::npos)
    {
        std::cout << "Could not find end of request line\n";
        return request;
    }

    std::string request_line = request_text.substr(0, pos_target);
    std::istringstream iss(request_line);

    if (iss >> request.method >> request.path >> request.version)
    {
        std::cout << "Method: " << request.method << "\n";
        std::cout << "Path: " << request.path << "\n";
        std::cout << "Version: " << request.version << "\n";
    }

    return request;
}

std::string get_content_type(const std::string &filePath)
{
    size_t dotPos = filePath.rfind('.');
    std::string type = "application/octet-stream";

    if (dotPos != std::string::npos)
    {
        if (filePath.substr(dotPos) == ".html")
        {
            return "text/html";
        }
        else if (filePath.substr(dotPos) == ".css")
        {
            return "text/css";
        }
        else if (filePath.substr(dotPos) == ".js")
        {
            return "application/javascript";
        }
        else if (filePath.substr(dotPos) == ".txt")
        {
            return "text/plain";
        }
    }

    return type;
}

HttpResponse make_error_response(const std::string &status, const std::string &message)
{
    HttpResponse response;

    response.status = status;
    response.content_type = "text/html";
    response.body = "<h1>" + message + "</h1>";

    return response;
}

HttpResponse build_file_response(const HttpRequest &request)
{
    HttpResponse response;
    std::string filepath;

    if (request.path == "/")
    {
        filepath = "public/index.html";
    }
    else
    {
        filepath = "public" + request.path;
    }

    std::ifstream file(filepath);

    if (file.is_open())
    {
        std::stringstream ss;
        ss << file.rdbuf();

        response.body = ss.str();
        response.status = "HTTP/1.1 200 OK";
        response.content_type = get_content_type(filepath);
    }
    else
    {
        response = make_error_response("HTTP/1.1 404 Not Found", "404 Not Found");
    }

    return response;
}

HttpResponse build_response(const HttpRequest &request)
{
    if (request.method.empty() || request.path.empty() || request.version.empty())
    {
        return make_error_response("HTTP/1.1 400 Bad Request", "400 Bad Request");
    }

    if (request.method == "GET")
    {
        return build_file_response(request);
    }

    return make_error_response("HTTP/1.1 405 Method Not Allowed", "405 Method Not Allowed");
}

std::string build_http_message(const HttpResponse &response)
{
    return response.status + "\r\n" +
           "Content-Type: " + response.content_type + "\r\n" +
           "Content-Length: " + std::to_string(response.body.size()) + "\r\n"
                                                                       "\r\n" +
           response.body;
}

void handle_client(int client_fd)
{
    char buffer[BUFFER_SIZE];
    int receive_result = recv(client_fd, buffer, BUFFER_SIZE, 0);

    if (receive_result > 0)
    {
        std::cout << "Read: " << receive_result << " Bytes\n";
        std::cout.write(buffer, receive_result);
    }
    else if (receive_result == 0)
    {
        std::cout << "The client closed the connection.\n";
        return;
    }
    else
    {
        std::cout << "Data receive fail\n";
        return;
    }

    std::string request_text(buffer, receive_result);

    HttpRequest request = parse_request_line(request_text);
    HttpResponse response = build_response(request);
    log_request(request, response);

    std::string message = build_http_message(response);

    int bytes_send = send(client_fd, message.c_str(), message.size(), 0);

    if (bytes_send == -1)
    {
        std::cerr << "Error in sending data\n";
    }
    else
    {
        std::cout << "Data sent successfully\n";
    }
}

int main()
{
    // Socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == -1)
    {
        std::cout << "Socket Failed\n";
        return 1;
    }
    else
    {
        std::cout << "Socket Success\n";
    }

    // Address setup
    std::memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    sockaddr *address_ptr = reinterpret_cast<sockaddr *>(&server_addr);

    // Socket options
    int option = 1;

    int setSockRes = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    if (setSockRes == -1)
    {
        std::cout << "setsockopt failed\n";
        close(sockfd);
        return 1;
    }
    else
    {
        std::cout << "SO_REUSEADDR Success\n";
    }

    // Bind
    int bind_result = bind(sockfd, address_ptr, sizeof(server_addr));

    if (bind_result == -1)
    {
        std::cout << "Bind failed\n";
        close(sockfd);
        return 1;
    }
    else
    {
        std::cout << "Bind Success\n";
    }

    // Listen
    int listen_result = listen(sockfd, backlog);

    if (listen_result == -1)
    {
        std::cout << "Listen Failed\n";
        close(sockfd);
        return 1;
    }
    else
    {
        std::cout << "Listening on port: " << PORT << "\n";
    }

    while (true)
    {
        int client_fd = accept(sockfd, nullptr, nullptr);

        if (client_fd == -1)
        {
            std::cout << "Accept Failed\n";
            close(sockfd);
            return 1;
        }
        else
        {
            std::cout << "Client Socket Success\n";
        }

        handle_client(client_fd);
        close(client_fd);
    }

    close(sockfd);

    return 0;
}