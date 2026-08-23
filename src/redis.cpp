
#include "redis.h"
#include <sw/redis++/connection.h>

namespace cpp_toolkit {
std::shared_ptr<sw::redis::Redis> RedisFactory::Create(const RedisSettings& settings) 
{
    sw::redis::ConnectionOptions options;
    options.host = settings.host;
    options.port = settings.port;
    options.user = settings.user;
    options.password = settings.password;
    options.db = settings.db;
    options.connect_timeout = std::chrono::seconds(settings.connect_timeout);

    sw::redis::ConnectionPoolOptions pool_options;
    pool_options.size = settings.pool_connections_size;

    return std::make_shared<sw::redis::Redis>(options, pool_options);
}


} // namespace cpp_toolkit
