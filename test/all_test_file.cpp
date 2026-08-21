#include <gtest/gtest.h>
// #include "test_logger.hpp"
#include "test_fdfs.hpp"

// file 模式: initLogger(g_log_file, ...),日志直接输出到文件
namespace cpp_toolkit_test {
bool g_use_stdout = false;
std::string g_log_file = "/tmp/cpp_toolkit_test_file.log";
} // namespace cpp_toolkit_test

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
