
#include "cfl_rpc.h"
#include <brpc/server.h>

namespace cpp_toolkit {

bool Channels::AddChannel(const std::string& addr_ip_and_port)
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::shared_ptr<brpc::Channel> channel = std::make_shared<brpc::Channel>();
    brpc::ChannelOptions options;
    options.protocol = "baidu_std";
    int ret = channel->Init(addr_ip_and_port.c_str() , &options);
    if(ret != 0)
    {
        return false;
    }
    if (channels_hash_.count(addr_ip_and_port) > 0) {
        return true;
    }
    channels_nums_.push_back(channel);
    channels_hash_.emplace(addr_ip_and_port, channel);
    return true;
}
void Channels::DelChannel(const std::string& addr_ip_and_port)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it_hash = channels_hash_.find(addr_ip_and_port);
    if (it_hash == channels_hash_.end())
    {
        return;
    }
    for(auto it_nums = channels_nums_.begin() ; it_nums != channels_nums_.end() ; it_nums++)
    {
        if (*it_nums == it_hash->second)
        {
            channels_nums_.erase(it_nums);
            break;
        }
    }
    channels_hash_.erase(it_hash);
}
ChannelPtr Channels::GetChannel()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if(channels_nums_.empty())
    {
        return nullptr;
    }
    index_ %= channels_nums_.size();
    size_t i = index_++;
    return channels_nums_[i];
}

void ChannelManager::AddService(const std::string& service_name, const std::string& addr_ip_and_port)
{
    Channels::Ptr channel;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = channels_.find(service_name);
        if (it == channels_.end()) {
            it = channels_.emplace(service_name, std::make_shared<Channels>()).first;
        }
        channel = it->second;
    }
    channel->AddChannel(addr_ip_and_port);
}
void ChannelManager::DelService(const std::string& service_name, const std::string& addr_ip_and_port)
{
    Channels::Ptr channel;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = channels_.find(service_name);
        if (it == channels_.end())
        {
            return;
        }
        channel = it->second;
    }
    channel->DelChannel(addr_ip_and_port);
}
ChannelPtr ChannelManager::GetChannel(const std::string& service_name)
{
    Channels::Ptr channel;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = channels_.find(service_name);
        if (it == channels_.end())
        {
            return nullptr;
        }
        channel = it->second;
    }
    return channel->GetChannel();
}

void ClosureFactory::ClosureCallback(std::shared_ptr<CallbackObject> callback_object)
{
    callback_object->callback();
}

google::protobuf::Closure* ClosureFactory::CreateClosure(Callback&& callback)
{
    std::shared_ptr<CallbackObject> callback_object = std::make_shared<CallbackObject>();
    callback_object->callback = std::move(callback);
    return brpc::NewCallback(ClosureCallback, callback_object);
}

std::shared_ptr<brpc ::Server> ServerFactory::CreateServer(int port, google::protobuf::Service* service)
{
    std::shared_ptr<brpc::Server> server = std::make_shared<brpc::Server>();
    int ret = server->AddService(service , brpc::SERVER_DOESNT_OWN_SERVICE); // 如果服务创建失败 , 会删除服务
    if(ret != 0)
    {
        return nullptr;
    }
    brpc::ServerOptions options;
    ret = server->Start(port , &options);
    if(ret != 0)
    {
        return nullptr;
    }
    return server;
}


} // namespace cpp_toolkit
