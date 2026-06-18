
#pragma once

/**
 *   对文件描述符的封装， RAII 
*/


#include <iostream>
#include <unistd.h>
#include <utility>

namespace cfl
{

class UniqueFd
{
private:
    int fd_ = -1;

public:
    UniqueFd() = default;
    explicit UniqueFd(int fd);
    UniqueFd(UniqueFd&& unique_fd);
    UniqueFd& operator=(UniqueFd&& unique_fd);

    explicit operator bool() const;
    bool IsValid() const;

    // 禁用拷贝
    UniqueFd(const UniqueFd& unique_fd) = delete;
    UniqueFd& operator=(const UniqueFd& unique_fd) = delete;

    int GetFd() const;

    int Release();
    void Reset(int fd);
    ~UniqueFd();
};
} // namespace cfl