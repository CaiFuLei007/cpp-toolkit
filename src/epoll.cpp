
#include "epoll.h"

#include <unistd.h>

namespace cfl
{
int Epoll::CreateEpoll()
{
    return epoll_create(1);
}
Epoll::Epoll()
:epoll_fd_(CreateEpoll())
{}

int Epoll::EpollCtl(int op, int fd, struct epoll_event* event)
{
    return epoll_ctl(epoll_fd_, op, fd, event);
}

Epoll::~Epoll()
{
    close(epoll_fd_);
}

bool Epoll::AddFd(int fd, uint32_t events)
{
    struct epoll_event event;
    event.events = events;
    event.data.fd = fd;
    return EpollCtl(EPOLL_CTL_ADD, fd, &event);
}
bool Epoll::ModFd(int fd, uint32_t events)
{
    struct epoll_event event;
    event.events = events;
    event.data.fd = fd;
    return EpollCtl(EPOLL_CTL_MOD, fd, &event);
}
bool Epoll::DelFd(int fd)
{
    return EpollCtl(EPOLL_CTL_DEL, fd, nullptr);
}

int Epoll::Wait(std::vector<struct epoll_event>& events, int timeout)
{
    return epoll_wait(epoll_fd_, events.data(), events.size(), timeout);
}

}   // namespace cfl