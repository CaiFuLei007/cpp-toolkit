#pragma once

#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include "logger.h"

#include <chrono>
#include <fstream>
#include <filesystem>
#include <string>
#include <thread>

namespace fs = std::filesystem;

// 由各 all_test*.cpp 设置: 决定 initLogger 的模式与日志文件路径
//   - g_use_stdout = true : initLogger("stdout", ...),并将 stdout 重定向到 g_log_file
//   - g_use_stdout = false: initLogger(g_log_file, ...)
namespace cpp_toolkit_test {
extern bool g_use_stdout;
extern std::string g_log_file;
} // namespace cpp_toolkit_test

namespace {

// 读取文件全部内容
inline std::string ReadFileContent(const std::string& path)
{
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open()) {
        return "";
    }
    return std::string((std::istreambuf_iterator<char>(ifs)),
                       std::istreambuf_iterator<char>());
}

} // anonymous namespace

class LoggerTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // 初始化前: getLogger 应返回 nullptr
        EXPECT_EQ(cpp_toolkit::Logger::getLogger(), nullptr);

        // 清理旧文件
        fs::remove(cpp_toolkit_test::g_log_file);

        if (cpp_toolkit_test::g_use_stdout) {
            // stdout 模式: 将 stdout 重定向到文件以便自动验证
            // 注意: 必须在 initLogger 之前重定向,这样 stdout_color_mt
            //       内部持有的 FILE* 即为重定向后的 stdout
            FILE* fp = std::freopen(cpp_toolkit_test::g_log_file.c_str(), "w", stdout);
            ASSERT_NE(fp, nullptr) << "freopen failed to redirect stdout";
            cpp_toolkit::Logger::initLogger("stdout", "test_logger", spdlog::level::trace);
        } else {
            // file 模式: 直接输出到文件
            cpp_toolkit::Logger::initLogger(cpp_toolkit_test::g_log_file, "test_logger", spdlog::level::trace);
        }
    }

    static void TearDownTestSuite()
    {
        if (auto logger = cpp_toolkit::Logger::getLogger()) {
            logger->flush();
        }
        spdlog::shutdown();
        // 保留日志文件以便查看(stdout 模式下 gtest 输出与日志输出都在其中)
        // 下次 SetUpTestSuite 会重新清空/截断
    }

    void TearDown() override
    {
        // 每个测试结束后刷新缓冲,确保日志已落盘
        if (auto logger = cpp_toolkit::Logger::getLogger()) {
            logger->flush();
        }
    }

    // 读取当前日志文件内容
    static std::string GetLogFileContent()
    {
        return ReadFileContent(cpp_toolkit_test::g_log_file);
    }

    // 刷新并读取日志内容
    // 注意: file 模式下日志器为 async,flush() 仅投递刷新消息即返回,
    // 因此需要轮询等待后台线程将消息落盘
    static std::string WaitForContent(const std::string& expected,
                                       int timeout_ms = 1000)
    {
        auto logger = cpp_toolkit::Logger::getLogger();
        std::string content;
        const auto deadline = std::chrono::steady_clock::now()
                              + std::chrono::milliseconds(timeout_ms);
        do {
            if (logger) {
                logger->flush();
            }
            content = GetLogFileContent();
            if (content.find(expected) != std::string::npos) {
                return content;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        } while (std::chrono::steady_clock::now() < deadline);
        // 超时后返回最后一次读取的内容,让上层断言失败时能看到实际内容
        if (logger) {
            logger->flush();
        }
        return GetLogFileContent();
    }

    // 刷新并读取(用于不需要等待特定内容的场景)
    static std::string FlushAndRead()
    {
        if (auto logger = cpp_toolkit::Logger::getLogger()) {
            logger->flush();
        }
        // 给异步后台线程一点时间落盘
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        return GetLogFileContent();
    }
};

// ============================================================
// initLogger + getLogger: 基本初始化
// ============================================================
TEST_F(LoggerTest, GetLoggerNotNullAfterInit)
{
    auto logger = cpp_toolkit::Logger::getLogger();
    ASSERT_NE(logger, nullptr);
}

TEST_F(LoggerTest, LoggerName)
{
    auto logger = cpp_toolkit::Logger::getLogger();
    ASSERT_NE(logger, nullptr);
    EXPECT_EQ(logger->name(), "test_logger");
}

