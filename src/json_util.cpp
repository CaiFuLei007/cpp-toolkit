
#include "json_util.h"

namespace cfl
{
bool JsonUtil::UnSerialize(Json::Value& json, std::string_view str)
{
    Json::Reader reader;
    return reader.parse(str.data(), json);
}
bool JsonUtil::SerializeCompact(const Json::Value& json, std::string& str)
{   
    Json::FastWriter writer;
    str = writer.write(json);
    return true;
}

bool JsonUtil::SerializePretty(const Json::Value& json, std::string& str)
{
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "  ";  // 2空格缩进
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    std::ostringstream os;
    writer->write(json, &os);
    str = os.str();
    return true;
}

} // namespace cfl