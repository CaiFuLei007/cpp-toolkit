#include <gtest/gtest.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <set>
#include "epoll.h"

// ============================================================
// 辅助函数
// ============================================================

// 创建一对相互连接的 socket，返回 {fd0, fd1}
static std::pair<int, int> MakeSocketPair()
{
    int fds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    if (ret != 0) {
        return {-1, -1};
    }
    return {fds[0], fds[1]};
}

// 设置 fd 为非阻塞
static void SetNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// ============================================================
// 构造 / 析构
// ============================================================
TEST(EpollTest, Construct)
{
    cpp_toolkit::Epoll epoll;
    // 构造不应崩溃，内部 epoll_fd_ 应为有效 fd
    SUCCEED();
}

TEST(EpollTest, Destruct)
{
    {
        cpp_toolkit::Epoll epoll;
    }
    // 析构不应崩溃
    SUCCEED();
}

// ============================================================
// AddFd
// 语义: 操作成功返回 true, 失败返回 false
// ============================================================
TEST(EpollTest, AddFdRead)
{
    cpp_toolkit::Epoll epoll;
    auto [fd0, fd1] = MakeSocketPair();
    ASSERT_NE(fd0, -1);
    ASSERT_NE(fd1, -1);

    // 添加成功返回 true
    EXPECT_TRUE(epoll.AddFd(fd0, EPOLLIN));

    close(fd0);
    close(fd1);
}

TEST(EpollTest, AddFdWrite)
{
    cpp_toolkit::Epoll epoll;
    auto [fd0, fd1] = MakeSocketPair();
    ASSERT_NE(fd0, -1);
    ASSERT_NE(fd1, -1);

    EXPECT_TRUE(epoll.AddFd(fd0, EPOLLOUT));

    close(fd0);
    close(fd1);
}

TEST(EpollTest, AddFdReadWrite)
{
    cpp_toolkit::Epoll epoll;
    auto [fd0, fd1] = MakeSocketPair();
    ASSERT_NE(fd0, -1);
    ASSERT_NE(fd1, -1);

    // 同时监听读写事件
    EXPECT_TRUE(epoll.AddFd(fd0, EPOLLIN | EPOLLOUT));

    close(fd0);
    close(fd1);
}

TEST(EpollTest, AddFdMultiple)
{
    cpp_toolkit::Epoll epoll;
    auto [fd0, fd1] = MakeSocketPair();
    auto [fd2, fd3] = MakeSocketPair();
    ASSERT_NE(fd0, -1);
    ASSERT_NE(fd2, -1);

    // 添加多个 fd
    EXPECT_TRUE(epoll.AddFd(fd0, EPOLLIN));
    EXPECT_TRUE(epoll.AddFd(fd2, EPOLLIN));

    close(fd0);
    close(fd1);
    close(fd2);
    close(fd3);
}

TEST(EpollTest, AddFdInvalidFd)
{
    cpp_toolkit::Epoll epoll;

    // 添加无效 fd, epoll_ctl 失败返回 false
    EXPECT_FALSE(epoll.AddFd(-1, EPOLLIN));
}

TEST(EpollTest, AddFdDuplicateFd)
{
    cpp_toolkit::Epoll epoll;
    auto [fd0, fd1] = MakeSocketPair();
    ASSERT_NE(fd0, -1);

    // 第一次添加成功
    EXPECT_TRUE(epoll.AddFd(fd0, EPOLLIN));

    // 重复添加同一 fd 应该失败 (EEXIST)
    EXPECT_FALSE(epoll.AddFd(fd0, EPOLLIN));

    close(fd0);
    close(fd1);
}

// ============================================================
// ModFd
// ============================================================
TEST(EpollTest, ModFdChangeEvents)
{
    cpp_toolkit::Epoll epoll;
    auto [fd0, fd1] = MakeSocketPair();
    ASSERT_NE(fd0, -1);

    // 先添加读事件
    EXPECT_TRUE(epoll.AddFd(fd0, EPOLLIN));

    // 修改为写事件
    EXPECT_TRUE(epoll.ModFd(fd0, EPOLLOUT));

    close(fd0);
    close(fd1);
}

