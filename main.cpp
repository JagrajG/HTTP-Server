#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <string>

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
        std::cout << "Socket Sucess\n";
    }

    // Bind
    std::memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // cast server_addr
    sockaddr *address_ptr = reinterpret_cast<sockaddr *>(&server_addr);
    int bind_result = bind(sockfd, address_ptr, sizeof(server_addr));
    bool bind_success = false;
    if (bind_result == -1)
    {
        std::cout << "Bind failed \n";
        close(sockfd);
        return 1;
    }
    else
    {
        std::cout << "Bind Sucess\n";
        bind_success = true;
    }

    // Listen
    if (bind_success)
    {
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
    }

    int client_fd = accept(sockfd, nullptr, nullptr);
    if (client_fd == -1)
    {
        std::cout << "Accept Failed\n";
        return 1;
    }
    else
    {
        std::cout << "Client Socket Success\n";
    }
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
    }
    else
    {
        std::cout << "Data receive fail\n";
    }

    std::string start_line(buffer, receive_result);
    std::string target = "\r\n";
    size_t pos_target = start_line.find(target);

    if (pos_target == std::string::npos)
    {
        std::cout << "Could not find end of request line\n";
    }

    else
    {
        std::string request_line = start_line.substr(0, pos_target);

        HttpRequest request;
        std::istringstream iss(request_line);

        if (iss >> request.method >> request.path >> request.version)
        {
            std::cout << "Method: " << request.method << "\n";
            std::cout << "Path: " << request.path << "\n";
            std::cout << "Version: " << request.version << "\n";
        }
    }

    std::string body = "<h1>Hello from my C++ server</h1>";

    std::string message =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: " +
        std::to_string(body.size()) + "\r\n"
                                      "\r\n" +
        body;
    int bytes_send = send(client_fd, message.c_str(), message.size(), 0);

    if (bytes_send == -1)
    {
        std::cerr << "Error in sending data\n";
    }
    else
    {
        std::cout << "Data sent sucesfully\n";
    }

    close(client_fd);
    close(sockfd);
    return 0;
}