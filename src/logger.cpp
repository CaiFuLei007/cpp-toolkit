
#include "logger.h"

namespace cpp_toolkit {

std::shared_ptr<spdlog::logger> Logger::_logger = nullptr;

Logger::Logger()
{}

void Logger::initLogger(const logger_settings& settings)
{
    spdlog::flush_on(settings.logLevel);

    if(settings.async) // 异步日志
    {
        if("stdout" == settings.loggerFile)
        {
            _logger = spdlog::stdout_color_mt<spdlog::async_factory>(settings.loggerName);
        }   
        else
        {
            _logger = spdlog::basic_logger_mt<spdlog::async_factory>(settings.loggerName, settings.loggerFile);
        }
        spdlog::init_thread_pool(settings.queue_size, settings.thread_count);
    }
    else 
    {
        if("stdout" == settings.loggerFile)
        {
            _logger = spdlog::stdout_color_mt(settings.loggerName);
        }
        else
        {
            _logger = spdlog::basic_logger_mt(settings.loggerName, settings.loggerFile);
        }
    }
    
    _logger->set_pattern(settings.pattern);
    _logger->set_level(settings.logLevel);
}

std::shared_ptr<spdlog::logger> Logger::getLogger(){
    return _logger;
}

} // namespace cpp_toolkit
