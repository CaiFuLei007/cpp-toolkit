
#pragma once

/*
    对 etcd 进行封装
    1. SvcProvider 服务注册类 : 将添加数据和数据保活结合 , 实现服务注册
    2. SvcWatcher 服务发现类 : 将获取数据和数据监控结合 , 实现服务发现
*/

#include <etcd/Client.hpp>
#include <etcd/KeepAlive.hpp>
#include <etcd/Watcher.hpp>
#include <string>
#include <memory>
#include <functional>
#include <atomic>

namespace cpp_toolkit {

constexpr size_t ETCD_MAX_WAIT_TIME = 60;
constexpr size_t ETCD_MAX_RECONNECT_TIME = 60;

class SvcProvider {
public:
    using Ptr = std::shared_ptr<SvcProvider>;
private:
    std::atomic<size_t> reconnect_time_;
    std::string etcd_center_addr_;
    std::string service_name_;
    std::string service_addr_;
    std::string instance_id_;
    std::shared_ptr<etcd::KeepAlive> etcd_keep_alive_;

private:
    std::string GenerateKey();
public:
    SvcProvider(const std::string &etcd_center_addr , const std::string &service_name, const std::string &service_addr, const std::string &instance_id = "");
    bool Registry(int ttl);
};


class SvcWatcher {
public:
    using Ptr = std::shared_ptr<SvcWatcher>;
    using WatchCallback = std::function<void(const std::string &key, const std::string &value)>;
private:
    std::atomic<size_t> reconnect_time_;
    std::string etcd_center_addr_;
    WatchCallback online_callback_;
    WatchCallback offline_callback_;
    std::shared_ptr<etcd::Watcher> etcd_watcher_;

private:
    std::string ParseKey(const std::string &key);
    void HandleWatchEvent(const etcd::Response &resp);
public:
    SvcWatcher(const std::string &etcd_center_addr , WatchCallback&& online_callback , WatchCallback&& offline_callback);
    bool Watch(const std::string &key_prefix);
};

} // namespace cpp_toolkit
