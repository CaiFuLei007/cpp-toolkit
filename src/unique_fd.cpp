
#include "unique_fd.h"

namespace cfl
{

UniqueFd::UniqueFd(int fd)
:fd_(fd)
{}

UniqueFd::UniqueFd(UniqueFd&& unique_fd)
{
    std::swap(fd_ , unique_fd.fd_);
}

UniqueFd::operator bool() const
{
    return IsValid();
}
bool UniqueFd::IsValid() const
{
    return fd_ != -1;
}

UniqueFd& UniqueFd::operator=(UniqueFd&& unique_fd)
{
    if (this != &unique_fd)
    {
        std::swap(fd_ , unique_fd.fd_);
    }
    return *this;
}

int UniqueFd::GetFd() const
{
    return fd_;
}

int UniqueFd::Release()
{
    int fd = fd_;
    fd_ = -1;
    return fd;
}
void UniqueFd::Reset(int fd)
{
    if(fd != fd_ && fd_ != -1)
        close(fd_);
    fd_ = fd;
}

UniqueFd::~UniqueFd()
{
    if (fd_ != -1)
    {
        close(fd_);
    }
}

} // namespace cfl