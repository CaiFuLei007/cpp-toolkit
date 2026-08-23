
#include "util.h"

namespace cpp_toolkit
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

std::string UuidUtil::GenerateUuidV4()
{
    boost::uuids::random_generator gen;
    boost::uuids::uuid id = gen(); 
    return boost::uuids::to_string(id);
}

std::string UuidUtil::GenerateUuidV3(const std::string& str , boost::uuids::uuid namespace_uuid)
{
     boost::uuids::name_generator_sha1 gen(namespace_uuid);
    boost::uuids::uuid id = gen(str);
    return boost::uuids::to_string(id);
}


} // namespace cpp_toolkit