TEST(EpollTest, ModFdToReadWrite)
{
    cpp_toolkit::Epoll epoll;
    auto [fd0, fd1] = MakeSocketPair();
    ASSERT_NE(fd0, -1);

    EXPECT_TRUE(epoll.AddFd(fd0, EPOLLIN));

    // 修改为同时监听读写
    EXPECT_TRUE(epoll.ModFd(fd0, EPOLLIN | EPOLLOUT));

    close(fd0);
    close(fd1);
}

TEST(EpollTest, ModFdWithoutAdd)
{
    cpp_toolkit::Epoll epoll;
    auto [fd0, fd1] = MakeSocketPair();
    ASSERT_NE(fd0, -1);

    // 未添加的 fd 进行修改应该失败 (ENOENT)
    EXPECT_FALSE(epoll.ModFd(fd0, EPOLLIN));

    close(fd0);
    close(fd1);
}

TEST(EpollTest, ModFdInvalidFd)
{
    cpp_toolkit::Epoll epoll;

    // 修改无效 fd 应该失败
    EXPECT_FALSE(epoll.ModFd(-1, EPOLLIN));
}

// ============================================================
// DelFd
// ============================================================
TEST(EpollTest, DelFdAfterAdd)
{
    cpp_toolkit::Epoll epoll;
    auto [fd0, fd1] = MakeSocketPair();
    ASSERT_NE(fd0, -1);

    EXPECT_TRUE(epoll.AddFd(fd0, EPOLLIN));
    // 删除成功返回 true
    EXPECT_TRUE(epoll.DelFd(fd0));

    close(fd0);
    close(fd1);
}

TEST(EpollTest, DelFdWithoutAdd)
{
    cpp_toolkit::Epoll epoll;
    auto [fd0, fd1] = MakeSocketPair();
    ASSERT_NE(fd0, -1);

    // 删除未添加的 fd 应该失败 (ENOENT)
    EXPECT_FALSE(epoll.DelFd(fd0));

    close(fd0);
    close(fd1);
}

TEST(EpollTest, DelFdInvalidFd)
{
    cpp_toolkit::Epoll epoll;

    // 删除无效 fd 应该失败
    EXPECT_FALSE(epoll.DelFd(-1));
}

TEST(EpollTest, DelFdTwice)
{
    cpp_toolkit::Epoll epoll;
    auto [fd0, fd1] = MakeSocketPair();
    ASSERT_NE(fd0, -1);

    EXPECT_TRUE(epoll.AddFd(fd0, EPOLLIN));
    EXPECT_TRUE(epoll.DelFd(fd0));

    // 第二次删除应该失败 (ENOENT)
    EXPECT_FALSE(epoll.DelFd(fd0));

    close(fd0);
    close(fd1);
}

TEST(EpollTest, AddAfterDel)
{
    cpp_toolkit::Epoll epoll;
    auto [fd0, fd1] = MakeSocketPair();
    ASSERT_NE(fd0, -1);

    // 添加 -> 删除 -> 再添加
    EXPECT_TRUE(epoll.AddFd(fd0, EPOLLIN));
    EXPECT_TRUE(epoll.DelFd(fd0));
    EXPECT_TRUE(epoll.AddFd(fd0, EPOLLOUT));

    close(fd0);
    close(fd1);
}

// ============================================================
// Wait
// ============================================================
TEST(EpollTest, WaitReadReady)
{
    cpp_toolkit::Epoll epoll;
    auto [fd0, fd1] = MakeSocketPair();
    ASSERT_NE(fd0, -1);
    ASSERT_NE(fd1, -1);

    SetNonBlocking(fd0);

    EXPECT_TRUE(epoll.AddFd(fd0, EPOLLIN));

    // 往 fd1 写入数据，使 fd0 变为可读
    const char* msg = "hello";
    write(fd1, msg, strlen(msg));

    std::vector<struct epoll_event> events(10);
    int n = epoll.Wait(events, 100);  // 100ms 超时
    EXPECT_EQ(n, 1);
    EXPECT_EQ(events[0].data.fd, fd0);
    EXPECT_TRUE(events[0].events & EPOLLIN);

    close(fd0);
    close(fd1);
}

