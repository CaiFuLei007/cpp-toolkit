
#pragma once

/*
    redis 封装
    1. RedisSettings 配置
    2. RedisFactory 工厂类
*/

#include <sw/redis++/redis.h>
#include <sw/redis++/connection.h>
#include <memory>
#include <string>

namespace cpp_toolkit {

struct RedisSettings {
    std::string host;
    int port = 6379;
    std::string user = "default";
    std::string password;
    int db = 0;
    size_t pool_connections_size = 3;
    int connect_timeout = 60;
};

class RedisFactory {
public:
    static std::shared_ptr<sw::redis::Redis> Create(const RedisSettings& settings);

};
} // namespace toolkit
