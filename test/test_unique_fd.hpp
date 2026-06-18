#include <gtest/gtest.h>
#include <fcntl.h>
#include <unistd.h>
#include "unique_fd.h"

// 辅助函数：创建一个有效的 fd（通过 pipe）
static int MakeFd()
{
    int fds[2];
    pipe(fds);
    // 关闭读端，只保留写端作为测试用 fd
    close(fds[0]);
    return fds[1];
}

// ============================================================
// 默认构造
// ============================================================
TEST(UniqueFdTest, DefaultConstruct)
{
    cfl::UniqueFd ufd;
    EXPECT_FALSE(ufd.IsValid());
    EXPECT_FALSE(ufd);
    EXPECT_EQ(ufd.GetFd(), -1);
}

// ============================================================
// explicit 构造（int）
// ============================================================
TEST(UniqueFdTest, ExplicitConstruct)
{
    int fd = MakeFd();
    cfl::UniqueFd ufd(fd);
    EXPECT_TRUE(ufd.IsValid());
    EXPECT_TRUE(ufd);
    EXPECT_EQ(ufd.GetFd(), fd);
}

TEST(UniqueFdTest, ExplicitConstructMinusOne)
{
    cfl::UniqueFd ufd(-1);
    EXPECT_FALSE(ufd.IsValid());
    EXPECT_EQ(ufd.GetFd(), -1);
}

// 确认 explicit 生效：以下代码不应编译
// void foo(cfl::UniqueFd ufd) {}
// void test() { foo(5); }  // 编译错误，explicit 阻止隐式转换

// ============================================================
// 移动构造
// ============================================================
TEST(UniqueFdTest, MoveConstruct)
{
    int fd = MakeFd();
    cfl::UniqueFd src(fd);
    cfl::UniqueFd dst(std::move(src));

    // dst 接管了 fd
    EXPECT_TRUE(dst.IsValid());
    EXPECT_EQ(dst.GetFd(), fd);

    // src 被置空
    EXPECT_FALSE(src.IsValid());
    EXPECT_EQ(src.GetFd(), -1);
}

TEST(UniqueFdTest, MoveConstructFromInvalid)
{
    cfl::UniqueFd src;
    cfl::UniqueFd dst(std::move(src));

    EXPECT_FALSE(dst.IsValid());
    EXPECT_FALSE(src.IsValid());
}

// ============================================================
// 移动赋值
// ============================================================
TEST(UniqueFdTest, MoveAssign)
{
    int fd1 = MakeFd();
    int fd2 = MakeFd();

    cfl::UniqueFd ufd1(fd1);
    cfl::UniqueFd ufd2(fd2);

    ufd1 = std::move(ufd2);

    // ufd1 接管 fd2
    EXPECT_TRUE(ufd1.IsValid());
    EXPECT_EQ(ufd1.GetFd(), fd2);

    // ufd2 持有 ufd1 的旧 fd（合法但未指定状态）
    EXPECT_TRUE(ufd2.IsValid());
    EXPECT_EQ(ufd2.GetFd(), fd1);

    // 两个 fd 都不应被关闭
    EXPECT_EQ(fcntl(fd1, F_GETFD), 0);
    EXPECT_EQ(fcntl(fd2, F_GETFD), 0);
}

TEST(UniqueFdTest, MoveAssignSelf)
{
    int fd = MakeFd();
    cfl::UniqueFd ufd(fd);

    // 自赋值不应出错
    ufd = std::move(ufd);
    EXPECT_TRUE(ufd.IsValid());
    EXPECT_EQ(ufd.GetFd(), fd);
}

TEST(UniqueFdTest, MoveAssignToInvalid)
{
    int fd = MakeFd();
    cfl::UniqueFd src(fd);
    cfl::UniqueFd dst;

    dst = std::move(src);

    EXPECT_TRUE(dst.IsValid());
    EXPECT_EQ(dst.GetFd(), fd);
    EXPECT_FALSE(src.IsValid());
}

TEST(UniqueFdTest, MoveAssignFromInvalid)
{
    int fd = MakeFd();
    cfl::UniqueFd src;
    cfl::UniqueFd dst(fd);

    dst = std::move(src);

    EXPECT_FALSE(dst.IsValid());
    EXPECT_EQ(dst.GetFd(), -1);
}

// ============================================================
// GetFd
// ============================================================
TEST(UniqueFdTest, GetFd)
{
    int fd = MakeFd();
    cfl::UniqueFd ufd(fd);
    EXPECT_EQ(ufd.GetFd(), fd);
}