TEST(EpollTest, WaitWriteReady)
{
    cpp_toolkit::Epoll epoll;
    auto [fd0, fd1] = MakeSocketPair();
    ASSERT_NE(fd0, -1);
    ASSERT_NE(fd1, -1);

    EXPECT_TRUE(epoll.AddFd(fd0, EPOLLOUT));

    // socket 默认发送缓冲区为空，应该立即可写
    std::vector<struct epoll_event> events(10);
    int n = epoll.Wait(events, 100);
    EXPECT_GE(n, 1);
    EXPECT_EQ(events[0].data.fd, fd0);
    EXPECT_TRUE(events[0].events & EPOLLOUT);

    close(fd0);
    close(fd1);
}

TEST(EpollTest, WaitTimeout)
{
    cpp_toolkit::Epoll epoll;
    auto [fd0, fd1] = MakeSocketPair();
    ASSERT_NE(fd0, -1);
    ASSERT_NE(fd1, -1);

    SetNonBlocking(fd0);

    EXPECT_TRUE(epoll.AddFd(fd0, EPOLLIN));

    // 没有数据可读，等待 50ms 应超时返回 0
    std::vector<struct epoll_event> events(10);
    int n = epoll.Wait(events, 50);
    EXPECT_EQ(n, 0);

    close(fd0);
    close(fd1);
}

TEST(EpollTest, WaitMultipleFds)
{
    cpp_toolkit::Epoll epoll;
    auto [fd0, fd1] = MakeSocketPair();
    auto [fd2, fd3] = MakeSocketPair();
    ASSERT_NE(fd0, -1);
    ASSERT_NE(fd2, -1);

    SetNonBlocking(fd0);
    SetNonBlocking(fd2);

    EXPECT_TRUE(epoll.AddFd(fd0, EPOLLIN));
    EXPECT_TRUE(epoll.AddFd(fd2, EPOLLIN));

    // 向两个 socket 写入数据
    const char* msg = "test";
    write(fd1, msg, strlen(msg));
    write(fd3, msg, strlen(msg));

    std::vector<struct epoll_event> events(10);
    int n = epoll.Wait(events, 100);
    EXPECT_EQ(n, 2);

    // 检查两个 fd 都被返回
    std::set<int> ready_fds;
    for (int i = 0; i < n; i++) {
        ready_fds.insert(events[i].data.fd);
    }
    EXPECT_TRUE(ready_fds.count(fd0));
    EXPECT_TRUE(ready_fds.count(fd2));

    close(fd0);
    close(fd1);
    close(fd2);
    close(fd3);
}

TEST(EpollTest, WaitAfterDel)
{
    cpp_toolkit::Epoll epoll;
    auto [fd0, fd1] = MakeSocketPair();
    ASSERT_NE(fd0, -1);
    ASSERT_NE(fd1, -1);

    SetNonBlocking(fd0);

    EXPECT_TRUE(epoll.AddFd(fd0, EPOLLIN));
    EXPECT_TRUE(epoll.DelFd(fd0));

    // 写入数据
    const char* msg = "test";
    write(fd1, msg, strlen(msg));

    // fd 已删除，Wait 应超时返回 0
    std::vector<struct epoll_event> events(10);
    int n = epoll.Wait(events, 50);
    EXPECT_EQ(n, 0);

    close(fd0);
    close(fd1);
}

TEST(EpollTest, WaitEmptyEvents)
{
    cpp_toolkit::Epoll epoll;

    // 没有添加任何 fd，Wait 应超时返回 0
    std::vector<struct epoll_event> events(10);
    int n = epoll.Wait(events, 50);
    EXPECT_EQ(n, 0);
}

TEST(EpollTest, WaitZeroTimeout)
{
    cpp_toolkit::Epoll epoll;
    auto [fd0, fd1] = MakeSocketPair();
    ASSERT_NE(fd0, -1);
    ASSERT_NE(fd1, -1);

    SetNonBlocking(fd0);

    EXPECT_TRUE(epoll.AddFd(fd0, EPOLLIN));

    // 没有数据可读，timeout=0 应立即返回 0
    std::vector<struct epoll_event> events(10);
    int n = epoll.Wait(events, 0);
    EXPECT_EQ(n, 0);

    close(fd0);
    close(fd1);
}

