
#include "odb.h"

namespace cpp_toolkit
{
std::shared_ptr<odb::database> ODBFactory::Create(const MySQLSettings& settings)
{
    std::unique_ptr<odb::mysql::connection_factory> pool(new odb::mysql::connection_pool_factory(settings.connection_pool_size));
    auto handle = std::make_shared<odb::mysql::database>(
        settings.user.c_str(), settings.password.c_str(), settings.database.c_str(), 
        settings.host.c_str(), settings.port, nullptr ,settings.charset.c_str(), 0 , std::move(pool));
    return handle;
}



} // namespace cpp_toolkit
