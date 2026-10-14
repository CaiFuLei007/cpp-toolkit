
#pragma once

/**
 * json 工具类
 */

#include <string>
#include <string_view>
#include <cstdint>
#include <chrono>
#include <jsoncpp/json/json.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/syscall.h>
#endif

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

/**
 * 禁止拷贝构造与拷贝赋值的基类
 */
class Noncopyable
{
public:
    Noncopyable() = default;
    ~Noncopyable() = default;
    Noncopyable(const Noncopyable&) = delete;
    Noncopyable& operator=(const Noncopyable&) = delete;
};

/**
 * 单例模板 , T 需可默认构造 ; C++11 起静态局部变量初始化线程安全
 */
template<class T>
class Singleton
{
public:
    static T& GetInstance()
    {
        static T instance;
        return instance;
    }
};

/**
 * 获取当前线程 ID ( Windows 返回线程 ID , Linux 返回内核 tid )
 */
inline unsigned long GetThreadId()
{
#ifdef _WIN32
    return static_cast<unsigned long>(::GetCurrentThreadId());
#elif defined(__linux__)
    static thread_local unsigned long tid = static_cast<unsigned long>(::syscall(SYS_gettid));
    return tid;
#else
    #error "Unsupported platform"
#endif
}

/**
 * 获取当前墙钟时间 , 单位 : 毫秒 ( 与 CLOCK_REALTIME 语义一致 )
 */
inline uint64_t GetCurrentMs()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}

} // namespace cpp_toolkit