
#pragma once

/**
 * json 工具类
 */

#include <string>
#include <string_view>
#include <jsoncpp/json/json.h>

namespace cfl
{
class JsonUtil
{
public:
    static bool UnSerialize(Json::Value& json, std::string_view str);
    static bool SerializeCompact(const Json::Value& json, std::string& str);
    static bool SerializePretty(const Json::Value& json, std::string& str);
};

} // namespace cfl