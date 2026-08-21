#include <gtest/gtest.h>
// #include "test_unique_fd.hpp"
// #include "test_socket.hpp"
// #include "test_epoll.hpp"
// #include "test_json_util.hpp"
// #include "test_timerwheel.hpp"
#include "test_logger.hpp"
#include "test_fdfs.hpp"

// stdout 模式: initLogger("stdout", ...),stdout 被重定向到 g_log_file
namespace cpp_toolkit_test {
bool g_use_stdout = true;
std::string g_log_file = "/tmp/cpp_toolkit_test_stdout.log";
} // namespace cpp_toolkit_test

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    // stdout 模式下 stdout 被重定向到 g_log_file,gtest 的详细输出也在文件中
    // 在 stderr 上输出摘要,让用户在终端看到测试结果
    std::cerr << "[stdout mode] stdout redirected to: " << cpp_toolkit_test::g_log_file
              << "\n[stdout mode] detailed gtest+log output will be written there\n";
    int result = RUN_ALL_TESTS();
    std::cerr << "\n[stdout mode] Test result: "
              << (result == 0 ? "ALL PASSED" : "FAILED") << "\n";
    std::cerr << "[stdout mode] inspect output: cat " << cpp_toolkit_test::g_log_file << "\n";
    return result;
}