TEST_F(LoggerTest, LoggerLevel)
{
    auto logger = cpp_toolkit::Logger::getLogger();
    ASSERT_NE(logger, nullptr);
    EXPECT_EQ(logger->level(), spdlog::level::trace);
}

// ============================================================
// 基础功能: 日志能正常输出到终端(stdout 模式)/文件(file 模式)
// ============================================================
TEST_F(LoggerTest, LogOutputWritten)
{
    INFO("log_output_test_msg");
    std::string content = WaitForContent("log_output_test_msg");
    EXPECT_TRUE(content.find("log_output_test_msg") != std::string::npos)
        << "log message missing from output file: " << cpp_toolkit_test::g_log_file;
}

// ============================================================
// 基础功能: 6 个级别宏均可正常输出
// ============================================================
TEST_F(LoggerTest, MacroTrace)
{
    TRACE("macro_trace_msg_{}", 1);
    std::string content = WaitForContent("macro_trace_msg_1");
    EXPECT_TRUE(content.find("macro_trace_msg_1") != std::string::npos)
        << "trace message missing";
}

TEST_F(LoggerTest, MacroDbg)
{
    DBG("macro_dbg_msg_{}", 2);
    std::string content = WaitForContent("macro_dbg_msg_2");
    EXPECT_TRUE(content.find("macro_dbg_msg_2") != std::string::npos)
        << "debug message missing";
}

TEST_F(LoggerTest, MacroInfo)
{
    INFO("macro_info_msg_{}", 3);
    std::string content = WaitForContent("macro_info_msg_3");
    EXPECT_TRUE(content.find("macro_info_msg_3") != std::string::npos)
        << "info message missing";
}

TEST_F(LoggerTest, MacroWarn)
{
    WARN("macro_warn_msg_{}", 4);
    std::string content = WaitForContent("macro_warn_msg_4");
    EXPECT_TRUE(content.find("macro_warn_msg_4") != std::string::npos)
        << "warn message missing";
}

TEST_F(LoggerTest, MacroErr)
{
    ERR("macro_err_msg_{}", 5);
    std::string content = WaitForContent("macro_err_msg_5");
    EXPECT_TRUE(content.find("macro_err_msg_5") != std::string::npos)
        << "error message missing";
}

TEST_F(LoggerTest, MacroCrit)
{
    CRIT("macro_crit_msg_{}", 6);
    std::string content = WaitForContent("macro_crit_msg_6");
    EXPECT_TRUE(content.find("macro_crit_msg_6") != std::string::npos)
        << "critical message missing";
}

// ============================================================
// 基础功能: 日志级别过滤
// 设置 warn 级别,验证 trace/debug/info 被过滤,warn/error/critical 被写入
// ============================================================
TEST_F(LoggerTest, LogLevelFilter)
{
    auto logger = cpp_toolkit::Logger::getLogger();
    ASSERT_NE(logger, nullptr);

    // 先刷新清空,确保读到的是过滤后的内容
    FlushAndRead();

    // 设置 warn 级别
    logger->set_level(spdlog::level::warn);

    // 写入各级别日志
    TRACE("filtered_trace_msg");
    DBG("filtered_dbg_msg");
    INFO("filtered_info_msg");
    WARN("kept_warn_msg");
    ERR("kept_err_msg");
    CRIT("kept_crit_msg");

    // 等待最后一条 critical 落盘,确保所有过滤后的消息都已写入
    std::string content = WaitForContent("kept_crit_msg");

    // warn 以下应被过滤(不写入)
    EXPECT_TRUE(content.find("filtered_trace_msg") == std::string::npos)
        << "trace should be filtered at warn level";
    EXPECT_TRUE(content.find("filtered_dbg_msg") == std::string::npos)
        << "debug should be filtered at warn level";
    EXPECT_TRUE(content.find("filtered_info_msg") == std::string::npos)
        << "info should be filtered at warn level";

    // warn 及以上应被写入
    EXPECT_TRUE(content.find("kept_warn_msg") != std::string::npos)
        << "warn should be written at warn level";
    EXPECT_TRUE(content.find("kept_err_msg") != std::string::npos)
        << "error should be written at warn level";
    EXPECT_TRUE(content.find("kept_crit_msg") != std::string::npos)
        << "critical should be written at warn level";

    // 恢复 trace 级别,避免影响后续测试
    logger->set_level(spdlog::level::trace);
}
