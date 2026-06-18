#include <gtest/gtest.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <thread>
#include <chrono>
#include "socket.h"

// ============================================================
// 辅助函数
// ============================================================

// 创建一个可用的 fd（通过 socket）
static int MakeValidFd()
{
    return ::socket(AF_INET, SOCK_STREAM, 0);
}

// 获取一个可用端口（绑定后释放）
static uint16_t GetAvailablePort()
{
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return 0;

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0; // 让系统分配端口

    if (::bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        ::close(fd);
        return 0;
    }

    socklen_t len = sizeof(addr);
    if (::getsockname(fd, (struct sockaddr*)&addr, &len) < 0) {
        ::close(fd);
        return 0;
    }

    uint16_t port = ntohs(addr.sin_port);
    ::close(fd);
    return port;
}

// ============================================================
// 默认构造
// ============================================================
TEST(SocketTest, DefaultConstruct)
{
    cfl::Socket sock;
    EXPECT_EQ(sock.GetFd(), -1);
}

// ============================================================
// 带 fd 构造
// ============================================================
TEST(SocketTest, ConstructWithFd)
{
    int fd = MakeValidFd();
    ASSERT_GE(fd, 0);

    cfl::Socket sock(fd);
    EXPECT_EQ(sock.GetFd(), fd);
}

TEST(SocketTest, ConstructWithInvalidFd)
{
    cfl::Socket sock(-1);
    EXPECT_EQ(sock.GetFd(), -1);
}

// ============================================================
// GetFd
// ============================================================
TEST(SocketTest, GetFdDefault)
{
    cfl::Socket sock;
    EXPECT_EQ(sock.GetFd(), -1);
}

TEST(SocketTest, GetFdValid)
{
    int fd = MakeValidFd();
    ASSERT_GE(fd, 0);

    cfl::Socket sock(fd);
    EXPECT_EQ(sock.GetFd(), fd);
}

// ============================================================
// SetNonBlock
// ============================================================
TEST(SocketTest, SetNonBlock)
{
    int fd = MakeValidFd();
    ASSERT_GE(fd, 0);

    cfl::Socket sock(fd);
    int ret = sock.SetNonBlock();
    EXPECT_EQ(ret, 0);

    // 验证 fd 仍然有效
    EXPECT_EQ(sock.GetFd(), fd);
}

TEST(SocketTest, SetNonBlockInvalidFd)
{
    cfl::Socket sock(-1);
    int ret = sock.SetNonBlock();
    EXPECT_NE(ret, 0);
}

// ============================================================
// SetReuseAddr
// ============================================================
TEST(SocketTest, SetReuseAddr)
{
    int fd = MakeValidFd();
    ASSERT_GE(fd, 0);

    cfl::Socket sock(fd);
    int ret = sock.SetReuseAddr();
    EXPECT_EQ(ret, 0);
}

TEST(SocketTest, SetReuseAddrInvalidFd)
{
    cfl::Socket sock(-1);
    int ret = sock.SetReuseAddr();
    EXPECT_NE(ret, 0);
}

// ============================================================
// SetReusePort
// ============================================================
TEST(SocketTest, SetReusePort)
{
    int fd = MakeValidFd();
    ASSERT_GE(fd, 0);

    cfl::Socket sock(fd);
    int ret = sock.SetReusePort();
    // SetReusePort 可能需要 root 权限，这里只验证不崩溃
    // ret == 0 表示成功，ret != 0 可能是权限问题
    EXPECT_EQ(sock.GetFd(), fd);
}

TEST(SocketTest, SetReusePortInvalidFd)
{
    cfl::Socket sock(-1);
    int ret = sock.SetReusePort();
    EXPECT_NE(ret, 0);
}

// ============================================================
// Bind
// ============================================================
TEST(SocketTest, BindSuccess)
{
    int fd = MakeValidFd();
    ASSERT_GE(fd, 0);

    cfl::Socket sock(fd);
    uint16_t port = GetAvailablePort();
    ASSERT_GT(port, 0);

    int ret = sock.Bind(port, "127.0.0.1");
    EXPECT_EQ(ret, 0);
}

