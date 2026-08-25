
#pragma once

/*
    odb 封装
    1. struct MySQLSettings  mysql 连接参数
    2. ODBFactory  odb 工厂类，用于创建 odb handle 句柄
*/

#include <string>
#include <odb/database.hxx>
#include <odb/mysql/transaction.hxx>
#include <odb/mysql/database.hxx>

namespace cpp_toolkit
{

struct MySQLSettings
{
    std::string host;
    unsigned int port = 0;
    std::string user;
    std::string password;
    std::string database;
    std::string charset = "utf8";
    size_t connection_pool_size = 3;
};

class ODBFactory
{
public:
    static std::shared_ptr<odb::database> Create(const MySQLSettings& settings);
};


} // namespace cpp_toolkit
