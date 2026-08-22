
#include "etcd.h"
#include <etcd/Client.hpp>

namespace cpp_toolkit {


bool WaitConnected(std::shared_ptr<etcd::Client> etcd_client)
{
    int wait_time = 1;
    while(!etcd_client->head().get().is_ok() && wait_time <= ETCD_MAX_WAIT_TIME)
    {
        std::this_thread::sleep_for(std::chrono::seconds(wait_time));
        wait_time *= 2;
    }
    return etcd_client->head().get().is_ok();
}

SvcProvider::SvcProvider(const std::string &etcd_center_addr , const std::string &service_name, const std::string &service_addr, const std::string &instance_id)
:reconnect_time_(1) ,
etcd_center_addr_(etcd_center_addr) ,
service_name_(service_name) ,
service_addr_(service_addr) ,
instance_id_(instance_id)
{
}

std::string SvcProvider::GenerateKey()
{
    std::string id = instance_id_.empty() ? service_addr_ : instance_id_;
    return "/" + service_name_ + "/" + id;
}

bool SvcProvider::Registry(int ttl)
{
    std::shared_ptr<etcd::Client> etcd_client = std::make_shared<etcd::Client>(etcd_center_addr_);
    if(!WaitConnected(etcd_client))
    {
        return false;
    }

    auto lease_resp = etcd_client->leasegrant(ttl).get();
    if(!lease_resp.is_ok())
    {
        return false;
    }
    auto lease_id = lease_resp.value().lease();
    auto put_resp = etcd_client->put(GenerateKey(), service_addr_, lease_id).get();
    if(!put_resp.is_ok())
    {
        return false;
    }

    auto callback = [this, ttl](std::exception_ptr eptr)
    {
        if(reconnect_time_ > ETCD_MAX_RECONNECT_TIME)
        {
            return;
        }
        std::thread([this, ttl]{
            std::this_thread::sleep_for(std::chrono::seconds(reconnect_time_));
            if (this->Registry(ttl)) 
            {
                reconnect_time_ = 1;        // 重连成功 → 重置退避
            } 
            else 
            {
                reconnect_time_ = reconnect_time_ * 2; // 失败 → 翻倍
            }
        }).detach();
    };

    etcd_keep_alive_.reset(new etcd::KeepAlive(etcd_center_addr_ , callback, ttl , lease_id));
    return true;
}

SvcWatcher::SvcWatcher(const std::string &etcd_center_addr , WatchCallback&& online_callback , WatchCallback&& offline_callback)
:reconnect_time_(0) ,
etcd_center_addr_(etcd_center_addr) ,
online_callback_(std::move(online_callback)) ,
offline_callback_(std::move(offline_callback))
{
}

std::string SvcWatcher::ParseKey(const std::string &key)
{
    size_t end = key.find_last_of('/');
    size_t begin = key.find('/');
    if(begin == std::string::npos || end == std::string::npos || begin >= end)
    {
        return "";
    }
    return key.substr(begin + 1 , end - begin - 1); 
}
void SvcWatcher::HandleWatchEvent(const etcd::Response &resp)
{
    if(!resp.is_ok())
    {
        return;
    }
    const auto& events = resp.events();
    for(const auto& event : events)
    {
        
        if(event.event_type() == etcd::Event::EventType::PUT)
        {
            std::string key = ParseKey(event.kv().key());
            std::string value = event.kv().as_string();
            if(online_callback_)
                online_callback_(key, value);
        }
        else if(event.event_type() == etcd::Event::EventType::DELETE_)
        {
            std::string prev_key = ParseKey(event.prev_kv().key());
            std::string prev_value = event.prev_kv().as_string();
            if(offline_callback_)
                offline_callback_(prev_key, prev_value);
        }
    }
}

bool SvcWatcher::Watch(const std::string &key_prefix)
{
    std::shared_ptr<etcd::Client> etcd_client = std::make_shared<etcd::Client>(etcd_center_addr_);
    if(!WaitConnected(etcd_client))
    {
        return false;
    }

    auto resp = etcd_client->ls(key_prefix , 0).get();
    if(!resp.is_ok())
    {
        return false;
    }
    for(const auto& value : resp.values())
    {
        std::string key = ParseKey(value.key());
        if(online_callback_)
            online_callback_(key, value.as_string());
    }

    etcd_watcher_.reset(new etcd::Watcher(etcd_center_addr_ , key_prefix , std::bind(&SvcWatcher::HandleWatchEvent, this , std::placeholders::_1) , true));
    etcd_watcher_->Wait([this, key_prefix](bool cond) {
        if(cond)
        {
            return;
        }
        if (reconnect_time_ > ETCD_MAX_RECONNECT_TIME) {
            return;
        }
        std::this_thread::sleep_for(std::chrono::seconds(reconnect_time_));

        if (this->Watch(key_prefix)) 
        {
            reconnect_time_ = 1;
        } 
        else 
        {
            reconnect_time_ = reconnect_time_ * 2;
        }
    });
    return true;
}

} // namespace cpp_toolkit
