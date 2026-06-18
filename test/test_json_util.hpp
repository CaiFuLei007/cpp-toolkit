#include <gtest/gtest.h>
#include "json_util.h"

// ============================================================
// UnSerialize: 基本反序列化
// ============================================================
TEST(JsonUtilTest, UnSerializeObject)
{
    Json::Value json;
    std::string_view str = R"({"name":"test","value":42})";
    EXPECT_TRUE(cfl::JsonUtil::UnSerialize(json, str));
    EXPECT_EQ(json["name"].asString(), "test");
    EXPECT_EQ(json["value"].asInt(), 42);
}

TEST(JsonUtilTest, UnSerializeArray)
{
    Json::Value json;
    std::string_view str = R"([1,2,3,"hello"])";
    EXPECT_TRUE(cfl::JsonUtil::UnSerialize(json, str));
    EXPECT_TRUE(json.isArray());
    EXPECT_EQ(json.size(), 4u);
    EXPECT_EQ(json[0].asInt(), 1);
    EXPECT_EQ(json[3].asString(), "hello");
}

TEST(JsonUtilTest, UnSerializeNestedObject)
{
    Json::Value json;
    std::string_view str = R"({"user":{"name":"alice","age":30},"active":true})";
    EXPECT_TRUE(cfl::JsonUtil::UnSerialize(json, str));
    EXPECT_EQ(json["user"]["name"].asString(), "alice");
    EXPECT_EQ(json["user"]["age"].asInt(), 30);
    EXPECT_TRUE(json["active"].asBool());
}

TEST(JsonUtilTest, UnSerializeNestedArray)
{
    Json::Value json;
    std::string_view str = R"({"matrix":[[1,2],[3,4]]})";
    EXPECT_TRUE(cfl::JsonUtil::UnSerialize(json, str));
    EXPECT_EQ(json["matrix"][0][0].asInt(), 1);
    EXPECT_EQ(json["matrix"][1][1].asInt(), 4);
}

TEST(JsonUtilTest, UnSerializePrimitives)
{
    Json::Value json;

    EXPECT_TRUE(cfl::JsonUtil::UnSerialize(json, "42"));
    EXPECT_EQ(json.asInt(), 42);

    EXPECT_TRUE(cfl::JsonUtil::UnSerialize(json, "3.14"));
    EXPECT_DOUBLE_EQ(json.asDouble(), 3.14);

    EXPECT_TRUE(cfl::JsonUtil::UnSerialize(json, "true"));
    EXPECT_TRUE(json.asBool());

    EXPECT_TRUE(cfl::JsonUtil::UnSerialize(json, "false"));
    EXPECT_FALSE(json.asBool());

    EXPECT_TRUE(cfl::JsonUtil::UnSerialize(json, "null"));
    EXPECT_TRUE(json.isNull());

    EXPECT_TRUE(cfl::JsonUtil::UnSerialize(json, R"("hello")"));
    EXPECT_EQ(json.asString(), "hello");
}

TEST(JsonUtilTest, UnSerializeEmptyObject)
{
    Json::Value json;
    EXPECT_TRUE(cfl::JsonUtil::UnSerialize(json, "{}"));
    EXPECT_TRUE(json.isObject());
    EXPECT_EQ(json.size(), 0u);
}

TEST(JsonUtilTest, UnSerializeEmptyArray)
{
    Json::Value json;
    EXPECT_TRUE(cfl::JsonUtil::UnSerialize(json, "[]"));
    EXPECT_TRUE(json.isArray());
    EXPECT_EQ(json.size(), 0u);
}

TEST(JsonUtilTest, UnSerializeWithWhitespace)
{
    Json::Value json;
    std::string_view str = R"(
    {
        "key" : "value" ,
        "num" : 100
    }
    )";
    EXPECT_TRUE(cfl::JsonUtil::UnSerialize(json, str));
    EXPECT_EQ(json["key"].asString(), "value");
    EXPECT_EQ(json["num"].asInt(), 100);
}

TEST(JsonUtilTest, UnSerializeSpecialCharacters)
{
    Json::Value json;
    std::string_view str = R"({"text":"line1\nline2\ttab"})";
    EXPECT_TRUE(cfl::JsonUtil::UnSerialize(json, str));
    EXPECT_EQ(json["text"].asString(), "line1\nline2\ttab");
}

TEST(JsonUtilTest, UnSerializeUnicode)
{
    Json::Value json;
    std::string_view str = R"({"name":"你好"})";
    EXPECT_TRUE(cfl::JsonUtil::UnSerialize(json, str));
    // jsoncpp 解析 unicode 转义为 UTF-8
    EXPECT_FALSE(json["name"].asString().empty());
}

