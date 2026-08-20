
#pragma once

/*
    对 spdlog 进行封装 , 使用之前需已经安装完成 spdlog 库
    initLogger 进行配置 , 输出位置 , 输出文件 , 输出级别

*/

#include <spdlog/logger.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/async.h>
#include <memory>
#include <mutex>


namespace cpp_toolkit {

class Logger{
    public:
        static void initLogger(const std::string& loggerFile = "stdout", const std::string& loggerName = "", spdlog::level::level_enum logLevel = spdlog::level::info);
        static std::shared_ptr<spdlog::logger> getLogger();
    private:
        Logger();
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;
    private:
        static std::shared_ptr<spdlog::logger> _logger;
        static std::mutex _mutex;
};


#define TRACE(format, ...) cpp_toolkit::Logger::getLogger()->trace("[{:>10s}:{:<4d}]" format, __FILE__, __LINE__, ##__VA_ARGS__)
#define DBG(format, ...) cpp_toolkit::Logger::getLogger()->debug("[{:>10s}:{:<4d}]" format, __FILE__, __LINE__, ##__VA_ARGS__)
#define INFO(format, ...) cpp_toolkit::Logger::getLogger()->info("[{:>10s}:{:<4d}]" format, __FILE__, __LINE__, ##__VA_ARGS__)
#define WARN(format, ...) cpp_toolkit::Logger::getLogger()->warn("[{:>10s}:{:<4d}]" format, __FILE__, __LINE__, ##__VA_ARGS__)
#define ERR(format, ...) cpp_toolkit::Logger::getLogger()->error("[{:>10s}:{:<4d}]" format, __FILE__, __LINE__, ##__VA_ARGS__)
#define CRIT(format, ...) cpp_toolkit::Logger::getLogger()->critical("[{:>10s}:{:<4d}]" format, __FILE__, __LINE__, ##__VA_ARGS__)


} // namespace cpp_toolkit