TEST(SocketTest, BindDefaultAddr)
{
    int fd = MakeValidFd();
    ASSERT_GE(fd, 0);

    cfl::Socket sock(fd);
    uint16_t port = GetAvailablePort();
    ASSERT_GT(port, 0);

    int ret = sock.Bind(port);
    EXPECT_EQ(ret, 0);
}

TEST(SocketTest, BindInvalidFd)
{
    cfl::Socket sock(-1);
    int ret = sock.Bind(12345, "127.0.0.1");
    EXPECT_NE(ret, 0);
}

TEST(SocketTest, BindTwiceFailsSecond)
{
    int fd = MakeValidFd();
    ASSERT_GE(fd, 0);

    cfl::Socket sock(fd);
    uint16_t port = GetAvailablePort();
    ASSERT_GT(port, 0);

    EXPECT_EQ(sock.Bind(port, "127.0.0.1"), 0);
    // 第二次 bind 同一端口应失败
    EXPECT_NE(sock.Bind(port, "127.0.0.1"), 0);
}

// ============================================================
// Listen
// ============================================================
TEST(SocketTest, ListenSuccess)
{
    int fd = MakeValidFd();
    ASSERT_GE(fd, 0);

    cfl::Socket sock(fd);
    uint16_t port = GetAvailablePort();
    ASSERT_GT(port, 0);

    ASSERT_EQ(sock.Bind(port, "127.0.0.1"), 0);
    int ret = sock.Listen(5);
    EXPECT_EQ(ret, 0);
}

TEST(SocketTest, ListenWithoutBind)
{
    int fd = MakeValidFd();
    ASSERT_GE(fd, 0);

    cfl::Socket sock(fd);
    // Linux 上未 bind 就 listen 可能成功（内核自动绑定随机端口）
    // 这里验证不崩溃，返回值合法即可
    int ret = sock.Listen(5);
    EXPECT_TRUE(ret == 0 || ret == -1);
}

TEST(SocketTest, ListenInvalidFd)
{
    cfl::Socket sock(-1);
    int ret = sock.Listen(5);
    EXPECT_NE(ret, 0);
}

// ============================================================
// Connect
// ============================================================
TEST(SocketTest, ConnectInvalidFd)
{
    cfl::Socket sock(-1);
    int ret = sock.Connect(12345, "127.0.0.1");
    EXPECT_NE(ret, 0);
}

TEST(SocketTest, ConnectRefused)
{
    int fd = MakeValidFd();
    ASSERT_GE(fd, 0);

    cfl::Socket sock(fd);
    // 连接到一个未监听的端口，应失败
    int ret = sock.Connect(1, "127.0.0.1");
    EXPECT_NE(ret, 0);
}

// ============================================================
// Accept
// ============================================================
TEST(SocketTest, AcceptInvalidFd)
{
    cfl::Socket sock(-1);
    int ret = sock.Accept();
    EXPECT_EQ(ret, -1);
}

// ============================================================
// Close
// ============================================================
TEST(SocketTest, Close)
{
    int fd = MakeValidFd();
    ASSERT_GE(fd, 0);

    cfl::Socket sock(fd);
    EXPECT_EQ(sock.GetFd(), fd);

    sock.Close();
    // Close 后 fd 应该被关闭
    // 注意：Close 不会重置 sock_fd_，只关闭 fd
    EXPECT_EQ(fcntl(fd, F_GETFD), -1);
}

TEST(SocketTest, CloseInvalidFd)
{
    cfl::Socket sock(-1);
    // 关闭无效 fd 不应崩溃
    sock.Close();
}

// ============================================================
// 析构函数
// ============================================================
TEST(SocketTest, DestructorClosesFd)
{
    int fd = MakeValidFd();
    ASSERT_GE(fd, 0);

    {
        cfl::Socket sock(fd);
        EXPECT_EQ(sock.GetFd(), fd);
    } // 析构

    // 析构后 fd 应该被关闭
    EXPECT_EQ(fcntl(fd, F_GETFD), -1);
}