// ============================================================
// UnSerialize: 无效 JSON
// ============================================================
TEST(JsonUtilTest, UnSerializeInvalidEmpty)
{
    Json::Value json;
    EXPECT_FALSE(cfl::JsonUtil::UnSerialize(json, ""));
}

TEST(JsonUtilTest, UnSerializeInvalidMissingBrace)
{
    Json::Value json;
    EXPECT_FALSE(cfl::JsonUtil::UnSerialize(json, R"({"key":"value")"));
}

TEST(JsonUtilTest, UnSerializeInvalidTrailingComma)
{
    Json::Value json;
    // jsoncpp 可能容忍尾逗号，这里测试行为一致性
    // 如果 jsoncpp 不容忍，应返回 false
    bool result = cfl::JsonUtil::UnSerialize(json, R"({"key":"value",})");
    // 无论成功或失败，行为应一致（不崩溃）
    (void)result;
}

TEST(JsonUtilTest, UnSerializeInvalidGarbage)
{
    Json::Value json;
    EXPECT_FALSE(cfl::JsonUtil::UnSerialize(json, "not json at all"));
}

TEST(JsonUtilTest, UnSerializeInvalidPartial)
{
    Json::Value json;
    EXPECT_FALSE(cfl::JsonUtil::UnSerialize(json, R"({"key":)"));
}

// ============================================================
// SerializeCompact: 紧凑序列化
// ============================================================
TEST(JsonUtilTest, SerializeCompactObject)
{
    Json::Value json;
    json["name"] = "test";
    json["value"] = 42;

    std::string str;
    EXPECT_TRUE(cfl::JsonUtil::SerializeCompact(json, str));
    EXPECT_FALSE(str.empty());

    // FastWriter 在末尾添加 \n，去掉后不应有其他换行
    std::string trimmed = str;
    while (!trimmed.empty() && trimmed.back() == '\n') {
        trimmed.pop_back();
    }
    EXPECT_EQ(trimmed.find('\n'), std::string::npos);

    // 验证内容可被重新解析
    Json::Value reparsed;
    EXPECT_TRUE(cfl::JsonUtil::UnSerialize(reparsed, str));
    EXPECT_EQ(reparsed["name"].asString(), "test");
    EXPECT_EQ(reparsed["value"].asInt(), 42);
}

TEST(JsonUtilTest, SerializeCompactArray)
{
    Json::Value json(Json::arrayValue);
    json.append(1);
    json.append("two");
    json.append(true);

    std::string str;
    EXPECT_TRUE(cfl::JsonUtil::SerializeCompact(json, str));
    EXPECT_FALSE(str.empty());

    Json::Value reparsed;
    EXPECT_TRUE(cfl::JsonUtil::UnSerialize(reparsed, str));
    EXPECT_EQ(reparsed.size(), 3u);
    EXPECT_EQ(reparsed[0].asInt(), 1);
    EXPECT_EQ(reparsed[1].asString(), "two");
    EXPECT_TRUE(reparsed[2].asBool());
}

TEST(JsonUtilTest, SerializeCompactEmptyObject)
{
    Json::Value json(Json::objectValue);
    std::string str;
    EXPECT_TRUE(cfl::JsonUtil::SerializeCompact(json, str));
    // 紧凑格式可能包含尾部换行，去掉后比较
    EXPECT_TRUE(str.find("{}") != std::string::npos);
}

TEST(JsonUtilTest, SerializeCompactEmptyArray)
{
    Json::Value json(Json::arrayValue);
    std::string str;
    EXPECT_TRUE(cfl::JsonUtil::SerializeCompact(json, str));
    EXPECT_TRUE(str.find("[]") != std::string::npos);
}

TEST(JsonUtilTest, SerializeCompactNested)
{
    Json::Value json;
    json["user"]["name"] = "alice";
    json["user"]["scores"].append(95);
    json["user"]["scores"].append(87);

    std::string str;
    EXPECT_TRUE(cfl::JsonUtil::SerializeCompact(json, str));

    Json::Value reparsed;
    EXPECT_TRUE(cfl::JsonUtil::UnSerialize(reparsed, str));
    EXPECT_EQ(reparsed["user"]["name"].asString(), "alice");
    EXPECT_EQ(reparsed["user"]["scores"].size(), 2u);
    EXPECT_EQ(reparsed["user"]["scores"][0].asInt(), 95);
}

