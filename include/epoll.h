
#pragma once

/**
 * epoll 封装
*/

#include <vector>
#include <sys/epoll.h>

namespace cfl {

class Epoll {
private:
    int epoll_fd_;
private:
    int CreateEpoll();
    int EpollCtl(int op, int fd, struct epoll_event* event);
public:
    Epoll();
    ~Epoll();

    bool AddFd(int fd, uint32_t events);
    bool ModFd(int fd, uint32_t events);
    bool DelFd(int fd);

    int Wait(std::vector<struct epoll_event>& events, int timeout = -1);


};

} // namespace cfl