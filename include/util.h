
#pragma once

/**
 * json 工具类
 */

#include <string>
#include <string_view>
#include <jsoncpp/json/json.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace cpp_toolkit
{
class JsonUtil
{
public:
    static bool UnSerialize(Json::Value& json, std::string_view str);
    static bool SerializeCompact(const Json::Value& json, std::string& str);
    static bool SerializePretty(const Json::Value& json, std::string& str);
};

class UuidUtil
{
public:
    static std::string GenerateUuidV4();
    static std::string GenerateUuidV3(const std::string& str , boost::uuids::uuid namespace_uuid = boost::uuids::ns::dns());
};

} // namespace cpp_toolkit