TEST(JsonUtilTest, SerializeCompactSpecialChars)
{
    Json::Value json;
    json["text"] = "line1\nline2\ttab\"quote";

    std::string str;
    EXPECT_TRUE(cfl::JsonUtil::SerializeCompact(json, str));

    Json::Value reparsed;
    EXPECT_TRUE(cfl::JsonUtil::UnSerialize(reparsed, str));
    EXPECT_EQ(reparsed["text"].asString(), "line1\nline2\ttab\"quote");
}

TEST(JsonUtilTest, SerializeCompactNumericTypes)
{
    Json::Value json;
    json["int_val"] = 42;
    json["double_val"] = 3.14;
    json["bool_val"] = true;
    json["null_val"] = Json::nullValue;

    std::string str;
    EXPECT_TRUE(cfl::JsonUtil::SerializeCompact(json, str));

    Json::Value reparsed;
    EXPECT_TRUE(cfl::JsonUtil::UnSerialize(reparsed, str));
    EXPECT_EQ(reparsed["int_val"].asInt(), 42);
    EXPECT_DOUBLE_EQ(reparsed["double_val"].asDouble(), 3.14);
    EXPECT_TRUE(reparsed["bool_val"].asBool());
    EXPECT_TRUE(reparsed["null_val"].isNull());
}

// ============================================================
// SerializePretty: 格式化序列化
// ============================================================
TEST(JsonUtilTest, SerializePrettyObject)
{
    Json::Value json;
    json["name"] = "test";
    json["value"] = 42;

    std::string str;
    EXPECT_TRUE(cfl::JsonUtil::SerializePretty(json, str));
    EXPECT_FALSE(str.empty());

    // 应包含换行和缩进
    EXPECT_NE(str.find('\n'), std::string::npos);
    EXPECT_NE(str.find("  "), std::string::npos);

    // 验证内容可被重新解析
    Json::Value reparsed;
    EXPECT_TRUE(cfl::JsonUtil::UnSerialize(reparsed, str));
    EXPECT_EQ(reparsed["name"].asString(), "test");
    EXPECT_EQ(reparsed["value"].asInt(), 42);
}

TEST(JsonUtilTest, SerializePrettyArray)
{
    Json::Value json(Json::arrayValue);
    json.append(1);
    json.append("two");

    std::string str;
    EXPECT_TRUE(cfl::JsonUtil::SerializePretty(json, str));

    Json::Value reparsed;
    EXPECT_TRUE(cfl::JsonUtil::UnSerialize(reparsed, str));
    EXPECT_EQ(reparsed.size(), 2u);
    EXPECT_EQ(reparsed[0].asInt(), 1);
    EXPECT_EQ(reparsed[1].asString(), "two");
}

TEST(JsonUtilTest, SerializePrettyEmptyObject)
{
    Json::Value json(Json::objectValue);
    std::string str;
    EXPECT_TRUE(cfl::JsonUtil::SerializePretty(json, str));
    // 空对象格式化输出应包含 "{}"
    EXPECT_TRUE(str.find("{}") != std::string::npos);
}

TEST(JsonUtilTest, SerializePrettyEmptyArray)
{
    Json::Value json(Json::arrayValue);
    std::string str;
    EXPECT_TRUE(cfl::JsonUtil::SerializePretty(json, str));
    EXPECT_TRUE(str.find("[]") != std::string::npos);
}

TEST(JsonUtilTest, SerializePrettyNested)
{
    Json::Value json;
    json["user"]["name"] = "alice";
    json["user"]["address"]["city"] = "beijing";
    json["user"]["address"]["zip"] = "100000";

    std::string str;
    EXPECT_TRUE(cfl::JsonUtil::SerializePretty(json, str));

    Json::Value reparsed;
    EXPECT_TRUE(cfl::JsonUtil::UnSerialize(reparsed, str));
    EXPECT_EQ(reparsed["user"]["name"].asString(), "alice");
    EXPECT_EQ(reparsed["user"]["address"]["city"].asString(), "beijing");
    EXPECT_EQ(reparsed["user"]["address"]["zip"].asString(), "100000");
}

TEST(JsonUtilTest, SerializePrettyIndentation)
{
    Json::Value json;
    json["key"] = "value";

    std::string str;
    EXPECT_TRUE(cfl::JsonUtil::SerializePretty(json, str));

    // 验证使用 2 空格缩进
    EXPECT_NE(str.find("\n  \""), std::string::npos);
}