TEST(EpollTest, WaitZeroTimeoutWithData)
{
    cpp_toolkit::Epoll epoll;
    auto [fd0, fd1] = MakeSocketPair();
    ASSERT_NE(fd0, -1);
    ASSERT_NE(fd1, -1);

    SetNonBlocking(fd0);

    EXPECT_TRUE(epoll.AddFd(fd0, EPOLLIN));

    // 写入数据
    const char* msg = "hello";
    write(fd1, msg, strlen(msg));

    // 有数据可读，timeout=0 应立即返回 1
    std::vector<struct epoll_event> events(10);
    int n = epoll.Wait(events, 0);
    EXPECT_EQ(n, 1);
    EXPECT_EQ(events[0].data.fd, fd0);
    EXPECT_TRUE(events[0].events & EPOLLIN);

    close(fd0);
    close(fd1);
}

TEST(EpollTest, WaitSmallEventsBuffer)
{
    cpp_toolkit::Epoll epoll;
    auto [fd0, fd1] = MakeSocketPair();
    auto [fd2, fd3] = MakeSocketPair();
    ASSERT_NE(fd0, -1);
    ASSERT_NE(fd2, -1);

    SetNonBlocking(fd0);
    SetNonBlocking(fd2);

    EXPECT_TRUE(epoll.AddFd(fd0, EPOLLIN));
    EXPECT_TRUE(epoll.AddFd(fd2, EPOLLIN));

    // 向两个 socket 写入数据
    const char* msg = "test";
    write(fd1, msg, strlen(msg));
    write(fd3, msg, strlen(msg));

    // events 缓冲区只够放 1 个事件
    std::vector<struct epoll_event> events(1);
    int n = epoll.Wait(events, 100);
    // epoll_wait 只返回能放下的事件数量
    EXPECT_GE(n, 1);

    close(fd0);
    close(fd1);
    close(fd2);
    close(fd3);
}

// ============================================================
// 综合测试
// ============================================================
TEST(EpollTest, AddModDelFlow)
{
    cpp_toolkit::Epoll epoll;
    auto [fd0, fd1] = MakeSocketPair();
    ASSERT_NE(fd0, -1);
    ASSERT_NE(fd1, -1);

    // 添加读事件
    EXPECT_TRUE(epoll.AddFd(fd0, EPOLLIN));

    // 修改为写事件
    EXPECT_TRUE(epoll.ModFd(fd0, EPOLLOUT));

    // 验证写事件可用
    std::vector<struct epoll_event> events(10);
    int n = epoll.Wait(events, 100);
    EXPECT_GE(n, 1);
    EXPECT_TRUE(events[0].events & EPOLLOUT);

    // 删除
    EXPECT_TRUE(epoll.DelFd(fd0));

    // 删除后 Wait 应超时
    n = epoll.Wait(events, 50);
    EXPECT_EQ(n, 0);

    close(fd0);
    close(fd1);
}

TEST(EpollTest, MultipleInstances)
{
    // 两个独立的 Epoll 实例
    cpp_toolkit::Epoll epoll1;
    cpp_toolkit::Epoll epoll2;

    auto [fd0, fd1] = MakeSocketPair();
    auto [fd2, fd3] = MakeSocketPair();
    ASSERT_NE(fd0, -1);
    ASSERT_NE(fd2, -1);

    SetNonBlocking(fd0);
    SetNonBlocking(fd2);

    // epoll1 监听 fd0，epoll2 监听 fd2
    EXPECT_TRUE(epoll1.AddFd(fd0, EPOLLIN));
    EXPECT_TRUE(epoll2.AddFd(fd2, EPOLLIN));

    const char* msg = "test";
    write(fd1, msg, strlen(msg));
    write(fd3, msg, strlen(msg));

    // epoll1 应该只收到 fd0 的事件
    std::vector<struct epoll_event> events1(10);
    int n1 = epoll1.Wait(events1, 100);
    EXPECT_EQ(n1, 1);
    EXPECT_EQ(events1[0].data.fd, fd0);

    // epoll2 应该只收到 fd2 的事件
    std::vector<struct epoll_event> events2(10);
    int n2 = epoll2.Wait(events2, 100);
    EXPECT_EQ(n2, 1);
    EXPECT_EQ(events2[0].data.fd, fd2);

    close(fd0);
    close(fd1);
    close(fd2);
    close(fd3);
}
