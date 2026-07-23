#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <string>
#include <fstream>
#include <map>
#include <cstdio>

#define BUFFER_SIZE 4096
#define PORT 8080

sockaddr_in server_addr;
int backlog = 10;

struct HttpRequest
{
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;
};

struct HttpResponse
{
    std::string status;
    std::string content_type;
    std::string body;
};

// Function prototypes
void log_request(const HttpRequest &request, const HttpResponse &response);

void remove_trailing_carriage_return(std::string &line);

HttpRequest parse_http_request(const std::string &request_text);

std::string get_content_type(const std::string &filePath);

HttpResponse make_error_response(const std::string &status, const std::string &message);

bool is_safe_path(const std::string &path);

HttpResponse build_file_response(const HttpRequest &request);

HttpResponse build_echo_response(const HttpRequest &request);

HttpResponse build_delete_response(const HttpRequest &request);

HttpResponse build_response(const HttpRequest &request);

std::string build_http_message(const HttpResponse &response);

bool has_valid_content_length(const HttpRequest &request);

void handle_client(int client_fd);

bool has_valid_content_length(const HttpRequest &request)
{
    auto it = request.headers.find("Content-Length");

    if (it == request.headers.end())
    {
        return true;
    }

    try
    {
        int expected_length = std::stoi(it->second);
        return expected_length == static_cast<int>(request.body.size());
    }
    catch (...)
    {
        return false;
    }
}

void log_request(const HttpRequest &request, const HttpResponse &response)
{
    std::cout << request.method << " "
              << request.path << " -> "
              << response.status << " "
              << response.content_type << " "
              << response.body.size() << " bytes\n";
}

void remove_trailing_carriage_return(std::string &line)
{
    if (!line.empty() && line.back() == '\r')
    {
        line.pop_back();
    }
}

HttpRequest parse_http_request(const std::string &request_text)
{
    HttpRequest request;

    std::istringstream stream(request_text);
    std::string line;

    if (!std::getline(stream, line))
    {
        std::cout << "Could not read request line\n";
        return request;
    }

    remove_trailing_carriage_return(line);

    std::istringstream request_line_stream(line);

    if (request_line_stream >> request.method >> request.path >> request.version)
    {
        std::cout << "Method: " << request.method << "\n";
        std::cout << "Path: " << request.path << "\n";
        std::cout << "Version: " << request.version << "\n";
    }
    else
    {
        std::cout << "Failed to parse request line\n";
        return request;
    }

    while (std::getline(stream, line))
    {
        remove_trailing_carriage_return(line);

        if (line.empty())
        {
            break;
        }

        size_t colon_pos = line.find(':');

        if (colon_pos == std::string::npos)
        {
            continue;
        }

        std::string key = line.substr(0, colon_pos);
        std::string value = line.substr(colon_pos + 1);

        if (!value.empty() && value[0] == ' ')
        {
            value.erase(0, 1);
        }

        request.headers[key] = value;
    }

    std::string body;
    std::string body_line;

    while (std::getline(stream, body_line))
    {
        body += body_line;

        if (!stream.eof())
        {
            body += "\n";
        }
    }

    request.body = body;

    std::cout << "Headers:\n";
    for (const auto &header : request.headers)
    {
        std::cout << header.first << ": " << header.second << "\n";
    }

    if (!request.body.empty())
    {
        std::cout << "Body: " << request.body << "\n";
    }

    return request;
}

HttpResponse build_delete_response(const HttpRequest &request)
{
    if (!is_safe_path(request.path))
    {
        return make_error_response("HTTP/1.1 403 Forbidden", "403 Forbidden");
    }

    if (request.path == "/")
    {
        return make_error_response("HTTP/1.1 400 Bad Request", "400 Bad Request");
    }

    std::string filepath = "public" + request.path;

    int result = std::remove(filepath.c_str());

    if (result == 0)
    {
        HttpResponse response;
        response.status = "HTTP/1.1 200 OK";
        response.content_type = "text/plain";
        response.body = "Deleted: " + request.path;
        return response;
    }

    return make_error_response("HTTP/1.1 404 Not Found", "404 Not Found");
}

HttpResponse build_echo_response(const HttpRequest &request)
{
    HttpResponse response;

    response.status = "HTTP/1.1 200 OK";
    response.content_type = "text/plain";
    response.body = request.body;

    return response;
}

std::string get_content_type(const std::string &filePath)
{
    size_t dotPos = filePath.rfind('.');
    std::string type = "application/octet-stream";

    if (dotPos != std::string::npos)
    {
        std::string ext = filePath.substr(dotPos);

        if (ext == ".html")
        {
            return "text/html";
        }
        else if (ext == ".css")
        {
            return "text/css";
        }
        else if (ext == ".js")
        {
            return "application/javascript";
        }
        else if (ext == ".txt")
        {
            return "text/plain";
        }
        else if (ext == ".png")
        {
            return "image/png";
        }
        else if (ext == ".jpg")
        {
            return "image/jpeg";
        }
        else if (ext == ".jpeg")
        {
            return "image/jpeg";
        }
        else if (ext == ".gif")
        {
            return "image/gif";
        }
        else if (ext == ".svg")
        {
            return "image/svg+xml";
        }
        else if (ext == ".ico")
        {
            return "image/x-icon";
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

bool is_safe_path(const std::string &path)
{
    if (path.find("..") != std::string::npos)
    {
        return false;
    }

    return true;
}

HttpResponse build_file_response(const HttpRequest &request)
{
    HttpResponse response;
    std::string filepath;

    if (!is_safe_path(request.path))
    {
        return make_error_response("HTTP/1.1 403 Forbidden", "403 Forbidden");
    }

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

    if (!has_valid_content_length(request))
    {
        return make_error_response("HTTP/1.1 400 Bad Request", "400 Bad Request");
    }

    if (request.method == "GET")
    {
        return build_file_response(request);
    }

    if (request.method == "POST" && request.path == "/echo")
    {
        return build_echo_response(request);
    }

    if (request.method == "DELETE")
    {
        return build_delete_response(request);
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
        std::cout << "\n";
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

    HttpRequest request = parse_http_request(request_text);
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