
#pragma once

/*
    - 对 rpc 进行二次封装
    1. 对 channel 进行封装 : 管理所有提供服务的节点
    2. 对 closure 进行封装 , 可以使用 lambda 表达式 来定义服务
    3. 创建 ServerFactory , 用于创建 rpc 服务
*/


#include <brpc/channel.h>
#include <brpc/server.h>
#include <mutex>
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include <functional>

namespace cpp_toolkit {

using ChannelPtr = std::shared_ptr<brpc::Channel>;

class Channels
{
public:
    using Ptr = std::shared_ptr<Channels>;
private:
    std::mutex mutex_;
    size_t index_;
    std::vector<ChannelPtr> channels_nums_;
    std::unordered_map<std::string, ChannelPtr> channels_hash_;

public:
    Channels()
    : index_(0)
    {
    }
    ~Channels() {}

    bool AddChannel(const std::string& addr_ip_and_port);
    void DelChannel(const std::string& addr_ip_and_port);
    ChannelPtr GetChannel();
};


class ChannelManager
{
public:
    using Ptr = std::shared_ptr<ChannelManager>;
private:
    std::mutex mutex_;
    std::unordered_map<std::string, Channels::Ptr> channels_;
public:
    ChannelManager() {}
    ~ChannelManager() {}

    void AddService(const std::string& service_name, const std::string& addr_ip_and_port);
    void DelService(const std::string& service_name, const std::string& addr_ip_and_port);
    ChannelPtr GetChannel(const std::string& service_name);
};

class ClosureFactory
{
public:
    using Callback = std::function<void()>;
    using Ptr = std::shared_ptr<ClosureFactory>;
private:
    struct CallbackObject
    {
        Callback callback;
    };
    static void ClosureCallback(std::shared_ptr<CallbackObject> callback_object);
public:
    ClosureFactory() {}
    ~ClosureFactory() {}

    static google::protobuf::Closure* CreateClosure(Callback&& callback);
};


class ServerFactory
{
public:
    using Ptr = std::shared_ptr<ServerFactory>;
public:
    ServerFactory() {}
    ~ServerFactory() {}

    static std::shared_ptr<brpc::Server> CreateServer(int port, google::protobuf::Service* service);
};

} // namespace cpp_toolkit