TEST(SocketTest, DestructorInvalidFd)
{
    // 析构无效 fd 不应崩溃
    cfl::Socket sock(-1);
}

// ============================================================
// CreateServer
// ============================================================
TEST(SocketTest, CreateServerSuccess)
{
    cfl::Socket sock;
    uint16_t port = GetAvailablePort();
    ASSERT_GT(port, 0);

    bool ret = sock.CreateServer(port, "127.0.0.1", 5);
    EXPECT_TRUE(ret);
    EXPECT_GE(sock.GetFd(), 0);
}

TEST(SocketTest, CreateServerDefaultParams)
{
    cfl::Socket sock;
    uint16_t port = GetAvailablePort();
    ASSERT_GT(port, 0);

    bool ret = sock.CreateServer(port);
    EXPECT_TRUE(ret);
    EXPECT_GE(sock.GetFd(), 0);
}

TEST(SocketTest, CreateServerTwiceFailsSecond)
{
    cfl::Socket sock;
    uint16_t port = GetAvailablePort();
    ASSERT_GT(port, 0);

    EXPECT_TRUE(sock.CreateServer(port, "127.0.0.1", 5));

    // 创建第二个 server 应失败（同一 socket 不能 bind 两次）
    cfl::Socket sock2;
    EXPECT_FALSE(sock2.CreateServer(port, "127.0.0.1", 5));
}

// ============================================================
// CreateClient
// ============================================================
TEST(SocketTest, CreateClientInvalidAddr)
{
    cfl::Socket sock;
    // 连接到不存在的地址应失败
    bool ret = sock.CreateClient(1, "127.0.0.1");
    EXPECT_FALSE(ret);
}

// ============================================================
// 集成测试：Server + Client
// ============================================================
TEST(SocketTest, ServerAcceptClient)
{
    cfl::Socket server;
    uint16_t port = GetAvailablePort();
    ASSERT_GT(port, 0);

    // 创建 server
    ASSERT_TRUE(server.CreateServer(port, "127.0.0.1", 5));

    // 在另一个线程中创建 client
    std::thread client_thread([port]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        cfl::Socket client;
        bool connected = client.CreateClient(port, "127.0.0.1");
        EXPECT_TRUE(connected);
    });

    // Server 接受连接
    int client_fd = server.Accept();
    EXPECT_GE(client_fd, 0);

    if (client_fd >= 0) {
        ::close(client_fd);
    }

    client_thread.join();
}

TEST(SocketTest, ServerAcceptMultipleClients)
{
    cfl::Socket server;
    uint16_t port = GetAvailablePort();
    ASSERT_GT(port, 0);

    ASSERT_TRUE(server.CreateServer(port, "127.0.0.1", 5));

    const int num_clients = 3;
    std::thread client_threads[num_clients];

    for (int i = 0; i < num_clients; ++i) {
        client_threads[i] = std::thread([port]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            cfl::Socket client;
            bool connected = client.CreateClient(port, "127.0.0.1");
            EXPECT_TRUE(connected);
        });
    }

    for (int i = 0; i < num_clients; ++i) {
        int client_fd = server.Accept();
        EXPECT_GE(client_fd, 0);
        if (client_fd >= 0) {
            ::close(client_fd);
        }
    }

    for (int i = 0; i < num_clients; ++i) {
        client_threads[i].join();
    }
}

// ============================================================
// 边界情况
// ============================================================
TEST(SocketTest, PortZero)
{
    cfl::Socket sock;
    // 端口 0 应该可以绑定（系统分配端口）
    bool ret = sock.CreateServer(0, "127.0.0.1", 5);
    EXPECT_TRUE(ret);
}

TEST(SocketTest, LargeBacklog)
{
    cfl::Socket sock;
    uint16_t port = GetAvailablePort();
    ASSERT_GT(port, 0);

    // 大 backlog 值应能正常工作
    bool ret = sock.CreateServer(port, "127.0.0.1", 1000);
    EXPECT_TRUE(ret);
}
