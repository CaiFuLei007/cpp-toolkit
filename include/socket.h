
#pragma once


/**
 *  对 socket 的封装
*/

#include <iostream>
#include <stdint.h>
#include <string_view>

namespace cfl
{
class Socket
{
private:
    int sock_fd_ = -1;

    int CreateFd();
public:
    Socket() = default;
    Socket(int fd) : sock_fd_(fd) {}
    ~Socket();

    int GetFd() const;

    int Bind(uint16_t port , std::string_view addr = "0.0.0.0");
    int Listen(int backlog);
    int Accept();

    int Connect(uint16_t port, std::string_view addr);
    void Close();

    int SetNonBlock();
    int SetReuseAddr();
    int SetReusePort();
    bool CreateServer(uint16_t port, std::string_view addr = "0.0.0.0" , int backlog = 5);
    bool CreateClient(uint16_t port, std::string_view addr);
};
    
} // namespace cfl