TEST(UniqueFdTest, GetFdInvalid)
{
    cfl::UniqueFd ufd;
    EXPECT_EQ(ufd.GetFd(), -1);
}

// ============================================================
// operator bool / IsValid
// ============================================================
TEST(UniqueFdTest, BoolTrue)
{
    int fd = MakeFd();
    cfl::UniqueFd ufd(fd);
    EXPECT_TRUE(ufd);
    EXPECT_TRUE(ufd.IsValid());
}

TEST(UniqueFdTest, BoolFalse)
{
    cfl::UniqueFd ufd;
    EXPECT_FALSE(ufd);
    EXPECT_FALSE(ufd.IsValid());
}

// ============================================================
// Release
// ============================================================
TEST(UniqueFdTest, Release)
{
    int fd = MakeFd();
    cfl::UniqueFd ufd(fd);

    int released = ufd.Release();

    EXPECT_EQ(released, fd);
    EXPECT_FALSE(ufd.IsValid());
    EXPECT_EQ(ufd.GetFd(), -1);

    // fd 仍然有效，需要手动关闭
    EXPECT_EQ(fcntl(fd, F_GETFD), 0);
    close(fd);
}

TEST(UniqueFdTest, ReleaseInvalid)
{
    cfl::UniqueFd ufd;
    int released = ufd.Release();
    EXPECT_EQ(released, -1);
}

// ============================================================
// Reset
// ============================================================
TEST(UniqueFdTest, ResetFromInvalid)
{
    int fd = MakeFd();
    cfl::UniqueFd ufd;

    ufd.Reset(fd);

    EXPECT_TRUE(ufd.IsValid());
    EXPECT_EQ(ufd.GetFd(), fd);
}

TEST(UniqueFdTest, ResetToInvalid)
{
    int fd = MakeFd();
    cfl::UniqueFd ufd(fd);

    ufd.Reset(-1);

    EXPECT_FALSE(ufd.IsValid());
    EXPECT_EQ(ufd.GetFd(), -1);
}

TEST(UniqueFdTest, ResetToNewFd)
{
    int fd1 = MakeFd();
    int fd2 = MakeFd();
    cfl::UniqueFd ufd(fd1);

    ufd.Reset(fd2);

    EXPECT_TRUE(ufd.IsValid());
    EXPECT_EQ(ufd.GetFd(), fd2);

    // fd1 应该已经被关闭
    EXPECT_NE(fcntl(fd1, F_GETFD), 0);
}

TEST(UniqueFdTest, ResetToSameFd)
{
    int fd = MakeFd();
    cfl::UniqueFd ufd(fd);

    // 重置为同一个 fd，不应关闭
    ufd.Reset(fd);

    EXPECT_TRUE(ufd.IsValid());
    EXPECT_EQ(ufd.GetFd(), fd);
    EXPECT_EQ(fcntl(fd, F_GETFD), 0);
}

// ============================================================
// 析构
// ============================================================
TEST(UniqueFdTest, DestructClosesFd)
{
    int fd = MakeFd();
    EXPECT_EQ(fcntl(fd, F_GETFD), 0);  // fd 有效

    {
        cfl::UniqueFd ufd(fd);
    }  // 析构

    // fd 应该已经被关闭
    EXPECT_NE(fcntl(fd, F_GETFD), 0);
}

TEST(UniqueFdTest, DestructInvalidNoop)
{
    // 不应崩溃
    cfl::UniqueFd ufd;
}

TEST(UniqueFdTest, MoveConstructDestructClosesCorrectly)
{
    int fd = MakeFd();

    {
        cfl::UniqueFd src(fd);
        cfl::UniqueFd dst(std::move(src));
        // src 析构时不应关闭 fd（已移交）
        // dst 析构时应关闭 fd
    }

    EXPECT_NE(fcntl(fd, F_GETFD), 0);
}

// ============================================================
// 拷贝禁止（编译期验证，以下代码不应编译）
// ============================================================
// TEST(UniqueFdTest, CopyConstructDeleted)
// {
//     cfl::UniqueFd ufd(MakeFd());
//     cfl::UniqueFd copy(ufd);  // 编译错误
// }
//
// TEST(UniqueFdTest, CopyAssignDeleted)
// {
//     cfl::UniqueFd ufd1(MakeFd());
//     cfl::UniqueFd ufd2(MakeFd());
//     ufd1 = ufd2;  // 编译错误
// }
