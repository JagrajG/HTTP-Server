#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

#define BUFFER_SIZE 4096
#define PORT 8080
sockaddr_in server_addr;
int backlog = 10;
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
        std::cout << "Client Socket Sucess\n";
    }
    char buffer[BUFFER_SIZE];
    int recieve_result = recv(client_fd, buffer, BUFFER_SIZE, 0);

    if (recieve_result > 0)
    {
        std::cout << "Read: " << recieve_result << " Byte\n";
        std::cout.write(buffer, recieve_result);
    }
    else if (recieve_result == 0)
    {
        std::cout << "Data send fail\n";
    }
    else
    {
        std::cout << "Data recieve fail\n";
    }

    close(sockfd);
    return 0;
}