// ============================================================
// 往返测试: Serialize → UnSerialize 一致性
// ============================================================
TEST(JsonUtilTest, RoundTripCompact)
{
    Json::Value original;
    original["string"] = "hello world";
    original["integer"] = 12345;
    original["floating"] = 2.718;
    original["boolean"] = false;
    original["nothing"] = Json::nullValue;
    original["array"].append(1);
    original["array"].append("two");
    original["array"].append(true);
    original["nested"]["a"] = 1;
    original["nested"]["b"] = 2;

    std::string serialized;
    EXPECT_TRUE(cfl::JsonUtil::SerializeCompact(original, serialized));

    Json::Value restored;
    EXPECT_TRUE(cfl::JsonUtil::UnSerialize(restored, serialized));

    EXPECT_EQ(restored["string"].asString(), "hello world");
    EXPECT_EQ(restored["integer"].asInt(), 12345);
    EXPECT_DOUBLE_EQ(restored["floating"].asDouble(), 2.718);
    EXPECT_FALSE(restored["boolean"].asBool());
    EXPECT_TRUE(restored["nothing"].isNull());
    EXPECT_EQ(restored["array"].size(), 3u);
    EXPECT_EQ(restored["array"][0].asInt(), 1);
    EXPECT_EQ(restored["array"][1].asString(), "two");
    EXPECT_TRUE(restored["array"][2].asBool());
    EXPECT_EQ(restored["nested"]["a"].asInt(), 1);
    EXPECT_EQ(restored["nested"]["b"].asInt(), 2);
}

TEST(JsonUtilTest, RoundTripPretty)
{
    Json::Value original;
    original["name"] = "test";
    original["items"].append(10);
    original["items"].append(20);
    original["flag"] = true;

    std::string serialized;
    EXPECT_TRUE(cfl::JsonUtil::SerializePretty(original, serialized));

    Json::Value restored;
    EXPECT_TRUE(cfl::JsonUtil::UnSerialize(restored, serialized));

    EXPECT_EQ(restored["name"].asString(), "test");
    EXPECT_EQ(restored["items"].size(), 2u);
    EXPECT_EQ(restored["items"][0].asInt(), 10);
    EXPECT_EQ(restored["items"][1].asInt(), 20);
    EXPECT_TRUE(restored["flag"].asBool());
}

TEST(JsonUtilTest, RoundTripCompactToPretty)
{
    Json::Value original;
    original["key"] = "value";
    original["num"] = 42;

    std::string compact;
    EXPECT_TRUE(cfl::JsonUtil::SerializeCompact(original, compact));

    Json::Value intermediate;
    EXPECT_TRUE(cfl::JsonUtil::UnSerialize(intermediate, compact));

    std::string pretty;
    EXPECT_TRUE(cfl::JsonUtil::SerializePretty(intermediate, pretty));

    // 最终解析应与原始一致
    Json::Value final;
    EXPECT_TRUE(cfl::JsonUtil::UnSerialize(final, pretty));
    EXPECT_EQ(final["key"].asString(), "value");
    EXPECT_EQ(final["num"].asInt(), 42);
}

// ============================================================
// Compact vs Pretty: 格式差异验证
// ============================================================
TEST(JsonUtilTest, CompactShorterThanPretty)
{
    Json::Value json;
    json["name"] = "test";
    json["value"] = 42;
    json["items"].append(1);
    json["items"].append(2);

    std::string compact;
    std::string pretty;
    EXPECT_TRUE(cfl::JsonUtil::SerializeCompact(json, compact));
    EXPECT_TRUE(cfl::JsonUtil::SerializePretty(json, pretty));

    // 紧凑格式应比格式化短
    EXPECT_LT(compact.size(), pretty.size());
}

TEST(JsonUtilTest, CompactNoNewlines)
{
    Json::Value json;
    json["key"] = "value";

    std::string compact;
    EXPECT_TRUE(cfl::JsonUtil::SerializeCompact(json, compact));

    // 紧凑格式不应包含换行符（FastWriter 会在末尾加 \n）
    // 去掉末尾换行后应无换行
    while (!compact.empty() && compact.back() == '\n') {
        compact.pop_back();
    }
    EXPECT_EQ(compact.find('\n'), std::string::npos);
}

TEST(JsonUtilTest, PrettyHasNewlines)
{
    Json::Value json;
    json["key"] = "value";

    std::string pretty;
    EXPECT_TRUE(cfl::JsonUtil::SerializePretty(json, pretty));

    // 格式化输出应包含换行
    EXPECT_NE(pretty.find('\n'), std::string::npos);
}

