
#include "socket.h"

#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>

namespace cfl
{

int Socket::CreateFd()
{
    return socket(AF_INET , SOCK_STREAM , 0);
}

Socket::~Socket()
{
    close(sock_fd_);
}

int Socket::GetFd() const
{
    return sock_fd_;
}

int Socket::Bind(uint16_t port, std::string_view addr)
{
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, addr.data(), &server_addr.sin_addr);

    return bind(sock_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr));
}

int Socket::Listen(int backlog)
{
    return listen(sock_fd_, backlog);
}

int Socket::Accept()
{
    return accept(sock_fd_ , nullptr , nullptr);
}

int Socket::Connect(uint16_t port, std::string_view addr)
{
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, addr.data(), &server_addr.sin_addr);

    return connect(sock_fd_ , (struct sockaddr*)&server_addr, sizeof(server_addr));
}

void Socket::Close()
{
    close(sock_fd_);
}

int Socket::SetNonBlock()
{
    int flag = 1;
    return setsockopt(sock_fd_ , SOL_SOCKET , SO_REUSEADDR , &flag , sizeof(flag));
}

int Socket::SetReuseAddr()
{
    int flag = 1;
    return setsockopt(sock_fd_ , SOL_SOCKET , SO_REUSEADDR , &flag , sizeof(flag));
}

int Socket::SetReusePort()
{
    int flag = 1;
    return setsockopt(sock_fd_ , SOL_SOCKET , SO_REUSEPORT , &flag , sizeof(flag));
}

bool Socket::CreateServer(uint16_t port, std::string_view addr , int backlog)
{
    if((sock_fd_ = CreateFd()) == -1)
        return false;

    if(Bind(port , addr) == -1)
        return false;

    if(Listen(backlog) == -1)
        return false;

    return true;
}

bool Socket::CreateClient(uint16_t port, std::string_view addr)
{
    if((sock_fd_ = CreateFd()) == -1)
        return false;
    if(Connect(port , addr) == -1)
        return false;
    return true;
}

} // namespace cfl