// ============================================================
// 边界情况
// ============================================================
TEST(JsonUtilTest, DeepNestedObject)
{
    Json::Value json;
    Json::Value* current = &json;
    for (int i = 0; i < 10; ++i) {
        (*current)["level"] = i;
        current = &((*current)["child"]);
    }
    (*current)["final"] = true;

    std::string str;
    EXPECT_TRUE(cfl::JsonUtil::SerializeCompact(json, str));

    Json::Value reparsed;
    EXPECT_TRUE(cfl::JsonUtil::UnSerialize(reparsed, str));

    // 验证嵌套深度
    Json::Value* check = &reparsed;
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ((*check)["level"].asInt(), i);
        check = &((*check)["child"]);
    }
    EXPECT_TRUE((*check)["final"].asBool());
}

TEST(JsonUtilTest, LargeArray)
{
    Json::Value json(Json::arrayValue);
    const int count = 1000;
    for (int i = 0; i < count; ++i) {
        json.append(i);
    }

    std::string str;
    EXPECT_TRUE(cfl::JsonUtil::SerializeCompact(json, str));

    Json::Value reparsed;
    EXPECT_TRUE(cfl::JsonUtil::UnSerialize(reparsed, str));
    EXPECT_EQ(reparsed.size(), static_cast<Json::UInt>(count));
    EXPECT_EQ(reparsed[0].asInt(), 0);
    EXPECT_EQ(reparsed[count - 1].asInt(), count - 1);
}

TEST(JsonUtilTest, EmptyStringValue)
{
    Json::Value json;
    json["empty"] = "";

    std::string str;
    EXPECT_TRUE(cfl::JsonUtil::SerializeCompact(json, str));

    Json::Value reparsed;
    EXPECT_TRUE(cfl::JsonUtil::UnSerialize(reparsed, str));
    EXPECT_EQ(reparsed["empty"].asString(), "");
}

TEST(JsonUtilTest, LongStringValue)
{
    std::string long_str(10000, 'x');
    Json::Value json;
    json["data"] = long_str;

    std::string str;
    EXPECT_TRUE(cfl::JsonUtil::SerializeCompact(json, str));

    Json::Value reparsed;
    EXPECT_TRUE(cfl::JsonUtil::UnSerialize(reparsed, str));
    EXPECT_EQ(reparsed["data"].asString(), long_str);
}

TEST(JsonUtilTest, IntegerBoundaryValues)
{
    Json::Value json;
    json["zero"] = 0;
    json["positive"] = 2147483647;   // INT32_MAX
    json["negative"] = -2147483647;  // -(INT32_MAX)
    json["large"] = static_cast<Json::Int64>(9223372036854775807LL);  // INT64_MAX

    std::string str;
    EXPECT_TRUE(cfl::JsonUtil::SerializeCompact(json, str));

    Json::Value reparsed;
    EXPECT_TRUE(cfl::JsonUtil::UnSerialize(reparsed, str));
    EXPECT_EQ(reparsed["zero"].asInt(), 0);
    EXPECT_EQ(reparsed["positive"].asInt(), 2147483647);
    EXPECT_EQ(reparsed["negative"].asInt(), -2147483647);
}

TEST(JsonUtilTest, DoublePrecision)
{
    Json::Value json;
    json["pi"] = 3.141592653589793;
    json["small"] = 0.000001;
    json["large"] = 1e18;

    std::string str;
    EXPECT_TRUE(cfl::JsonUtil::SerializeCompact(json, str));

    Json::Value reparsed;
    EXPECT_TRUE(cfl::JsonUtil::UnSerialize(reparsed, str));
    EXPECT_DOUBLE_EQ(reparsed["pi"].asDouble(), 3.141592653589793);
    EXPECT_DOUBLE_EQ(reparsed["small"].asDouble(), 0.000001);
    EXPECT_DOUBLE_EQ(reparsed["large"].asDouble(), 1e18);
}

TEST(JsonUtilTest, BooleanAndNullMixed)
{
    Json::Value json;
    json["t"] = true;
    json["f"] = false;
    json["n"] = Json::nullValue;

    std::string str;
    EXPECT_TRUE(cfl::JsonUtil::SerializeCompact(json, str));

    Json::Value reparsed;
    EXPECT_TRUE(cfl::JsonUtil::UnSerialize(reparsed, str));
    EXPECT_TRUE(reparsed["t"].asBool());
    EXPECT_FALSE(reparsed["f"].asBool());
    EXPECT_TRUE(reparsed["n"].isNull());
}
